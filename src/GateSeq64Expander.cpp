//***********************************************************************************************
//Expander module for GateSeq64, by Marc Boulé
//
//Based on code from the Fundamental and Audible Instruments plugins by Andrew Belt and graphics  
//  from the Component Library by Wes Milholen. 
//See ./LICENSE.md for all licenses
//See ./res/fonts/ for font licenses
//
//***********************************************************************************************


#include "ImpromptuModular.hpp"


struct GateSeq64Expander : Module {
	enum InputIds {
		GATE_INPUT,// needs connected
		PROB_INPUT,// needs connected
		WRITE_INPUT,
		WRITE1_INPUT,
		WRITE0_INPUT,
		STEPL_INPUT,
		NUM_INPUTS
	};


	// Expander
	float leftMessages[2][1] = {};// messages from mother


	// No need to save
	int panelTheme;
	unsigned int expanderRefreshCounter = 0;	


	GateSeq64Expander() {
		config(0, NUM_INPUTS, 0, 0);
		
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
		
		panelTheme = (loadDarkAsDefault() ? 1 : 0);
	}


	void process(const ProcessArgs &args) override {		
		expanderRefreshCounter++;
		if (expanderRefreshCounter >= expanderRefreshStepSkips) {
			expanderRefreshCounter = 0;
			
			bool motherPresent = (leftExpander.module && leftExpander.module->model == modelGateSeq64);
			if (motherPresent) {
				// To Mother
				float *messagesToMother = (float*)leftExpander.module->rightExpander.producerMessage;
				messagesToMother[0] = (inputs[0].isConnected() ? inputs[0].getVoltage() : std::numeric_limits<float>::quiet_NaN());
				messagesToMother[1] = (inputs[1].isConnected() ? inputs[1].getVoltage() : std::numeric_limits<float>::quiet_NaN());
				for (int i = 2; i < NUM_INPUTS; i++) {
					messagesToMother[i] = inputs[i].getVoltage();
				}
				leftExpander.module->rightExpander.messageFlipRequested = true;

				// From Mother
				float *messagesFromMother = (float*)leftExpander.consumerMessage;
				panelTheme = clamp((int)(messagesFromMother[0] + 0.5f), 0, 1);
			}		
		}// expanderRefreshCounter
	}// process()
};


struct GateSeq64ExpanderWidget : ModuleWidget {
	SvgPanel* darkPanel;
	
	GateSeq64ExpanderWidget(GateSeq64Expander *module) {
		setModule(module);
	
		// Main panels from Inkscape
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/GateSeq64Expander.svg")));
        if (module) {
			darkPanel = new SvgPanel();
			darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dark/GateSeq64Expander_dark.svg")));
			darkPanel->visible = false;
			addChild(darkPanel);
		}
		
		// Screws
		addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 365), module ? &module->panelTheme : NULL));

		// Expansion module
		static const int rowRulerExpTop = 60;
		static const int rowSpacingExp = 50;
		static const int colRulerExp = 497 - 30 - 90 - 360;// GS64 is (2+6)HP less than PS32
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 0), true, module, GateSeq64Expander::WRITE_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 1), true, module, GateSeq64Expander::GATE_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 2), true, module, GateSeq64Expander::PROB_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 3), true, module, GateSeq64Expander::WRITE0_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 4), true, module, GateSeq64Expander::WRITE1_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 5), true, module, GateSeq64Expander::STEPL_INPUT, module ? &module->panelTheme : NULL));
	}
	
	void step() override {
		if (module) {
			panel->visible = ((((GateSeq64Expander*)module)->panelTheme) == 0);
			darkPanel->visible  = ((((GateSeq64Expander*)module)->panelTheme) == 1);
		}
		Widget::step();
	}
};

Model *modelGateSeq64Expander = createModel<GateSeq64Expander, GateSeq64ExpanderWidget>("Gate-Seq-64-Expander");
