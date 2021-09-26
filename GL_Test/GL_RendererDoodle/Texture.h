#pragma once

#include <algorithm>
#include <vector>

#include "Image.h"


struct id_code {
  unsigned int id;
  operator unsigned int() { return id; }

  static unsigned int last_id;

  id_code() {
    id = last_id;
    last_id++;
  }

  id_code(const id_code&& other) {
    id = other.id;
  }
  id_code& operator=(const id_code&& other) {
    id = other.id;
  }

  //todo: maybe disable copy constructor? (force manual duplicate call)
  id_code(const id_code& other) {
    id = id_code();
  }
  id_code& operator=(const id_code& other) {
    id = id_code();
  }

};

inline float fract(float f)
{
  return f - (long)f;
}

// collection of images ... with type information ?
class Texture {
  // num layers cannot be changed after creation
  unsigned int layers = 1;
public:
  id_code id;

  Texture(const Texture&) = default;
  Texture& operator=(const Texture&) = default;

  unsigned int levels = 0;

  // TODO: make a new layered image class instead of using std::array ... make easier api so I don't have to do xxx[0].width(), and can allocate the memory upfront, and contiguously
  // for now just a single array of layer * level images
  std::vector<Image> images;

  double operator() (unsigned int x, unsigned int y, int c, unsigned int level, unsigned int layer) {
    return images[level * layers + layer](x, y, c);
  }

  glm::dvec3& operator() (unsigned int x, unsigned int y, unsigned int level, unsigned int layer) {
    return (glm::dvec3&)images[level * layers + layer](x, y, 0);
  }

  glm::dvec3& operator() (glm::vec3 d, unsigned int level);

  void generateMipMaps() {
    while (images.back().width() > 1 && images.back().height() > 1) {
      size_t  width = images.back().width() / 2;
      size_t height = images.back().height() / 2;
      levels++;

      for (int layer = 0; layer < layers; layer++) {
        images.emplace_back(width, height);

        Image& upperLevel = images[(levels - 2) * layers + layer];
        Image& curLevel = images.back();
        curLevel = Image(width, height);

        double wRatio = (double)upperLevel.width() / curLevel.width();
        double hRatio = (double)upperLevel.height() / curLevel.height();

        // 1 approaches: over the mip than over the above
        for (int i = 0; i < curLevel.width(); i++)
        {
          for (int j = 0; j < curLevel.height(); j++)
          {
            double wLower = i * wRatio;
            double wUpper = (i + 1) * wRatio;

            double hLower = j * hRatio;
            double hUpper = (j + 1) * hRatio;

            long x = 0;
            long y = 0;

            glm::dvec4 pixelColour{ 0,0,0,0 };

            double pixelArea = (wUpper - wLower) * (hUpper - hLower);

            while (wLower < wUpper)
            {
              double wScale = 1.0;
              double wNext;


              if (fract(wLower))
              {
                wNext = std::ceil(wLower);
              }
              else
              {
                wNext = wLower + 1;
              }
              wNext = std::min(wNext, wUpper);

              wScale = wNext - wLower;
              x = (long)wLower;
              wLower = wNext;

              hLower = j * wRatio;
              while (hLower < hUpper)
              {
                double hScale = 1.0;
                double hNext;


                if (fract(hLower))
                {
                  hNext = std::ceilf(hLower);
                }
                else
                {
                  hNext = hLower + 1;
                }
                hNext = std::min(hNext, hUpper);

                hScale = hNext - hLower;
                y = (long)hLower;
                hLower = hNext;

                for (int c = 0; c < 3; c++)
                {
                  pixelColour[c] += upperLevel(x, y, c) * wScale * hScale;
                }
              }
            }

            pixelColour = pixelColour / pixelArea;
            for (int c = 0; c < 3; c++)
            {
              curLevel(i, j, c) = pixelColour[c];
            }
          }
        }
      }
    }
  }

  Texture(unsigned int layers = 0) : layers(layers) {}

  Texture(std::vector<Image> initImages, bool genMipMaps) : layers(initImages.size())
  {
    for (const Image& image : initImages) {
      images.push_back(image);
    }

    if (genMipMaps) {
      generateMipMaps();
    }
  }

  Texture(Image& img, bool genMipMaps = true) : layers(1), levels(1)
  {
    images.push_back(img);

    if (genMipMaps) {
      generateMipMaps();
    }
  }

};