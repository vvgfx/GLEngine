#ifndef _INSERTGROUPJOB_H_
#define _INSERTGROUPJOB_H_

#include <string>
#include "../../Model.h"
#include "../Commands/InsertGroupCommand.h"
#include "IJob.h"
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to insert a group node under the given node.
     *
     * Note: This is a part of the controller.
     */
    class InsertGroupJob : public IJob
    {
    public:
        InsertGroupJob(string nodeName, string newNodeName)
        {
            this->nodeName = nodeName;
            this->newNodeName = newNodeName;
        }

        virtual void execute(Model *m)
        {
            command::InsertGroupCommand* groupCommand = new command::InsertGroupCommand(nodeName, newNodeName, m->getScenegraph());
            cout<<"Adding to command queue in job"<<endl;
            m->addToCommandQueue(groupCommand);
        }

    private:
        string newNodeName;
    };
}

#endif