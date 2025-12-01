#ifndef _INSERTLIGHTJOB_H_
#define _INSERTLIGHTJOB_H_

#include <string>
#include "../../Model.h"
#include "../Commands/InsertLightCommand.h"
#include "IJob.h"
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to insert a light into a node.
     *
     * Note: This is a part of the controller.
     */
    class InsertLightJob : public IJob
    {
    public:
        InsertLightJob(string nodeName, string lightName, glm::vec3 colorVal, glm::vec4 spotDirVal, glm::vec4 posVal, float angleVal)
        {
            this->nodeName = nodeName;
            this->lightName = lightName;
            this->color = colorVal;
            this->position = posVal;
            this->spotDirecction = spotDirVal;
            this->angleVal = angleVal;
        }

        virtual void execute(Model *m)
        {
            command::InsertLightCommand* insertLightCommand = new command::InsertLightCommand(nodeName, lightName, color, spotDirecction, position, angleVal);
            cout<<"Adding light insert to command queue in job"<<endl;
            m->addToCommandQueue(insertLightCommand);
        }

    private:
        float angleVal;
        glm::vec3 color;
        glm::vec4 position, spotDirecction;
        string lightName;


    };
}

#endif