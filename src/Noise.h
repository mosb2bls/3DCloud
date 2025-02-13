#ifndef NOISE_H
#define NOISE_H

#include <vector>


class Noise {
public:
    static std::vector<unsigned char> generatePerlinNoiseTexture(int width, int height, int seed = 0);
};

#endif // NOISE_H
