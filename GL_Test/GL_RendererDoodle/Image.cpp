#include "Image.h"

#include <string.h>
#include <vector>

#include <glm/glm.hpp>

#include <stb_image.h>
#include <stb_image_write.h>

bool Image::savePng(const std::string& filename) const {
	std::vector<unsigned char> imgdata(w * h * 3);

	int idx = 0;
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			for (int k = 0; k < 3; k++) {
				imgdata[idx] = static_cast<unsigned char>(glm::clamp(data[idx], (imageData)0.0, (imageData)1.0) * 255.0);
				idx++;
			}
		}
	}

	return stbi_write_png(filename.c_str(), w, h, 3, imgdata.data(), sizeof(unsigned char) * w * 3);
}

Image Image::loadPng(const std::string& filename) {
	int x = 0, y = 0, n = 0;

	unsigned char* data = stbi_load(filename.c_str(), &x, &y, &n, 3);

	Image img(x, y);
	img.id = filename;

	if (data) {

		int idx = 0;
		for (int i = 0; i < x; i++) {
			for (int j = 0; j < y; j++) {
				for (int k = 0; k < 3; k++) {
					img(i, j, k) = data[idx] / 255.0;
					idx++;
				}
			}
		}
	}
	stbi_image_free(data);
	return img;
}