#include "KWAModules.hpp"


struct SequencerController : Module {
	enum ParamIds {
		CONTINUOUS_MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		CONTINUOUS_MODE_LIGHT,
		NUM_LIGHTS
	};

	dsp::PulseGenerator eocOutputPulse;
	dsp::SchmittTrigger inputTriggerSchmitt;
	dsp::SchmittTrigger inputResetSchmitt;

	// Link modules
	float leftMessages[2][8] = {};
	float rightMessages[2][8] = {};

	// Input Signals
	float signalInputMasterEoc = 0;

	//State vars
	bool clockTriggered = false;
	bool resetTriggered = false;
	bool continuousModeActive = false;
	bool continousModeActiveEocPulsed = false;

	SequencerController() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configButton(CONTINUOUS_MODE_PARAM, "Continuous Mode");

		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
	}

	void process(const ProcessArgs& args) override {

		bool continuousModeActive = params[CONTINUOUS_MODE_PARAM].getValue() > 0.f;
		lights[CONTINUOUS_MODE_LIGHT].setBrightness(continuousModeActive);

			//Continuous mode eoc pulse
		if (continuousModeActive && !continousModeActiveEocPulsed){
			eocOutputPulse.trigger(1e-3f);
			continousModeActiveEocPulsed = true;
		}
		if (!continuousModeActive){
			//Continuous mode is not active so we can prepare to send a pulse when it is
			continousModeActiveEocPulsed = false;
		}

		// Clock / Trigger input
		const bool is_momma = rightExpander.module && (rightExpander.module->model == modelEightGateSequencer
			|| rightExpander.module->model == modelEightGateSequencerChild);
		bool clockTriggered = false;
		if (inputTriggerSchmitt.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 1.f)){
			clockTriggered = true;
		}

		// Read Right
		if (is_momma) {
			float* rightMessage = (float*)rightExpander.consumerMessage;
			//[0]
			signalInputMasterEoc = rightMessage[0];
		}
		
		// Reset input
		if (inputResetSchmitt.process((inputs[RESET_INPUT].getVoltage() > signalInputMasterEoc ? inputs[RESET_INPUT].getVoltage() : signalInputMasterEoc), 0.1f, 1.f)) {
			eocOutputPulse.trigger(1e-3f);
			resetTriggered = true;
		}

		// Send right
		if (is_momma) {
			float* rightMessage = (float*)rightExpander.module->leftExpander.producerMessage;

			// Write message
			rightMessage[0] = clockTriggered;
			rightMessage[1] = resetTriggered;
			rightMessage[2] = continuousModeActive;
			rightMessage[3] = eocOutputPulse.process(args.sampleTime) ? 10.f : 0.f;
			
			// Flip messages at the end of the timestep
			rightExpander.module->leftExpander.messageFlipRequested = true;
		}
		//reinit messages
		resetTriggered = false;
		clockTriggered = false;
	}
};


struct SequencerControllerWidget : ModuleWidget {
	SequencerControllerWidget(SequencerController* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/n_SequencerController.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		//addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		//addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(5.08, 63.35)), module, SequencerController::CONTINUOUS_MODE_PARAM, SequencerController::CONTINUOUS_MODE_LIGHT));

		addInput(createInputCentered<DarkPJ301MPort>(mm2px(Vec(5.08, 13)), module, SequencerController::CLOCK_INPUT));
		addInput(createInputCentered<DarkPJ301MPort>(mm2px(Vec(5.08, 29.251)), module, SequencerController::RESET_INPUT));
	}
};


Model* modelSequencerController = createModel<SequencerController, SequencerControllerWidget>("SequencerController");
