//***********************************************************************************************
//Four channel note viewer module for VCV Rack by Marc Boulé
//
//Based on code from the Fundamental and AudibleInstruments plugins by Andrew Belt 
//and graphics from the Component Library by Wes Milholen 
//See ./LICENSE.md for all licenses
//See ./res/fonts/ for font licenses
//***********************************************************************************************


#include "ImpromptuModular.hpp"


struct ChordKeyExpander : Module {
	enum ParamIds {
		ENUMS(OCT_PARAMS, 4),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CV_INPUTS, 4),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(CV_OUTPUTS, 4),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	// Constants
	const float unusedValue = -100.0f;

	// Expander
	float leftMessages[2][5] = {};// messages from mother (CvPad or ChordKey): 4 CV values, panelTheme

	// Need to save, no reset
	// none
	
	// Need to save, with reset
	// none
	
	// No need to save, with reset
	float chordValues[4];
	bool enabledNotes[12];
	int ranges[24];

	// No need to save, no reset
	int panelTheme;
	RefreshCounter refresh;

	
	ChordKeyExpander() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		onReset();
		
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
		
		char strBuf[32];
		for (int c = 0; c < 4; c++) {
			snprintf(strBuf, 32, "Oct channel %i", c + 1);
			configParam(OCT_PARAMS + c, -4.0f, 4.0f, 0.0f, strBuf);
		}
		
		panelTheme = (loadDarkAsDefault() ? 1 : 0);
	}
	

	void onReset() override {
		resetNonJson();
	}
	void resetNonJson() {
		for (int i = 0; i < 4; i++) {
			chordValues[i] = unusedValue;
		}
		fillEnabledNotes();// uses chordValues[]
		updateRanges();// uses enabledNotes[]
	}
	
	void onRandomize() override {
	}

	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		resetNonJson();
	}

	
	void process(const ProcessArgs &args) override {
		
		if (refresh.processInputs()) {
			bool motherPresent = (leftExpander.module && leftExpander.module->model == modelChordKey);
			if (motherPresent) {
				// From Mother
				float *messagesFromMother = (float*)leftExpander.consumerMessage;
				for (int i = 0; i < 4; i++) {
					chordValues[i] = messagesFromMother[i];
				}
				panelTheme = clamp((int)(messagesFromMother[4] + 0.5f), 0, 1);
			}	
			else {
				for (int i = 0; i < 4; i++) {
					chordValues[i] = unusedValue;
				}
			}
			
		}// userInputs refresh
		
		
		// outputs
		for (int i = 0; i < 4; i++) {
			if (outputs[CV_OUTPUTS + i].isConnected()) {
				for (int c = 0; c < std::max(outputs[CV_OUTPUTS + i].getChannels(), 1); c++) {
					float pitch = params[OCT_PARAMS + i].getValue();
					if (inputs[CV_INPUTS + i].isConnected()) {
						pitch += inputs[CV_INPUTS + i].getVoltage(c);
					}
					// the next five lines are by Andrew Belt and are from the Fundamental Quantizer
					int range = std::floor(pitch * 24);
					int octave = eucDiv(range, 24);
					range -= octave * 24;
					int note = ranges[range] + octave * 12;
					pitch = float(note) / 12;
					outputs[CV_OUTPUTS + i].setVoltage(pitch, c);
				}
			}
		}
		
		// lights
		if (refresh.processLights()) {
			fillEnabledNotes();// uses chordValues[]
			updateRanges();// uses enabledNotes[]
			
			for (int i = 0; i < 4; i++) {
				outputs[CV_OUTPUTS + i].setChannels(inputs[CV_INPUTS + i].getChannels());
			}
		}// lightRefreshCounter
		
		if (refresh.processInputs()) {
			// To Expander
			if (rightExpander.module && (rightExpander.module->model == modelFourView || rightExpander.module->model == modelChordKeyExpander)) {
				float *messageToExpander = (float*)(rightExpander.module->leftExpander.producerMessage);
				for (int i = 0; i < 4; i++) {
					messageToExpander[i] = chordValues[i];
				}
				messageToExpander[4] = (float)panelTheme;
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}		
	}// process()
	
	
	void fillEnabledNotes() {// uses chordValues[]
		for (int i = 0; i < 12; i++) {
			enabledNotes[i] = false;
		}
		for (int i = 0; i < 4; i++) {
			if (chordValues[i] != unusedValue) {
				float noteScaled = (clamp(chordValues[i], -10.0f, 10.0f) + 10.0f) * 12.0f;
				int intNote = ((int)std::round(noteScaled));
				int baseNote = intNote % 12;// 0 to 11
				enabledNotes[baseNote] = true;
			}
		}
	}
	
	
	// This method is by Andrew Belt and is from the Fundamental Quantizer
	void updateRanges() {
		// Check if no notes are enabled
		bool anyEnabled = false;
		for (int note = 0; note < 12; note++) {
			if (enabledNotes[note]) {
				anyEnabled = true;
				break;
			}
		}
		// Find closest notes for each range
		for (int i = 0; i < 24; i++) {
			int closestNote = 0;
			int closestDist = INT_MAX;
			for (int note = -12; note <= 24; note++) {
				int dist = std::abs((i + 1) / 2 - note);
				// Ignore enabled state if no notes are enabled
				if (anyEnabled && !enabledNotes[eucMod(note, 12)]) {
					continue;
				}
				if (dist < closestDist) {
					closestNote = note;
					closestDist = dist;
				}
				else {
					// If dist increases, we won't find a better one.
					break;
				}
			}
			ranges[i] = closestNote;
		}
	}	
};


