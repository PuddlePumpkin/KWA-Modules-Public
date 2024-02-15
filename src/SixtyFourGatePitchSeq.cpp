#include "SixtyFourGatePitchSeq.hpp"

SixtyFourGatePitchSeq::SixtyFourGatePitchSeq() {
	//buttons
	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	for (int i = 0; i < 64; i++) {
		configButton(BUTTON_MATRIX_PARAMS + i, string::f("%d", i + 1));
		configParam(VOCT_PARAMS + i, -10, 10, 0);
		configParam(VELOCITY_PARAMS + i, -10, 10, 0);
		configParam(GATE_PARAMS + i, 0, 10, 0);
		configParam(HAS_RECORDED_PARAMS + i, 0, 10, 0);
	}
	configButton(RECORD_MODE_PARAM, "Record");
	configButton(EDIT_MODE_PARAM, "Edit Mode");
	configSwitch(GATE_MODE_PARAM, 0.f, 10.f, 10.f,"Gate Mode");

	//knobs
	configSwitch(DISPLAY_MODE_PARAM, 1.f, 3.f, 2.f, "Display Mode",{"Gates", "V/Oct", "Velocity"});
	getParamQuantity(DISPLAY_MODE_PARAM)->snapEnabled = true;
	configParam(STEP_COUNT_PARAM, 1.f, 64.f, 8.f, "Steps");
	getParamQuantity(STEP_COUNT_PARAM)->snapEnabled = true;

	//inputs
	configInput(CLOCK_INPUT, "Clock");
	configInput(RESET_INPUT, "Reset");
	configInput(GATE_INPUT, "Gate");
	configInput(VOCT_INPUT, "V/Oct");
	configInput(VELOCITY_INPUT, "Velocity");

	//outputs
	configOutput(CLOCK_OUTPUT, "Clock");
	configOutput(RESET_OUTPUT, "Reset");
	configOutput(EOC_OUTPUT, "EOC");

	configOutput(GATE_OUTPUT, "Gate");
	configOutput(VOCT_OUTPUT, "V/Oct");
	configOutput(VELOCITY_OUTPUT, "Velocity");
}

