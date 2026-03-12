#ifndef __IMAGELOADER_H_
#define __IMAGELOADER_H_

#include <glad/glad.h>
#include <string>
using namespace std;

namespace sgraph
{
    /**
     * @brief This abstract class represents an image loader. There would be 
     * one subclass for each file format
     * 
     */
    class ImageLoader {
        public:
            ImageLoader() {}
            inline GLubyte* getPixels() {return image;}
            inline float* getFPixels() { return floatImage;}
            inline int getWidth() {return width;}
            inline int getHeight() {return height;}        
            virtual void load(string filename)=0;
    
        protected:
            GLubyte *image;
            float* floatImage;
            int width;
            int height;
    
    };
}

#endif