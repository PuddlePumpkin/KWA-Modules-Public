#include "KWAModules.hpp"


struct SixtyFourGatePitchSeq : Module {
	enum ParamIds {
		ENUMS(BUTTON_MATRIX_PARAMS, 64),
		ENUMS(GATE_PARAMS, 640),
		ENUMS(VOCT_PARAMS, 640),
		ENUMS(HAS_RECORDED_PARAMS, 640),
		ENUMS(VELOCITY_PARAMS, 640),
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
	

	//State vars
	int currentStep = 0;
	int currentSubstep = 0;
	int currentWorkingStep = 0;
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
	bool gateModifiedSinceRelease = false;
	int processCounter = 0;

	struct Engine {
		float voctVoltage = 0.f;
		bool hasTriggerOutputPulsedThisStep = false;
		bool isGateHigh = false;
		bool gateFellThisSample = false;
		bool gateRoseThisSample = false;
		bool voctMovedThisSample = false;
		dsp::PulseGenerator triggerOutputPulse;
	};
	Engine engines[10];

	SixtyFourGatePitchSeq();

	void process(const ProcessArgs& args) override;

	void processFifty(const ProcessArgs& args, int channels);

	void setModeBrightnesses();
	
	void updateDisplay(int channels);

	void clearNoteParams(int i, int channel);
};

struct SixtyFourGatePitchSeqWidget : ModuleWidget {
	SixtyFourGatePitchSeqWidget(SixtyFourGatePitchSeq* module);
};


Model* modelSixtyFourGatePitchSeq = createModel<SixtyFourGatePitchSeq, SixtyFourGatePitchSeqWidget>("SixtyFourGatePitchSeq");