void SixtyFourGatePitchSeq::process(const ProcessArgs& args) {
	//less often process - for sample insensitive code
	if (processCounter >= 49) {
		processFifty(args);
		processCounter = 0;
	}
	processCounter++;

	// Gate Rise + Fall
	bool gateFellThisSample = false;
	bool gateRoseThisSample = false;
	float gateVoltage = inputs[GATE_INPUT].getVoltage();
			// HIGH to LOW
		if (isGateHigh) {
			if (gateVoltage <= 0.1f) {
				isGateHigh = false;
				gateFellThisSample = true;
			}
		}
		else {
			// LOW to HIGH
			if (gateVoltage >= 2.f) {
				isGateHigh = true;
				gateRoseThisSample = true;
			}
		}
	
	// Voct Movement Detection
	bool voctMovedThisSample = false;
	float newVoctVoltage = inputs[VOCT_INPUT].getVoltage();
	if ((newVoctVoltage != voctVoltage) && isGateHigh){
		voctMovedThisSample = true;
		voctVoltage = newVoctVoltage;
	}

	// Clock
	if (clockInputSchmitt.process(inputs[CLOCK_INPUT].getVoltage(),0.1f, 1.f)) {
		currentSubstep += 1;
		if (currentSubstep >= 4){
			if (currentStep >= params[STEP_COUNT_PARAM].getValue() - 1) {
				currentStep =  0;
				eocOutputPulse.trigger(1e-3f);
			} 
			else {
				currentStep += 1;
			}
			if(params[GATE_PARAMS + currentStep].getValue() > 0){
				if(!hasTriggerOutputPulsedThisStep){
					triggerOutputPulse.trigger(1e-3f);
					hasTriggerOutputPulsedThisStep = true;
				}
			}
			currentSubstep = 0;
			clockOutputPulse.trigger(1e-3f);
			hasTriggerOutputPulsedThisStep = false;
		}
		// Quantize substep to a working step
		if (currentSubstep >= 3){
			currentWorkingStep = (currentStep == params[STEP_COUNT_PARAM].getValue()-1) ? 0 : currentStep + 1;
		}else{
			currentWorkingStep = currentStep;
		}
		//Playback other voltages
		if(params[HAS_RECORDED_PARAMS + currentStep].getValue() > 0 && !(inputs[GATE_INPUT].getVoltage() > 0)){
			outputs[VOCT_OUTPUT].setVoltage(params[VOCT_PARAMS + currentStep].getValue());
			outputs[VELOCITY_OUTPUT].setVoltage(params[VELOCITY_PARAMS + currentStep].getValue());
		}
		shouldRefreshDisplay = true;
	}

	// On input gate rise
	if (gateRoseThisSample || voctMovedThisSample) {
		// recording
		if(isRecording){
			params[GATE_PARAMS + currentWorkingStep].setValue(10.f);
			params[HAS_RECORDED_PARAMS + currentWorkingStep].setValue(10.f);
			params[VOCT_PARAMS + currentWorkingStep].setValue(inputs[VOCT_INPUT].getVoltage());
			params[VELOCITY_PARAMS + currentWorkingStep].setValue(inputs[VELOCITY_INPUT].getVoltage());

		// in edit mode
		} else if (!gateModeSelected) { 
			int selectedCount = 0;
			int lastSelectedIndex = 0;
			for(int i = 0; i < 64; i++){
				if(editModeSelectedGates[i]){
					params[GATE_PARAMS + i].setValue(10.f);
					params[HAS_RECORDED_PARAMS + i].setValue(10.f);
					params[VOCT_PARAMS + i].setValue(inputs[VOCT_INPUT].getVoltage());
					params[VELOCITY_PARAMS + i].setValue(inputs[VELOCITY_INPUT].getVoltage());
					editModeSelectedGates[i] = false;
					selectedCount++;
					lastSelectedIndex = i;
				}
			}
			// only 1 gate was edited, step to next
			if(selectedCount == 1){
				editModeSelectedGates[lastSelectedIndex >= (params[STEP_COUNT_PARAM].getValue() - 1) ? 0 : lastSelectedIndex + 1] = true;
			}
		}
		
	}

	// Reset - Schmitt trigger
	if (resetInputSchmitt.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f)) {
		currentStep = 0;
		currentSubstep = 0;
		shouldRefreshDisplay = true;
	}

	if(gateFellThisSample){
		outputs[GATE_OUTPUT].setVoltage(0.f);
	}

	// Output - pulses
	outputs[GATE_OUTPUT].setVoltage((triggerOutputPulse.process(args.sampleTime) ? 10.f : 0.f) > inputs[GATE_INPUT].getVoltage() ? 10.f : inputs[GATE_INPUT].getVoltage());
	outputs[RESET_OUTPUT].setVoltage(inputs[RESET_INPUT].getVoltage());
	outputs[EOC_OUTPUT].setVoltage(eocOutputPulse.process(args.sampleTime) ? 10.f : 0.f);
	outputs[CLOCK_OUTPUT].setVoltage(clockOutputPulse.process(args.sampleTime) ? 10.f : 0.f);

}

