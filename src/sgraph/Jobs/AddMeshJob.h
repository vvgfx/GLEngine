#ifndef _ADDMESHJOB_H_
#define _ADDMESHJOB_H_

#include <string>
#include "../../Model.h"
#include "../../Tasks/AddMeshTask.h"
#include "IJob.h"
#include "../../GUICallbacks.h"
#include <fstream>
#include <ObjAdjImporter.h>
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to read a new texture from disk.
     *
     * Note: This is a part of the controller.
     */
    class AddMeshJob : public IJob
    {
    public:
        AddMeshJob(string meshName, string meshPath, GUICallbacks* callbacks)
        {
            this->meshName = meshName;
            this->meshPath = meshPath;
            this->callbacks = callbacks;
        }

        virtual void execute(Model *m)
        {
            cout<<"About to load a model!"<<endl;
            ifstream in(meshPath);
            if (in.is_open()) 
            {
                // happy to do this in parallel :)
                mesh = util::ObjAdjImporter<VertexAttrib>::importFile(in,false);
                cout<<"Loaded model from disk!"<<endl;

                // now create the task and add it to the task queue to add to the scenegraph
                task::AddMeshTask* addMeshTask = new task::AddMeshTask(m, meshName, meshPath, callbacks, mesh);
                m->addToTaskQueue(addMeshTask);
            }
        }

    private:
        util::PolygonMesh<VertexAttrib> mesh;
        string meshName, meshPath;
        GUICallbacks* callbacks;        
    };
}

#endif