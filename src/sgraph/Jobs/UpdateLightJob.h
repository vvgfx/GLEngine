#ifndef _UPDATELIGHTJOB_H_
#define _UPDATELIGHTJOB_H_

#include <string>
#include "../../Model.h"
#include "../Commands/UpdateLightCommand.h"
#include "IJob.h"
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to update the light of a node.
     *
     * Note: This is a part of the controller.
     */
    class UpdateLightJob : public IJob
    {
    public:
        UpdateLightJob(string nodeName, string lightName, float colorVal[3], float spotDirVal[3], float posVal[3], float angleVal)
        {
            this->nodeName = nodeName;
            this->lightName = lightName;
            copy(colorVal, colorVal + 3, this->colorVal);
            copy(spotDirVal, spotDirVal + 3, this->spotDirVal);
            copy(posVal, posVal + 3, this->posVal);
            this->angleVal = angleVal;
        }

        virtual void execute(Model *m)
        {
            command::UpdateLightCommand* updateLightCommand = new command::UpdateLightCommand(nodeName, lightName, colorVal, spotDirVal, posVal, angleVal);
            cout<<"Adding light update to command queue in job"<<endl;
            m->addToCommandQueue(updateLightCommand);
        }

    private:
        float angleVal;
        float colorVal[3], spotDirVal[3], posVal[3];
        string lightName;


    };
}

#endif