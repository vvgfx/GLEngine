#ifndef _UPDATEROTATEJOB_H_
#define _UPDATEROTATEJOB_H_

#include <string>
#include "../../Model.h"
#include "../Commands/RotateCommand.h"
#include "IJob.h"
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to update the rotation of an object.
     *
     * Note: This is a part of the controller.
     */
    class UpdateRotateJob : public IJob
    {
    public:
        UpdateRotateJob(string nodeName, float rx, float ry, float rz, float rAngleDegrees)
        {
            this->nodeName = nodeName;
            this->rx = rx;
            this->ry = ry;
            this->rz = rz;
            this->ra = glm::radians(rAngleDegrees);
        }

        virtual void execute(Model *m)
        {
            command::RotateCommand* rotateCommand = new command::RotateCommand(nodeName, rx, ry, rz, ra);
            cout<<"Adding to command queue in job"<<endl;
            m->addToCommandQueue(rotateCommand);
        }

    private:
        float rx, ry, rz, ra;
    };
}

#endif