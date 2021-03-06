//***********************************************************************************************
//Common constants and structs for Clocked and Clkd
//***********************************************************************************************

#ifndef CLOCK_COMMON_HPP
#define CLOCK_COMMON_HPP


#include "ImpromptuModular.hpp"


//struct ClockCommon {


static const float ratioValues[34] = {1, 1.5, 2, 2.5, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, 23, 24, 29, 31, 32, 37, 41, 43, 47, 48, 53, 59, 61, 64};

static const int bpmMax = 300;
static const int bpmMin = 30;

static const unsigned int ON_STOP_INT_RST_MSK = 0x1;
static const unsigned int ON_START_INT_RST_MSK = 0x2;
static const unsigned int ON_STOP_EXT_RST_MSK = 0x4;
static const unsigned int ON_START_EXT_RST_MSK = 0x8;



	
struct RatioParam : ParamQuantity {
	float getDisplayValue() override {
		int knobVal = (int) std::round(getValue());
		knobVal = clamp(knobVal, (34 - 1) * -1, 34 - 1);
		if (knobVal < 0) {
			knobVal *= -1;
		}
		return ratioValues[knobVal];
	}
	void setDisplayValue(float displayValue) override {}	
	std::string getUnit() override {
		if (getValue() >= 0.0f) return std::string("x");
		return std::string("÷");
	}
};


struct ResetModeBitToggleItem : MenuItem {
	unsigned int *resetOnStartStopPtr;
	unsigned int mask;
	void onAction(const event::Action &e) override {
		*resetOnStartStopPtr ^= mask;
	}
};

struct OnStartItem : MenuItem {
	unsigned int *resetOnStartStopPtr;

	Menu *createChildMenu() override {
		Menu *menu = new Menu;

		ResetModeBitToggleItem *startIntItem = createMenuItem<ResetModeBitToggleItem>("Do internal reset", CHECKMARK(*resetOnStartStopPtr & ON_START_INT_RST_MSK));
		startIntItem->resetOnStartStopPtr = resetOnStartStopPtr;
		startIntItem->mask = ON_START_INT_RST_MSK;
		menu->addChild(startIntItem);

		ResetModeBitToggleItem *startExtItem = createMenuItem<ResetModeBitToggleItem>("Send reset pulse", CHECKMARK(*resetOnStartStopPtr & ON_START_EXT_RST_MSK));
		startExtItem->resetOnStartStopPtr = resetOnStartStopPtr;
		startExtItem->mask = ON_START_EXT_RST_MSK;
		menu->addChild(startExtItem);

		return menu;
	}
};	

struct OnStopItem : MenuItem {
	unsigned int *resetOnStartStopPtr;

	Menu *createChildMenu() override {
		Menu *menu = new Menu;

		ResetModeBitToggleItem *stopIntItem = createMenuItem<ResetModeBitToggleItem>("Do internal reset", CHECKMARK(*resetOnStartStopPtr & ON_STOP_INT_RST_MSK));
		stopIntItem->resetOnStartStopPtr = resetOnStartStopPtr;
		stopIntItem->mask = ON_STOP_INT_RST_MSK;
		menu->addChild(stopIntItem);

		ResetModeBitToggleItem *stopExtItem = createMenuItem<ResetModeBitToggleItem>("Send reset pulse", CHECKMARK(*resetOnStartStopPtr & ON_STOP_EXT_RST_MSK));
		stopExtItem->resetOnStartStopPtr = resetOnStartStopPtr;
		stopExtItem->mask = ON_STOP_EXT_RST_MSK;
		menu->addChild(stopExtItem);

		return menu;
	}
};	


#endif
	
