#include "KWAModules.hpp"


struct SixtyFourGatePitchSeqExpander : Module {
	enum ParamIds {
		PLAY_PARAM,
		PLAYHEAD_KNOB_PARAM,
		PAGE_KNOB_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PLAY_CV_INPUT,
		PLAYHEAD_CV_INPUT,
		PAGE_CV_INPUT,
		DATA_ONE_INPUT,
		DATA_TWO_INPUT,
		DATA_THREE_INPUT,
		DATA_FOUR_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		DATA_ONE_OUTPUT,
		DATA_TWO_OUTPUT,
		DATA_THREE_OUTPUT,
		DATA_FOUR_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
	// Expander Message Arrays
	float leftMessages[2][8] = {};
	float rightMessages[2][8] = {};

	SixtyFourGatePitchSeqExpander() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		//knobs
		configSwitch(PLAYHEAD_KNOB_PARAM, 1.f, 3.f, 2.f, "Display Mode",{"Left/Right", "Up/Down", "Random", "Snake"});
		getParamQuantity(PLAYHEAD_KNOB_PARAM)->snapEnabled = true;
		configParam(PAGE_KNOB_PARAM, 1.f, 10.f, 1.f, "Page");
		getParamQuantity(PAGE_KNOB_PARAM)->snapEnabled = true;

		//inputs
		configInput(PLAY_CV_INPUT, "Play");
		configInput(PLAYHEAD_CV_INPUT, "Playhead Mode");
		configInput(PAGE_CV_INPUT, "Page");
		configInput(DATA_ONE_INPUT, "Data 1");
		configInput(DATA_TWO_INPUT, "Data 2");
		configInput(DATA_THREE_INPUT, "Data 3");
		configInput(DATA_FOUR_INPUT, "Data 4");
		
		//outputs
		configOutput(DATA_ONE_OUTPUT, "Data 1");
		configOutput(DATA_TWO_OUTPUT, "Data 2");
		configOutput(DATA_THREE_OUTPUT, "Data 3");
		configOutput(DATA_FOUR_OUTPUT, "Data 4");

		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
	}

	void process(const ProcessArgs& args) override {
		const bool is_baby = leftExpander.module && (leftExpander.module->model == modelEightGateSequencer
			|| leftExpander.module->model == modelEightGateSequencerChild);
		// Read left expander messages
		if (is_baby) {
			float* leftMessage = (float*)leftExpander.consumerMessage;
			//[0] Clock signal
			if(leftMessage[0] > 0.0f) {
			}
			//[1] Reset signal
			if (leftMessage[1] > 0.0f) {
			}
			//[2] Continuous Mode Signal
			if (leftMessage[2] > 0.0f) {
			}
			//[3] EOC Signal
			if (leftMessage[3] > 0.0f) {
			}
			//[4] Global Trigger Signal
			if (leftMessage[4] > 0.0f) {
			}
		}

		// Send left
		if (is_baby) {
			float* leftSendMessage = (float*)leftExpander.module->rightExpander.producerMessage;
			//leftSendMessage[0] = (eocOutputPulse.process(args.sampleTime) ? 10.f : 0.f);

			// Flip messages at the end of the timestep
			leftExpander.module->rightExpander.messageFlipRequested = true;
		}

		//[0] DATA 1
		outputs[DATA_ONE_OUTPUT].setVoltage(0.f);

		//[1] DATA 2
		outputs[DATA_TWO_OUTPUT].setVoltage(0.f);

		//[2] DATA 3
		outputs[DATA_THREE_OUTPUT].setVoltage(0.f);

		//[3] DATA 4
		outputs[DATA_FOUR_OUTPUT].setVoltage(0.f);
	}
};


struct SixtyFourGatePitchSeqExpanderWidget : ModuleWidget {
	SixtyFourGatePitchSeqExpanderWidget(SixtyFourGatePitchSeqExpander* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/n_SixtyFourGatePitchSeqExpander.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		float arr[5] = {0};
		for (int i = 0; i < 5; i++) {
			arr[i] = 117 - i * 16.251;
		}
		//[0] - DATA 1
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(18.24, arr[0])), module, SixtyFourGatePitchSeqExpander::DATA_ONE_OUTPUT));
		//[1] - DATA 2
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(18.24, arr[1])), module, SixtyFourGatePitchSeqExpander::DATA_TWO_OUTPUT));
		//[2] - DATA 3
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(18.24, arr[2])), module, SixtyFourGatePitchSeqExpander::DATA_THREE_OUTPUT));
		//[3] - DATA 4
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(18.24, arr[3])), module, SixtyFourGatePitchSeqExpander::DATA_FOUR_OUTPUT));
	}
};


Model* modelSixtyFourGatePitchSeqExpander = createModel<SixtyFourGatePitchSeqExpander, SixtyFourGatePitchSeqExpanderWidget>("SixtyFourGatePitchSeqExpander");
