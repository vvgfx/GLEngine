#ifndef _TEXTUREIMAGE_H_
#define _TEXTUREIMAGE_H_

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

using namespace std;


namespace util
{	
  /**
         A class that represents an image. Provides a function for bilinear interpolation
         */
  class TextureImage
  {
  protected:
    GLubyte *image;
    float* floatImage;
    int width,height;
    string name;
    bool isFloat;
  protected:
  void deleteImage()
  {

    if (image)
      delete []image;
  }

  public:
	  TextureImage() {
      image = NULL;
      width = 0;
      height = 0;
	  }

    TextureImage(GLubyte* image,int width,int height,string name) {
      this->image = image;
      this->width = width;
      this->height = height;
      isFloat = false;
    }

    /**
     * Adding a second constructor to get textures via float*
     * This is useful for hdr images.
     */
    TextureImage(float* image,int width,int height,string name) {
      this->floatImage = image;
      this->width = width;
      this->height = height;
      isFloat = true;

    }

    bool getIsFloat()  {
      return isFloat;
    }


    ~TextureImage() {
      deleteImage();
    }

    GLubyte *getImage() {
      return image;
    }

    float* getFloatImage()  {
      return floatImage;
    }

    int getWidth() {
      return width;
    }

    int getHeight() {
      return height;
    }

    string getName() {
      return name;
    }

    glm::vec4 getColor(float x,float y) {
      int x1,y1,x2,y2;

      x = x - (int)x; //GL_REPEAT
      y = y - (int)y; //GL_REPEAT

      x1 = (int)(x*width);
      y1 = (int)(y*height);

      x1 = (x1 + width)%width;
      y1 = (y1 + height)%height;

      x2 = x1+1;
      y2 = y1+1;

      if (x2>=width)
        x2 = width-1;

      if (y2>=height)
        y2 = height-1;

      glm::vec3 one = getColor(x1,y1);
      glm::vec3 two =getColor(x2,y1);
      glm::vec3 three = getColor(x1,y2);
      glm::vec3 four = getColor(x2,y2);

      glm::vec3 inter1 = glm::mix(one,three,y-(int)y);
      glm::vec3 inter2 = glm::mix(two,four,y-(int)y);
      glm::vec3 inter3 = glm::mix(inter1,inter2,x-(int)x);

      return glm::vec4(inter3,1);
    }

  private:
    glm::vec3 getColor(int x,int y) {
      if(isFloat)
        return glm::vec3((float)floatImage[3*(y*width+x)],(float)floatImage[3*(y*width+x)+1],(float)floatImage[3*(y*width+x)+2]);
      return glm::vec3((float)image[3*(y*width+x)],(float)image[3*(y*width+x)+1],(float)image[3*(y*width+x)+2]);
    }




  };
}

#endif
