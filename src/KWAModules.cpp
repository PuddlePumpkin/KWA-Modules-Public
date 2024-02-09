#include "KWAModules.hpp"

Plugin *pluginInstance;


void init(Plugin *p) {
	pluginInstance = p;
	p->addModel(modelSixtyFourGatePitchSeq);
	p->addModel(modelEightGateSequencer);
	p->addModel(modelEightGateSequencerChild);
	p->addModel(modelSequencerController);
	p->addModel(modelSequencerEnd);
}
