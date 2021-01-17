#include "transition.h"
#include "../util/serviceprovider.h"

void Transition::start()
{
    if(status != TransitionStatus::ComplOff ||
       status != TransitionStatus::ComplOn)
    {
        LOG(Severity::Warning, "Tried to start transition during other transition!");
    }

}
