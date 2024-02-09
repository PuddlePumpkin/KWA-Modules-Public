#include "KWAModules.hpp"


struct EightGateSequencerChild : Module {
	enum ParamIds {
		ENUMS(GATE_PARAMS, 8),
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		TRIG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(GATE_LIGHTS_PRESSED, 8),
		ENUMS(GATE_LIGHTS_STEPPED, 8),
		NUM_LIGHTS
	};
	//Input Schmitts
	dsp::SchmittTrigger clockInputSchmitt;
	dsp::SchmittTrigger resetInputSchmitt;
	dsp::SchmittTrigger eocInputSchmitt;

	//Signal Output Pulses
	dsp::PulseGenerator clockOutputPulse;
	dsp::PulseGenerator resetOutputPulse;
	dsp::PulseGenerator eocOutputPulse;
	dsp::PulseGenerator triggerOutputPulse;

	// Link modules
	float leftMessages[2][8] = {};
	float rightMessages[2][8] = {};

	// Input Signals
	float signalInputClock = 0;
	float signalInputReset = 0;
	float signalInputContinousMode = 0;
	float signalInputEOC = 0;
	float signalInputGlobalTrig = 0;
	float signalInputMasterEoc = 0;

	//State vars
	int activeNode = 0;
	bool clockSchmittTriggered = false;
	bool clockPulseTriggered = false;
	bool resetSchmittTriggered = false;
	bool triggerOutputPulseTriggered = false;
	bool eocOutputPulseTriggered = false;
	bool skipEoc = false;
	bool continuousModeActive = false;

