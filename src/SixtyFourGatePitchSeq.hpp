#include "KWAModules.hpp"


struct SixtyFourGatePitchSeq : Module {
	enum ParamIds {
		ENUMS(BUTTON_MATRIX_PARAMS, 64),
		ENUMS(GATE_PARAMS, 64),
		ENUMS(VOCT_PARAMS, 64),
		ENUMS(VOCT_HAS_WRITTEN_PARAMS, 64),
		ENUMS(VELOCITY_PARAMS, 64),
		RECORD_MODE_PARAM,
		EDIT_MODE_PARAM,
		GATE_MODE_PARAM,
		DISPLAY_MODE_PARAM,
		STEP_COUNT_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		GATE_INPUT,
		VOCT_INPUT,
		VELOCITY_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		VOCT_OUTPUT,
		VELOCITY_OUTPUT,
		CLOCK_OUTPUT,
		RESET_OUTPUT,
		EOC_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STEP_LIGHTS, 192),
		ENUMS(ACTIVE_STEP_LIGHTS, 64),
		RECORD_MODE_LIGHT,
		EDIT_MODE_LIGHT,
		GATE_MODE_LIGHT,
		NUM_LIGHTS
	};
	//Input Schmitt Triggers
	dsp::SchmittTrigger clockInputSchmitt;
	dsp::SchmittTrigger resetInputSchmitt;

	//Signal Output Pulses
	dsp::PulseGenerator clockOutputPulse;
	dsp::PulseGenerator eocOutputPulse;
	dsp::PulseGenerator triggerOutputPulse;

	//State vars
	bool hasTriggerOutputPulsedThisStep = false;
	int currentStep = 0;
	int currentSubstep = 0;
	int currentWorkingStep = 0;
	bool isInputGateOn = false;
	bool gateModeSelected = false;
	bool isRecording = false;
	bool editModeSelectedGates[64] = {false};
	bool shouldRefreshDisplay = false;
	bool wasButtonPressedThisSample = false;
	bool wasInitialized = false;
	bool anyPressed = false;
	float maxVoct = -10.f;
	float minVoct = 10.f;
	float maxVel = -10.f;
	float minVel = 10.f;
	
	int processCounter = 0;


	SixtyFourGatePitchSeq();

	void process(const ProcessArgs& args) override;

	void processFifty(const ProcessArgs& args);

	void setModeBrightnesses();
	
	void updateDisplay();

	void clearNoteParams(int i);
};

struct SixtyFourGatePitchSeqWidget : ModuleWidget {
	SixtyFourGatePitchSeqWidget(SixtyFourGatePitchSeq* module);
};


Model* modelSixtyFourGatePitchSeq = createModel<SixtyFourGatePitchSeq, SixtyFourGatePitchSeqWidget>("SixtyFourGatePitchSeq");
