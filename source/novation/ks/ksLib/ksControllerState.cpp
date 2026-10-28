#include "ksControllerState.h"
namespace ks {
namespace {ControllerReferenceFactory referenceFactory=nullptr;}
void setControllerReferenceFactoryForTests(ControllerReferenceFactory f){referenceFactory=f;}
bool controllerReferenceAvailable(){return referenceFactory!=nullptr;}
uint8_t ControllerReference::mappedRead(ControllerCpu& h,uint32_t a){return h.bus?h.bus->read(a):h.memory[a];}
void ControllerReference::mappedWrite(ControllerCpu& h,uint32_t a,uint8_t v){if(h.bus)h.bus->write(a,v);else h.memory[a]=v;}
ControllerCpu::ControllerCpu()=default;
ControllerCpu::~ControllerCpu()=default;
void ControllerCpu::memmap(ControllerBus* device,int start,int length){
    if(start!=0 || length!=(1<<24))throw std::invalid_argument("KS requires one explicit full-board bus");
    bus=device;if(bus)bus->setState(this);
}
void ControllerCpu::step(){
    if(!interpreterAllowed)throw std::logic_error("H8 interpreter called after native KS handoff");
    if(!reference){if(!referenceFactory)throw std::logic_error("KS H8 is available only in comparison tests");reference=referenceFactory();}
    ++interpretedSteps;reference->step(*this);
}
}
