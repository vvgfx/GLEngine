#ifndef _DELETELIGHTJOB_H_
#define _DELETELIGHTJOB_H_

#include <string>
#include "../../Model.h"
#include "../Commands/DeleteLightCommand.h"
#include "IJob.h"
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to delete a light of a node
     *
     * Note: This is a part of the controller.
     */
    class DeleteLightJob : public IJob
    {
    public:
        DeleteLightJob(string nodeName, string lightName)
        {
            this->nodeName = nodeName;
            this->lightName = lightName;
        }

        virtual void execute(Model *m)
        {
            command::DeleteLightCommand* deleteLightCommand = new command::DeleteLightCommand(nodeName, lightName);
            cout<<"Adding light delete to command queue in job"<<endl;
            m->addToCommandQueue(deleteLightCommand);
        }

    private:
        string lightName;


    };
}

#endif