void SixtyFourGatePitchSeq::processFifty(const ProcessArgs& args){
	// Buttons


	// [MODE 1] - Record mode button pressed:
	if(params[RECORD_MODE_PARAM].getValue() > 0.f) {
			isRecording = !isRecording;
			gateModeSelected = true;
			setModeBrightnesses();
			params[GATE_MODE_PARAM].setValue(true);
			params[RECORD_MODE_PARAM].setValue(false);
	}

	// [MODE 2] - Edit mode button pressed:
	if(params[EDIT_MODE_PARAM].getValue() > 0.f) {
		for (int i = 0; i < 64; i++) {
			editModeSelectedGates[i] = false;
		}
		isRecording = false;
		gateModeSelected = false;
		setModeBrightnesses();
		params[EDIT_MODE_PARAM].setValue(false);
	}

	// [MODE 3] - Gate mode button pressed:
	if(params[GATE_MODE_PARAM].getValue() > 0.f){
		gateModeSelected = true;
		setModeBrightnesses();
		params[GATE_MODE_PARAM].setValue(false);
	}

	// Iterate main button matrix to check for pressed buttons
	bool anyThisSet = false;
	for (int i = 0; i < 64; i++) {
		if(params[BUTTON_MATRIX_PARAMS + i].getValue() > 0.f){
			anyThisSet = true;
			wasButtonPressedThisSample = true;
			// Gate Mode
			if(gateModeSelected){
				params[GATE_PARAMS + i].setValue(params[GATE_PARAMS + i].getValue() > 0 ? 0.f : 10.f);
				clearNoteParams(i);
			}
			// Edit Mode
			else{
				if(!anyPressed){
					editModeSelectedGates[i] = !editModeSelectedGates[i];
					anyPressed = true;
				}
			}
		}
	}
	if(!anyThisSet){
		anyPressed = false;
	}


	//Display - update when a button is clicked or during a clock step
	if(shouldRefreshDisplay || wasButtonPressedThisSample){
		updateDisplay();
	}

	//Audition
	if(inputs[GATE_INPUT].getVoltage() > 0.f){
		outputs[GATE_OUTPUT].setVoltage(inputs[GATE_INPUT].getVoltage());
		outputs[VOCT_OUTPUT].setVoltage(inputs[VOCT_INPUT].getVoltage());
		outputs[VELOCITY_OUTPUT].setVoltage(inputs[VELOCITY_INPUT].getVoltage());
	}
}

void SixtyFourGatePitchSeq::setModeBrightnesses() {
	// Set Mode Brightnesses
	lights[RECORD_MODE_LIGHT].setBrightness(isRecording);
	lights[EDIT_MODE_LIGHT].setBrightness(!gateModeSelected);
	lights[GATE_MODE_LIGHT].setBrightness(gateModeSelected);
}

void SixtyFourGatePitchSeq::clearNoteParams(int i) {
	params[HAS_RECORDED_PARAMS + i].setValue(0);
	params[BUTTON_MATRIX_PARAMS + i].setValue(0);
	params[VELOCITY_PARAMS + i].setValue(0);
	params[VOCT_PARAMS + i].setValue(0);
}

