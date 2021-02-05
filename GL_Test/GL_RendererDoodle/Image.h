#pragma once

#include <string>

#include <glm/glm.hpp>

typedef float imageData;

struct Image {
	imageData* data;
	size_t w, h;

	Image(size_t width, size_t height) 
		: data(nullptr), w(width), h(height)
	{
		data = new imageData[width * height * 3];
	}

	Image(const Image& other) 
		: data(nullptr), w(other.w), h(other.h)
	{
		data = new imageData[w * h * 3];
		memcpy(data, other.data, sizeof(imageData) * w * h * 3);
	}

	~Image() {
		delete[] data;
	}

	uint32_t width() const { return w;  }
	uint32_t height() const { return h; }

	inline const glm::vec3& operator()(size_t x, size_t y) const 
	{
		return *((glm::dvec3*)&data[(y * w + x) * 3]);
	}

	glm::vec3& operator()(size_t x, size_t y) 
	{
		return *((glm::vec3*)&data[(y * w + x) * 3]);
	}

	inline const imageData& operator()(size_t x, size_t y, unsigned char c) const
	{
		return data[(y * w + x) * 3 + c];
	}

	inline imageData& operator()(size_t x, size_t y, unsigned char c)
	{
		return data[(y * w + x) * 3 + c];
	}

	bool savePng(const std::string& filename) const;

	static Image loadPng(const std::string& filename);
};