#ifndef _IJOB_H_
#define _IJOB_H_

#include <string>
#include "../../Model.h"
using namespace std;

namespace job {

/**
 * This is an interface for a Job on scene graph nodes.
 * It uses the command design pattern to execute heavy workloads on parallel-threads
 * 
 * Note: This is a part of the controller.
 */
    class IJob {
        public:
        virtual ~IJob() {}

        /**
         * This should do any sort of processing required in a parallel thread, and once done,
         * create a ICommand node and add it to the command-queue using m->addToCommandQueue()
         * Alternatively, if you do not work on a scenegraph node, you can create a task and add
         * it to the task queue through m->addToTaskQueue()
         */
        virtual void execute(Model* m)=0;

        protected:
        string nodeName;
    };
}

#endif