#include "Noise.h"
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>

// Anonymous namespace to encapsulate the Perlin noise implementation details
namespace {
    // Global permutation array of size 512 used for Perlin noise calculations
    int p[512];

    // Smooth interpolation function (defined by Ken Perlin)
    double fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    // Linear interpolation function
    double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }

    // Gradient function: returns a gradient value based on the hash value
    double grad(int hash, double x, double y) {
        int h = hash & 7; // Extract the lowest 3 bits
        double u = h < 4 ? x : y;
        double v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    // Initialize the permutation array `p[]` using a given seed
    void initPermutation(int seed) {
        std::vector<int> permutation(256);
        for (int i = 0; i < 256; i++) {
            permutation[i] = i;
        }
        // Shuffle using the given seed
        srand(seed);
        for (int i = 255; i > 0; i--) {
            int j = rand() % (i + 1);
            std::swap(permutation[i], permutation[j]);
        }
        // Duplicate the permutation values to extend it to 512 elements
        for (int i = 0; i < 256; i++) {
            p[i] = permutation[i];
            p[i + 256] = permutation[i];
        }
    }

    // 2D Perlin noise function, returns a value approximately in the range [-1, 1]
    double perlin(double x, double y) {
        // Determine grid cell coordinates
        int X = (int)floor(x) & 255;
        int Y = (int)floor(y) & 255;
        // Compute fractional part of input coordinates
        x -= floor(x);
        y -= floor(y);
        // Compute fade curves for x and y
        double u = fade(x);
        double v = fade(y);
        // Hash coordinates of the four cell corners
        int A = p[X] + Y;
        int B = p[X + 1] + Y;
        // Perform bilinear interpolation using gradient values
        double res = lerp(v,
                          lerp(u, grad(p[A], x, y),
                                  grad(p[B], x - 1, y)),
                          lerp(u, grad(p[A + 1], x, y - 1),
                                  grad(p[B + 1], x - 1, y - 1)));
        return res;
    }
}

// Generates a Perlin noise texture of specified width and height using a seed
std::vector<unsigned char> Noise::generatePerlinNoiseTexture(int width, int height, int seed) {
    // Initialize the permutation table with the given seed
    initPermutation(seed);

    // Vector to store the grayscale noise texture data (size: width * height)
    std::vector<unsigned char> textureData(width * height);

    // Controls the level of detail in the noise (higher values create finer noise patterns)
    double frequency = 8.0;

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            // Normalize pixel coordinates to range [0, 1]
            double x = (double)i / (double)width;
            double y = (double)j / (double)height;
            // Compute Perlin noise value at (x, y)
            double noiseValue = perlin(x * frequency, y * frequency);
            // Map Perlin noise value from [-1, 1] to [0, 1]
            noiseValue = (noiseValue + 1.0) / 2.0;
            // Clamp values to ensure they remain in the [0,1] range
            noiseValue = std::min(std::max(noiseValue, 0.0), 1.0);
            // Scale to the [0, 255] range for grayscale image representation
            unsigned char value = (unsigned char)(noiseValue * 255);
            textureData[j * width + i] = value;
        }
    }
    return textureData;
}
