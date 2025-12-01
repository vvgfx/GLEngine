#ifndef _INSERTSRTJOB_H_
#define _INSERTSRTJOB_H_

#include <string>
#include "../../Model.h"
#include "../Commands/InsertSRTCommand.h"
#include "IJob.h"
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to insert a SRT node under the given node.
     *
     * Note: This is a part of the controller.
     */
    class InsertSRTJob : public IJob
    {
    public:
        /**
         * @brief Construct a new InsertSRTJob object
         *
         * @param name Name of the node this command should effect
         * @param newNodeName Name of the new node.
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
        InsertSRTJob(string nodeName, string newNodeName,float sx, float sy, float sz, float rx, float ry, float rz, float tx, float ty, float tz)
        {
            this->nodeName = nodeName;
            this->newNodeName = newNodeName;
            this->sx = sx;
            this->sy = sy;
            this->sz = sz;

            // converting to radians here so that the command and insertion don't have to.
            this->rx = glm::radians(rx);
            this->ry = glm::radians(ry);
            this->rz = glm::radians(rz);


            this->tx = tx;
            this->ty = ty;
            this->tz = tz;
        }

        virtual void execute(Model *m)
        {
            command::InsertSRTCommand* srtCommand = new command::InsertSRTCommand(nodeName, newNodeName, sx, sy, sz, rx, ry, rz, tx, ty, tz, m->getScenegraph());
            cout<<"Adding SRT to command queue in job"<<endl;
            m->addToCommandQueue(srtCommand);
        }

    private:
        float rx, ry, rz;
        float sx, sy, sz;
        float tx, ty, tz;
        string newNodeName;
    };
}

#endif