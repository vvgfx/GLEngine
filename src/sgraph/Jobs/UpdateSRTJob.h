#ifndef _UPDATESRTJOB_H_
#define _UPDATESRTJOB_H_

#include <string>
#include "../../Model.h"
#include "../Commands/UpdateSRTCommand.h"
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
    class UpdateSRTJob : public IJob
    {
    public:
        /**
         * @brief Construct a new UpdateSRTCommand object
         *
         * @param name Name of the node this command should effect
         * @param sx scale in x-axis
         * @param sy scale in y-axis
         * @param sz scale in z-axis
         * @param rx rotation in x-axis (degrees)
         * @param ry rotation in y-axis (degrees)
         * @param rz rotation in z-axis (degrees)
         * @param tx translation in x-axis
         * @param ty translation in y-axis
         * @param tz translation in z-axis
         */
        UpdateSRTJob(string nodeName, float sx, float sy, float sz, float rx, float ry, float rz, float tx, float ty, float tz)
        {
            this->nodeName = nodeName;
            this->sx = sx;
            this->sy = sy;
            this->sz = sz;
            this->rx = glm::radians(rx);
            this->ry = glm::radians(ry);
            this->rz = glm::radians(rz);
            this->tx = tx;
            this->ty = ty;
            this->tz = tz;
        }

        virtual void execute(Model *m)
        {
            command::UpdateSRTCommand* srtCommand = new command::UpdateSRTCommand(nodeName, sx, sy, sz, rx, ry, rz, tx, ty, tz);
            cout<<"Adding to command queue in job"<<endl;
            m->addToCommandQueue(srtCommand);
        }

    private:
        float rx, ry, rz;
        float sx, sy, sz;
        float tx, ty, tz;
    };
}

#endif