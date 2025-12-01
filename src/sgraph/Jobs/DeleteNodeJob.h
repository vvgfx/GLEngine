#ifndef _DELETENODEJOB_H_
#define _DELETENODEJOB_H_

#include <string>
#include "../../Model.h"
#include "../Commands/DeleteNodeCommand.h"
#include "IJob.h"
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to delete a child node from a parent.
     *
     * Note: This is a part of the controller.
     * @param nodeName name of the node you want to delete
     * @param parentName name of the parent node that you want to delete
     */
    class DeleteNodeJob : public IJob
    {
    public:
        DeleteNodeJob(string nodeName, string parentName)
        {
            this->nodeName = nodeName;
            this->parentName = parentName;
        }

        virtual void execute(Model *m)
        {
            command::DeleteNodeCommand* deleteCommand = new command::DeleteNodeCommand(parentName, nodeName, m->getScenegraph());
            cout<<"Adding to command queue in job"<<endl;
            m->addToCommandQueue(deleteCommand);
        }

    private:
        string parentName;
    };
}

#endif