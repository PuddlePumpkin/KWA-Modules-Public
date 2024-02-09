#include "KWAModules.hpp"


struct SequencerEnd : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		EOC_OUTPUT,
		CONTINUOUSMODE_OUTPUT,
		CLOCK_OUTPUT,
		RESET_OUTPUT,
		GLOBAL_TRIG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
	dsp::PulseGenerator clockOutputPulse;
	//Continous Mode doesnt pulse;
	bool ContinousModeOutput;
	dsp::PulseGenerator resetOutputPulse;
	dsp::PulseGenerator eocOutputPulse;
	dsp::PulseGenerator globalTriggerOutputPulse;

	float leftMessages[2][8] = {};
	float rightMessages[2][8] = {};

	SequencerEnd() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configOutput(EOC_OUTPUT, "EOC");
		configOutput(CONTINUOUSMODE_OUTPUT, "Continuous mode on");
		configOutput(CLOCK_OUTPUT, "Clock");
		configOutput(RESET_OUTPUT, "EOC and reset");
		configOutput(GLOBAL_TRIG_OUTPUT, "Any stepped gate");

		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
	}

	void process(const ProcessArgs& args) override {
		const bool is_baby = leftExpander.module && (leftExpander.module->model == modelEightGateSequencer
			|| leftExpander.module->model == modelEightGateSequencerChild);
		// Read left
		if (is_baby) {
			float* leftMessage = (float*)leftExpander.consumerMessage;
			//[0] Clock signal
			if(leftMessage[0] > 0.0f) {
				clockOutputPulse.trigger(1e-3f);
			}
			//[1] Reset signal
			if (leftMessage[1] > 0.0f) {
				resetOutputPulse.trigger(1e-3f);
			}
			//[2] Continuous Mode Signal
			if (leftMessage[2] > 0.0f) {
				ContinousModeOutput = true;
			}
			//[3] EOC Signal
			if (leftMessage[3] > 0.0f) {
				eocOutputPulse.trigger(1e-3f);
			}
			//[4] Global Trigger Signal
			if (leftMessage[4] > 0.0f) {
				globalTriggerOutputPulse.trigger(1e-3f);
			}
		}

		// Send left
		if (is_baby) {
			float* leftSendMessage = (float*)leftExpander.module->rightExpander.producerMessage;
			leftSendMessage[0] = (eocOutputPulse.process(args.sampleTime) ? 10.f : 0.f);

			// Flip messages at the end of the timestep
			leftExpander.module->rightExpander.messageFlipRequested = true;
		}

		//[0] EOC Output
		outputs[EOC_OUTPUT].setVoltage(eocOutputPulse.process(args.sampleTime) ? 10.f : 0.f);

		//[1] CONTINUOUS MODE Output
		outputs[CONTINUOUSMODE_OUTPUT].setVoltage(ContinousModeOutput ? 10.f : 0.f);

		//[2] CLOCK Output
		outputs[CLOCK_OUTPUT].setVoltage(clockOutputPulse.process(args.sampleTime) ? 10.f : 0.f);

		//[3] RESET Output
		outputs[RESET_OUTPUT].setVoltage(resetOutputPulse.process(args.sampleTime) ? 10.f : 0.f);

		//[4] GLOBAL_TRIG Output
		outputs[GLOBAL_TRIG_OUTPUT].setVoltage(globalTriggerOutputPulse.process(args.sampleTime) ? 10.f : 0.f);

		//Reinit bools
		ContinousModeOutput = false;
	}
};


struct SequencerEndWidget : ModuleWidget {
	SequencerEndWidget(SequencerEnd* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/n_SequencerEnd.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		float arr[5] = { 0,0,0,0,0 };
		for (int i = 0; i < 5; i++) {
			arr[i] = 117 - i * 16.251;
		}
		//[0] - Global Trigger
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(5.08, arr[0])), module, SequencerEnd::GLOBAL_TRIG_OUTPUT));
		//[1] - Continuous Mode
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(5.08, arr[1])), module, SequencerEnd::CONTINUOUSMODE_OUTPUT));
		//[2] - Clock
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(5.08, arr[2])), module, SequencerEnd::CLOCK_OUTPUT));
		//[3] - Reset
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(5.08, arr[3])), module, SequencerEnd::RESET_OUTPUT));
		//[4] - End of cycle
		addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(5.08, arr[4])), module, SequencerEnd::EOC_OUTPUT));
	}
};


Model* modelSequencerEnd = createModel<SequencerEnd, SequencerEndWidget>("SequencerEnd");
