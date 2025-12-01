#ifndef _INSERTROTATEJOB_H_
#define _INSERTROTATEJOB_H_

#include <string>
#include "../../Model.h"
#include "../Commands/InsertRotateCommand.h"
#include "IJob.h"
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to insert a rotate node under the given node.
     *
     * Note: This is a part of the controller.
     */
    class InserteRotateJob : public IJob
    {
    public:
        InserteRotateJob(string nodeName, string newNodeName, float rx, float ry, float rz, float rAngleDegrees)
        {
            this->nodeName = nodeName;
            this->newNodeName = newNodeName;
            this->rx = rx;
            this->ry = ry;
            this->rz = rz;
            this->ra = glm::radians(rAngleDegrees);
        }

        virtual void execute(Model *m)
        {
            command::InsertRotateCommand* rotateCommand = new command::InsertRotateCommand(nodeName, newNodeName, rx, ry, rz, ra, m->getScenegraph());
            cout<<"Adding to command queue in job"<<endl;
            m->addToCommandQueue(rotateCommand);
        }

    private:
        float rx, ry, rz, ra;
        string newNodeName;
    };
}

#endif