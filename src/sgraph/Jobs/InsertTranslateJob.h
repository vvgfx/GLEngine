#ifndef _INSERTTRANSLATEJOB_H_
#define _INSERTTRANSLATEJOB_H_

#include <string>
#include "../../Model.h"
#include "../Commands/InsertTranslateCommand.h"
#include "IJob.h"
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to insert a translate node under the given node.
     *
     * Note: This is a part of the controller.
     */
    class InsertTranslateJob : public IJob
    {
    public:
        InsertTranslateJob(string nodeName, string newNodeName, float tx, float ty, float tz)
        {
            this->nodeName = nodeName;
            this->newNodeName = newNodeName;
            this->tx = tx;
            this->ty = ty;
            this->tz = tz;
        }

        virtual void execute(Model *m)
        {
            command::InsertTranslateCommand* translateCommand = new command::InsertTranslateCommand(nodeName, newNodeName, tx, ty, tz, m->getScenegraph());
            cout<<"Adding to command queue in job"<<endl;
            m->addToCommandQueue(translateCommand);
        }

    private:
        float tx, ty, tz;
        string newNodeName;
    };
}

#endif