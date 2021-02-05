#pragma once

#include <functional>

template <typename R>
R randomSampling(
	std::function<R(int i)> const &getSample,
    std::function<float(const R &, const R &)> const &sampleDistance,
	int minSamples = 4,
	int maxSamples = 1024,
	float accuracy = 0.005)
{
	R last_avg;
	R avg_r;

	uint32_t sampleCount = 0;
	int i = 0;

	for (i = 0; i < minSamples - 1; i++) {
		R sample = getSample(sampleCount);
		sampleCount++;
		avg_r = avg_r + (sample - avg_r) / R(sampleCount);
	}

	last_avg = avg_r;

	for (;i < maxSamples; i++) {
		R sample = getSample(sampleCount);
		sampleCount++;
		avg_r = avg_r + (sample - avg_r) / R(sampleCount);
		
		if(sampleDistance(last_avg, avg_r) < accuracy) {
			break;
		}
		
		last_avg = avg_r;
	}
	return avg_r;
}