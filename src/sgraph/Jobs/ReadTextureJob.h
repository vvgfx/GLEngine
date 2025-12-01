#ifndef _READTEXTUREJOB_H_
#define _READTEXTUREJOB_H_

#include <string>
#include "../../Model.h"
#include "../../Tasks/TransferTextureTask.h"
#include "../PPMImageLoader.h"
#include "../STBImageLoader.h"
#include "IJob.h"
using namespace std;

namespace job
{

    /**
     * This is an implementation of the IJob interface.
     * It uses the command design pattern to read a new texture from disk.
     *
     * Note: This is a part of the controller.
     */
    class ReadTextureJob : public IJob
    {
    public:
        ReadTextureJob(string texName, string texPath)
        {
            this->textureName = texName;
            this->texturePath = texPath;
        }
        ~ReadTextureJob()
        {
        }

        virtual void execute(Model *m)
        {
            cout<<"About to load textures!"<<endl;
            sgraph::ImageLoader* textureLoader;
            if(texturePath.find(".ppm") != string::npos)
                textureLoader = new sgraph::PPMImageLoader();
            else
                textureLoader = new sgraph::STBImageLoader();
            textureLoader->load(texturePath);
            util::TextureImage* texImage = new util::TextureImage(textureLoader->getPixels(), textureLoader->getWidth(), textureLoader->getHeight(), textureName);
            cout<<"Loaded textures!"<<endl;
            
            // now create the task and add it to the task queue
            // worried about object lifespan. the texImage should survive, but what about the 
            task::TransferTextureTask* memTransferTask = new task::TransferTextureTask(m, textureName, texturePath, texImage);
            m->addToTaskQueue(memTransferTask);
        }

    private:
        string textureName, texturePath;
    };
}

#endif