	EightGateSequencerChild() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < 8; i++) {
			configSwitch(GATE_PARAMS + i, 0.f, 1.f, 0.f, string::f("%d Enabled", i + 1));
		}
		configOutput(TRIG_OUTPUT, "Trigger");

		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
	}

	void process(const ProcessArgs& args) override {

		// Set Parent child Associations
		const bool is_momma = rightExpander.module && (rightExpander.module->model == modelEightGateSequencer
			|| rightExpander.module->model == modelEightGateSequencerChild
			|| rightExpander.module->model == modelSequencerEnd);
		const bool is_baby = leftExpander.module && (leftExpander.module->model == modelEightGateSequencer
			|| leftExpander.module->model == modelSequencerController
			|| leftExpander.module->model == modelEightGateSequencerChild);
		const bool is_baby_of_controller = leftExpander.module && (leftExpander.module->model == modelSequencerController);

		// Read Left Signal Inputs
		if (is_baby) {
			float* leftMessage = (float*)leftExpander.consumerMessage;
			//[0] - Clock
			signalInputClock= leftMessage[0];
			//[1] - Reset
			signalInputReset = leftMessage[1];
			//[2] - Continuous Mode
			signalInputContinousMode = (leftMessage[2] > 0.0f);
			//[3] - EOC
			signalInputEOC = leftMessage[3];
			//[4] - Global Trigger
			signalInputGlobalTrig = leftMessage[4];
		}

		//Contiunous mode state activation
		if (signalInputContinousMode && !continuousModeActive) {
			activeNode = -1;
			continuousModeActive = true;
			//When we first enter continuous mode, we need to skip eoc or it will fire immediately
			skipEoc = true;			
		}
		if (!signalInputContinousMode) {
			continuousModeActive = false;
		}

		// Clock Schmitt
		if (clockInputSchmitt.process(signalInputClock, 0.1f, 1.f)) {
			// if we're on -1 but not continuous mode, step
			if(activeNode != -1 || !signalInputContinousMode){
				if (activeNode == 7) {
					activeNode = (signalInputContinousMode ? -1 : 0);
				} 
				else {
					skipEoc = false;
					activeNode += 1;
					eocOutputPulseTriggered = false;
				}
			}
			clockSchmittTriggered = true;
			//Make sure we can trigger again after each step
			triggerOutputPulseTriggered = false;
		}

		//Eoc Out Pulse
		if (activeNode == -1 && !eocOutputPulseTriggered) {
			if(!skipEoc){
				eocOutputPulse.trigger(1e-3f);
			}
			eocOutputPulseTriggered = true;
			skipEoc = false;
		}
		if(!signalInputContinousMode && activeNode == 0 && !eocOutputPulseTriggered){
			if(!skipEoc){
				eocOutputPulse.trigger(1e-3f);
			}
			eocOutputPulseTriggered = true;
			skipEoc = false;
		}

		// Eoc Schmitt
		if (eocInputSchmitt.process(signalInputEOC, 0.1f, 1.f) && signalInputContinousMode) {
			activeNode +=1;
		}

		// Reset Schmitt 
		if (resetInputSchmitt.process(signalInputReset, 0.1f, 1.f)) {
			activeNode = (signalInputContinousMode ? -1 : 0);
			resetSchmittTriggered = true;
			skipEoc = !is_baby_of_controller;
			activeNode = is_baby_of_controller ? 0 : activeNode;
		}

		// Iterate Button Rows
		for (int i = 0; i < 8; i++) {
			const bool isButtonPressed = params[GATE_PARAMS + i].getValue() > 0.f;
			lights[GATE_LIGHTS_STEPPED + i].setBrightness(i == activeNode);
			if(i == activeNode && isButtonPressed && !triggerOutputPulseTriggered){
					triggerOutputPulse.trigger(1e-3f);
					triggerOutputPulseTriggered = true;
			}
			// Set lights
			lights[GATE_LIGHTS_PRESSED + i].setBrightness(isButtonPressed);
		}
		
		// Read Right
		if (is_momma) {
			float* rightMessage = (float*)rightExpander.consumerMessage;
			//[0] - EOC Signal
			signalInputMasterEoc = rightMessage[0];
		}else{
			signalInputMasterEoc = eocOutputPulse.process(args.sampleTime) ? 10.f : 0.f;
		}
		// Send Left
		if (is_baby) {
			float* leftSendMessage = (float*)leftExpander.module->rightExpander.producerMessage;
			//[0] - EOC Signal
			leftSendMessage[0] = signalInputMasterEoc;
			leftExpander.module->rightExpander.messageFlipRequested = true;
		}

		float processedPulseVal = triggerOutputPulse.process(args.sampleTime) ? 10.f : 0.f;
		// Send Right
		if (is_momma) {
			float* rightMessage = (float*)rightExpander.module->leftExpander.producerMessage;
			//[0] - Clock
			rightMessage[0] = signalInputClock;
			//[1] - Reset
			rightMessage[1] = signalInputReset;
			//[2] - Continuous mode
			rightMessage[2] = signalInputContinousMode;
			//[3] - EOC
			rightMessage[3] = eocOutputPulse.process(args.sampleTime) ? 10.f : 0.f;
			//[4] - Global Trigger
			rightMessage[4] = processedPulseVal > signalInputGlobalTrig ? processedPulseVal : signalInputGlobalTrig;

			// Flip messages at the end of the timestep
			rightExpander.module->leftExpander.messageFlipRequested = true;
		}
		//reinit message variables
		clockPulseTriggered = false;
		clockSchmittTriggered = false;
		resetSchmittTriggered = false;
		//Set output voltage
		outputs[TRIG_OUTPUT].setVoltage(processedPulseVal);
	}

	void invert() {
		for (int i = 0; i < 8; i++) {
			params[GATE_PARAMS + i].setValue(!params[GATE_PARAMS + i].getValue());
		}
	}
};


struct EightGateSequencerChildWidget : ModuleWidget {
	EightGateSequencerChildWidget(EightGateSequencerChild* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/n_EightGateSequencerChild.svg")));

		// create screws
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// create lights/buttons
		for (int i = 0; i < 8; i++) {
			addChild(createLightCentered<MidsizeButtonLight<GreenLight>>(mm2px(Vec(7.62, 13 + (i*12.43))), module, EightGateSequencerChild::GATE_LIGHTS_STEPPED + i));
			addParam(createLightParamCentered<LightButton<MidSizeButton, MidsizeButtonCenterLight<BlueLight>>>(mm2px(Vec(7.62, 13 + (i*12.43))), module, EightGateSequencerChild::GATE_PARAMS + i, EightGateSequencerChild::GATE_LIGHTS_PRESSED + i));
		}

		// output
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(7.62, 117)), module, EightGateSequencerChild::TRIG_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		EightGateSequencerChild* module = dynamic_cast<EightGateSequencerChild*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator);

		menu->addChild(createMenuItem("Invert Gates", "",
			[=]() {module->invert();}
		));
	}
};


Model* modelEightGateSequencerChild = createModel<EightGateSequencerChild, EightGateSequencerChildWidget>("EightGateSequencerChild");
