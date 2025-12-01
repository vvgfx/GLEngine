#ifndef _UPDATETRANSLATEJOB_H_
#define _UPDATETRANSLATEJOB_H_

#include <string>
#include "../../Model.h"
#include "../Commands/TranslateCommand.h"
#include "IJob.h"
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to update the translation of an object.
     *
     * Note: This is a part of the controller.
     */
    class UpdateTranslateJob : public IJob
    {
    public:
        UpdateTranslateJob(string nodeName, float tx, float ty, float tz)
        {
            this->nodeName = nodeName;
            this->tx = tx;
            this->ty = ty;
            this->tz = tz;
        }

        virtual void execute(Model *m)
        {
            command::TranslateCommand* translateCommand = new command::TranslateCommand(nodeName, tx, ty, tz);
            cout<<"Adding to command queue in job"<<endl;
            m->addToCommandQueue(translateCommand);
        }

    private:
        float tx, ty, tz;
    };
}

#endif