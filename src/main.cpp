#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cmath>
#include <iostream>
#include <vector>
#include <random>

#include "Shader.h"
#include "Noise.h"

// Structure to represent a sphere with a center and radius
struct Sphere {
    glm::vec3 center;
    float radius;
};

// Function to generate a collection of spheres that form a cloud-like shape
std::vector<Sphere> generateCloudSpheres(
    float L, int N, float delta_ratio=0.1f, float sigma_ratio=0.2f,
    float alpha=2.f, float beta=5.f, float base_radius_ratio=0.3f)
{
    std::vector<Sphere> spheres;
    spheres.reserve(N);

    // Random number generators
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> uniform01(0.0f, 1.0f);
    auto randf = [&]() { return uniform01(gen); };

    // Gaussian distribution for random positioning
    float sigma = L * sigma_ratio;
    std::normal_distribution<float> gaussX(L/2, sigma);
    std::normal_distribution<float> gaussZ(L/2, sigma);

    float delta = L * delta_ratio;
    glm::vec2 center2D(L/2, L/2);

    // Generate a base sphere that serves as the foundation
    float base_y = randf() * (delta / 2.0f);
    float dx = L/2.0f;
    float dz = L/2.0f;
    float max_base_radius = std::min(std::min(dx, dz), (L - base_y)*0.5f);
    float base_radius = std::min(L * base_radius_ratio, max_base_radius);

    {
        Sphere s;
        s.center = glm::vec3(center2D.x, base_y + base_radius, center2D.y);
        s.radius = base_radius;
        spheres.push_back(s);
    }

    // Beta distribution approximation using Gamma distribution
    auto betaRand = [&](float a, float b) {
        std::gamma_distribution<float> gaA(a, 1.0f);
        std::gamma_distribution<float> gaB(b, 1.0f);
        float x = gaA(gen);
        float y = gaB(gen);
        return x / (x + y);
    };

    // Generate additional spheres
    for(int i=0; i<N-1; i++)
    {
        float x = std::max(0.0f, std::min(L, gaussX(gen)));
        float z = std::max(0.0f, std::min(L, gaussZ(gen)));
        float dx_ = std::min(x, L - x);
        float dz_ = std::min(z, L - z);
        float y_base = randf() * delta;

        float max_radius_y = (L - y_base)*0.5f;
        float d_max = std::min(std::min(dx_, dz_), max_radius_y);

        float min_radius = std::max(0.05f*L, base_radius*0.2f);
        float max_radius = std::min(d_max, 0.5f*L);

        float br = betaRand(alpha, beta);
        float radius = min_radius + br*(max_radius - min_radius);

        Sphere s;
        s.center = glm::vec3(x, y_base + radius, z);
        s.radius = radius;
        spheres.push_back(s);
    }

    return spheres;
}

// Compute a bounding sphere that encompasses all generated spheres
Sphere computeBoundingSphere(const std::vector<Sphere>& spheres)
{
    glm::vec3 minP(1e9f), maxP(-1e9f);

    // Determine the minimum and maximum bounds
    for(auto &s : spheres)
    {
        glm::vec3 c = s.center;
        for(int k=0; k<3; k++){
            if(c[k] - s.radius < minP[k]) minP[k] = c[k] - s.radius;
            if(c[k] + s.radius > maxP[k]) maxP[k] = c[k] + s.radius;
        }
    }

    // Compute the center and radius of the bounding sphere
    glm::vec3 center = 0.5f*(minP + maxP);
    float radius = 0.f;
    for(auto &s : spheres)
    {
        float dist_ = glm::length(s.center - center) + s.radius;
        if(dist_ > radius) radius = dist_;
    }

    Sphere bounding;
    bounding.center = center;
    bounding.radius = radius;
    return bounding;
}

// Define vertices for a full-screen triangle
static float vertices[] = {
    -1.0f, -1.0f, 0.0f,
     3.0f, -1.0f, 0.0f,
    -1.0f,  3.0f, 0.0f
};

int main()
{
    // Initialize GLFW for window management
    if(!glfwInit())
    {
        std::cerr << "Failed to init GLFW" << std::endl;
        return -1;
    }

    // Set OpenGL version and profile mode
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create GLFW window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Cloud Ray Marching (Static Noise)", nullptr, nullptr);
    if(!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Load OpenGL function pointers using GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to init GLAD" << std::endl;
        return -1;
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Generate cloud sphere data
    float L = 10.0f;
    int N   = 20;
    auto spheres = generateCloudSpheres(L, N);
    Sphere bounding = computeBoundingSphere(spheres);

    // Generate and bind Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Load shaders
    Shader shader("Shader/vertex_shader.glsl", "Shader/fragment_shader.glsl");

    glEnable(GL_DEPTH_TEST);

    while(!glfwWindowShouldClose(window))
    {
        // Update viewport size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