void SixtyFourGatePitchSeq::updateDisplay(){
	float red = 0;
	float green = 0;
	float prevMinVoct = minVoct;
	float prevMaxVoct = maxVoct;
	minVoct = 10.f;
	maxVoct = -10.f;
	float prevMinVel = minVel;
	float prevMaxVel = maxVel;
	minVel = 10.f;
	maxVel = -10.f;

	for (int i = 0; i < 64; i++) {
		//min max
		const float voct = params[VOCT_PARAMS + i].getValue();
		const float vel = params[VELOCITY_PARAMS + i].getValue();
		const float gateVal = params[GATE_PARAMS + i].getValue() > 0.f ? 10.f : 0.f;
		const int displayMode = params[DISPLAY_MODE_PARAM].getValue();
		const bool written = params[HAS_RECORDED_PARAMS + i].getValue();
		//write min maxes
		if(written && i < (params[STEP_COUNT_PARAM].getValue())){
			minVoct = voct < minVoct ? voct : minVoct;
			maxVoct = voct > maxVoct ? voct : maxVoct;
			minVel = vel < minVel ? vel : minVel;
			maxVel = vel > maxVel ? vel : maxVel;
		}
		//Display Mode Brightness
		if(i <= (params[STEP_COUNT_PARAM].getValue() - 1)) {
			switch(displayMode){
				//[1] - Gate display
				case 1:
					lights[STEP_LIGHTS + (i * 3) + 0].setBrightness(0.f);
					lights[STEP_LIGHTS + (i * 3) + 1].setBrightness(0.f);
					//if iterating through the current step, divide the gate value by 10 (1.f fully lit) else divide by 12 = .83f (83% brightness)
					lights[STEP_LIGHTS + (i * 3) + 2].setBrightness(gateVal / (i == currentStep ? 10.f : 12.f));
					break;

				//[2] - Voct display
				case 2:
					//normalize colors unless min and max are the same, then it should just be green :3
					red = (prevMaxVoct == prevMinVoct) ? 0.f : 1 - ((voct - prevMinVoct) / (prevMaxVoct - prevMinVoct));
					green = (prevMaxVoct == prevMinVoct) ? 1.f : (voct - prevMinVoct) / (prevMaxVoct - prevMinVoct);
					
					lights[STEP_LIGHTS + (i * 3) + 0].setBrightness(written ? red : 0);
					lights[STEP_LIGHTS + (i * 3) + 1].setBrightness(written ? green: 0);
					//blue should be blank unless no voct then we do dull blue (10%) to show that theres still a gate there
					lights[STEP_LIGHTS + (i * 3) + 2].setBrightness(written ? 0 : gateVal / 100.f);
					break;

				//[3] - Velocity display
				case 3:
					//normalize velocity unless min and max are the same, then it should just be bright green :3
					red = (prevMaxVel == prevMinVel) ? 1.f : ((vel - prevMinVel) / (prevMaxVel - prevMinVel));
					//minimum 0.5 on vel
					red = red < .05f ? .05f : red;
					lights[STEP_LIGHTS + (i * 3) + 0].setBrightness(written ? red : 0);
					lights[STEP_LIGHTS + (i * 3) + 1].setBrightness(0.f);
					//blue should be blank unless no voct then we do dull blue (10%) to show that theres still a gate there
					lights[STEP_LIGHTS + (i * 3) + 2].setBrightness(written ? 0 : gateVal / 100.f);

					break;
				default:
					break;
			}
		}else{
			//if over step count, dont display
			lights[STEP_LIGHTS + (i * 3) + 0].setBrightness(0.f);
			lights[STEP_LIGHTS + (i * 3) + 1].setBrightness(0.f);
			lights[STEP_LIGHTS + (i * 3) + 2].setBrightness(0.f);
		}
		
		if(!gateModeSelected){
			//if we're in edit mode, light up selected gates
			if(editModeSelectedGates[i]){
				lights[STEP_LIGHTS + (i * 3) + 0].setBrightness(10.f);
				lights[STEP_LIGHTS + (i * 3) + 1].setBrightness(10.f);
				lights[STEP_LIGHTS + (i * 3) + 2].setBrightness(10.f);
			}
		}

		//Active step light
		lights[ACTIVE_STEP_LIGHTS + i].setBrightness(i == currentStep ? 0.8f : 0.f);
	}
	//reset vars so we dont iterate every sample
	wasButtonPressedThisSample = false;
	shouldRefreshDisplay = false;
}

