#pragma once

#include <vector>

namespace Perlin {

	static void randomize(std::vector<float>& values)
	{
		for (int i = 0; i < (int)values.size(); i++) values[i] = (float)rand() / (float)RAND_MAX;
	}

	static void perlinNoise(const int width, const std::vector<float> fSeed, const int octaves, const float fBias, std::vector<float>& fOut)
	{

		for (int x = 0; x < width; x++)
		{
			float fNoise = 0.0f;
			float fScaleAcc = 0.0f;
			float fScale = 1.0f;

			for (int o = 0; o < octaves; o++)
			{
				int nPitch = width >> o;
				nPitch = nPitch > 0 ? nPitch : 1;
				int nSample1 = (x / nPitch) * nPitch;
				int nSample2 = (nSample1 + nPitch) % width;

				float fBlend = (float)(x - nSample1) / (float)nPitch;

				float fSample = (1.0f - fBlend) * fSeed[nSample1] + fBlend * fSeed[nSample2];

				fScaleAcc += fScale;
				fNoise += fSample * fScale;
				fScale = fScale / fBias;
			}

			// Scale to seed range
			fOut[x] = fNoise / fScaleAcc;
		}
	}

	static void perlinNoise(const int width, const int height, const std::vector<float> fSeed, const int octaves, const float fBias, std::vector<float>& fOut)
	{

		for (int x = 0; x < width; x++)
			for (int y = 0; y < height; y++)
			{
				float fNoise = 0.0f;
				float fScaleAcc = 0.0f;
				float fScale = 1.0f;

				for (int o = 0; o < octaves; o++)
				{
					int nPitch = width >> o;
					nPitch = nPitch > 0 ? nPitch : 1;
					int nSampleX1 = (x / nPitch) * nPitch;
					int nSampleY1 = (y / nPitch) * nPitch;

					int nSampleX2 = (nSampleX1 + nPitch) % width;
					int nSampleY2 = (nSampleY1 + nPitch) % width;

					float fBlendX = (float)(x - nSampleX1) / (float)nPitch;
					float fBlendY = (float)(y - nSampleY1) / (float)nPitch;

					float fSampleT = (1.0f - fBlendX) * fSeed[(long long)nSampleY1 * width + nSampleX1] + fBlendX * fSeed[(long long)nSampleY1 * width + nSampleX2];
					float fSampleB = (1.0f - fBlendX) * fSeed[(long long)nSampleY2 * width + nSampleX1] + fBlendX * fSeed[(long long)nSampleY2 * width + nSampleX2];

					fScaleAcc += fScale;
					fNoise += (fBlendY * (fSampleB - fSampleT) + fSampleT) * fScale;
					fScale = fScale / fBias;
				}

				// Scale to seed range
				fOut[(long long)y * width + x] = fNoise / fScaleAcc;
			}

	}


};