struct ChordKeyExpanderWidget : ModuleWidget {
	SvgPanel* darkPanel;

	struct PanelThemeItem : MenuItem {
		ChordKeyExpander *module;
		void onAction(const event::Action &e) override {
			module->panelTheme ^= 0x1;
		}
	};
	
	ChordKeyExpanderWidget(ChordKeyExpander *module) {
		setModule(module);
		
		// Main panels from Inkscape
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/ChordKeyExpander.svg")));
        if (module) {
			darkPanel = new SvgPanel();
			darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dark/ChordKeyExpander_dark.svg")));
			darkPanel->visible = false;
			addChild(darkPanel);
		}

		// Screws
		addChild(createDynamicWidget<IMScrew>(Vec(15, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(15, 365), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 365), module ? &module->panelTheme : NULL));

		static const int col0 = 25;
		static const int col1 = 65;
		
		static const int rowD = 52;

		static const int row0 = 68;
		static const int row1 = row0 + rowD - 4;
		static const int row2 = row1 + rowD + 4;
		
		static const int row3 = 229;
		static const int row4 = row3 + rowD - 4;
		static const int row5 = row4 + rowD + 4;
		
		// Quantizer 1 (top left)
		addInput(createDynamicPortCentered<IMPort>(Vec(col0, row0), true, module, ChordKeyExpander::CV_INPUTS + 0, module ? &module->panelTheme : NULL));	
		addParam(createDynamicParamCentered<IMSmallKnob<true, true>>(Vec(col0, row1), module, ChordKeyExpander::OCT_PARAMS + 0, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(col0, row2), false, module, ChordKeyExpander::CV_OUTPUTS + 0, module ? &module->panelTheme : NULL));
		// Quantizer 2 (top right)
		addInput(createDynamicPortCentered<IMPort>(Vec(col1, row0), true, module, ChordKeyExpander::CV_INPUTS + 1, module ? &module->panelTheme : NULL));	
		addParam(createDynamicParamCentered<IMSmallKnob<true, true>>(Vec(col1, row1), module, ChordKeyExpander::OCT_PARAMS + 1, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(col1, row2), false, module, ChordKeyExpander::CV_OUTPUTS + 1, module ? &module->panelTheme : NULL));
		
		// Quantizer 3 (bot left)
		addInput(createDynamicPortCentered<IMPort>(Vec(col0, row3), true, module, ChordKeyExpander::CV_INPUTS + 2, module ? &module->panelTheme : NULL));	
		addParam(createDynamicParamCentered<IMSmallKnob<true, true>>(Vec(col0, row4), module, ChordKeyExpander::OCT_PARAMS + 2, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(col0, row5), false, module, ChordKeyExpander::CV_OUTPUTS + 2, module ? &module->panelTheme : NULL));
		// Quantizer 4 (bot right)
		addInput(createDynamicPortCentered<IMPort>(Vec(col1, row3), true, module, ChordKeyExpander::CV_INPUTS + 3, module ? &module->panelTheme : NULL));	
		addParam(createDynamicParamCentered<IMSmallKnob<true, true>>(Vec(col1, row4), module, ChordKeyExpander::OCT_PARAMS + 3, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(col1, row5), false, module, ChordKeyExpander::CV_OUTPUTS + 3, module ? &module->panelTheme : NULL));

	}
	
	void step() override {
		if (module) {
			panel->visible = ((((ChordKeyExpander*)module)->panelTheme) == 0);
			darkPanel->visible  = ((((ChordKeyExpander*)module)->panelTheme) == 1);
		}
		Widget::step();
	}
};

Model *modelChordKeyExpander = createModel<ChordKeyExpander, ChordKeyExpanderWidget>("Chord-Key-Expander");
