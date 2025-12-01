#ifndef _UPDATELEAFMATERIALJOB_H_
#define _UPDATELEAFMATERIALJOB_H_

#include <string>
#include "../../Model.h"
#include "../Commands/UpdateLeafMaterialCommand.h"
#include "IJob.h"
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to update the material of a leaf node.
     *
     * Note: This is a part of the controller.
     */
    class UpdateLeafMaterialJob : public IJob
    {
    public:
        UpdateLeafMaterialJob(string nodeName, glm::vec4 albedo, float metallic, float roughness, float ao)
        {
            this->nodeName = nodeName;
            this->albedo = albedo;
            this->metallic = metallic;
            this->roughness = roughness;
            this->ao = ao;
        }

        virtual void execute(Model *m)
        {
            command::UpdateLeafMaterialCommand* updateLeafMatJob = new command::UpdateLeafMaterialCommand(nodeName, albedo, metallic, roughness, ao);
            cout<<"Adding light update to command queue in job"<<endl;
            m->addToCommandQueue(updateLeafMatJob);
        }

    private:
        glm::vec4 albedo;
        float metallic, roughness, ao;


    };
}

#endif