SixtyFourGatePitchSeqWidget::SixtyFourGatePitchSeqWidget(SixtyFourGatePitchSeq* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/n_SixtyFourGatePitchSeq.svg")));

		// screws
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		//[0 - 63] - Matrix buttons / lights
		const float spacing = 14.77f;
		const float xStartDistance = 22.39f;
		const float yStartDistance = 13.0f;
		//Iterate Column
		for (int iy = 0; iy < 8; iy++) {
			//Iterate Row
			for (int ix = 0; ix < 8; ix++) {
				int iterator = iy * 8 + ix;
				addChild(createLightCentered<LargerButtonLight<RoundRectLight<GreenLight>>>(mm2px(Vec(xStartDistance + (ix * spacing), yStartDistance + (iy * spacing))), module, SixtyFourGatePitchSeq::ACTIVE_STEP_LIGHTS + iterator));
				addParam(createLightParamCentered<LightButton<RubberRectButtonLarge, LargerButtonCenterLight<RoundRectLight<RedGreenBlueLight>>>>(mm2px(Vec(xStartDistance + (ix * spacing), yStartDistance + (iy * spacing))), module, SixtyFourGatePitchSeq::BUTTON_MATRIX_PARAMS + iterator, SixtyFourGatePitchSeq::STEP_LIGHTS + iterator * 3));
			}
		}

		float PortY[8] = {0};
		const float inputPortSpacing = 14.77f;
		const float inputPortStartHeight = 13.0f;
		for (int i = 0; i < 8; i++) {
			PortY[i] = inputPortStartHeight + inputPortSpacing * i;
		}
		const float inputPortXPos = 7.62f;
		// Inputs
		//[0] - Clock
		addInput(createInputCentered<DarkPJ301MPort>(mm2px(Vec(inputPortXPos, PortY[0])), module, SixtyFourGatePitchSeq::CLOCK_INPUT));
		//[1] - Reset
		addInput(createInputCentered<DarkPJ301MPort>(mm2px(Vec(inputPortXPos, PortY[1])), module, SixtyFourGatePitchSeq::RESET_INPUT));
		//[2] - V/Oct
		addInput(createInputCentered<DarkPJ301MPort>(mm2px(Vec(inputPortXPos, PortY[5])), module, SixtyFourGatePitchSeq::VOCT_INPUT));
		//[3] - Gate
		addInput(createInputCentered<DarkPJ301MPort>(mm2px(Vec(inputPortXPos, PortY[6])), module, SixtyFourGatePitchSeq::GATE_INPUT));
		//[4] - Velocity
		addInput(createInputCentered<DarkPJ301MPort>(mm2px(Vec(inputPortXPos, PortY[7])), module, SixtyFourGatePitchSeq::VELOCITY_INPUT));

		// Buttons
		//[0] - Record mode
		addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<RedLight>>>(mm2px(Vec(inputPortXPos, PortY[2])), module, SixtyFourGatePitchSeq::RECORD_MODE_PARAM, SixtyFourGatePitchSeq::RECORD_MODE_LIGHT));
		//[1] - Edit mode
		addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(inputPortXPos, PortY[3])), module, SixtyFourGatePitchSeq::EDIT_MODE_PARAM, SixtyFourGatePitchSeq::EDIT_MODE_LIGHT));
		//[2] - Gate mode
		addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(inputPortXPos, PortY[4])), module, SixtyFourGatePitchSeq::GATE_MODE_PARAM, SixtyFourGatePitchSeq::GATE_MODE_LIGHT));

		const float outputPortXPos = 139.7f;
		// outputs
		//[0] - Clock
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(outputPortXPos, PortY[0])), module, SixtyFourGatePitchSeq::CLOCK_OUTPUT));
		//[1] - Reset
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(outputPortXPos, PortY[1])), module, SixtyFourGatePitchSeq::RESET_OUTPUT));
		//[2] - Eoc
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(outputPortXPos, PortY[2])), module, SixtyFourGatePitchSeq::EOC_OUTPUT));
		//[3] - V/Oct
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(outputPortXPos, PortY[5])), module, SixtyFourGatePitchSeq::VOCT_OUTPUT));
		//[4] - Gate
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(outputPortXPos, PortY[6])), module, SixtyFourGatePitchSeq::GATE_OUTPUT));
		//[5] - Velocity
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(outputPortXPos, PortY[7])), module, SixtyFourGatePitchSeq::VELOCITY_OUTPUT));

		// Knobs
		//[0] - Step count
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(outputPortXPos, PortY[3])), module, SixtyFourGatePitchSeq::STEP_COUNT_PARAM));
		//[1] - Display mode
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(outputPortXPos, PortY[4])), module, SixtyFourGatePitchSeq::DISPLAY_MODE_PARAM));
	}