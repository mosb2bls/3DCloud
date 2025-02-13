#version 330 core

// Screen UV coordinates passed from the vertex shader
in vec2 vTexCoord;
out vec4 FragColor;

// ========== Uniforms ==========
uniform vec2 iResolution;            // Screen resolution (can be retained or removed if unnecessary)
uniform float iTime;                 // Time variable for controlling rotation

// Bounding sphere information (used to determine the volumetric rendering region)
uniform vec3 uBoundingSphereCenter;
uniform float uBoundingSphereRadius;

// Cloud volume sphere data (each sphere is represented as vec4(x, y, z, r); maximum of 64 spheres)
uniform int uSphereCount;
uniform vec4 uSpheres[64];

// Static noise texture (uploaded from C++ and generated using Perlin noise)
uniform sampler2D uNoiseTex;

// ========== Signed Distance Function (SDF) for Cloud Volume ==========
// If the return value < 0, the point p is inside at least one sphere
float sdCloud(vec3 p)
{
    float d = 1e6;
    for (int i = 0; i < uSphereCount; i++) {
        vec3 center = uSpheres[i].xyz;
        float r = uSpheres[i].w;
        float distSphere = length(p - center) - r;
        d = min(d, distSphere);
    }
    return d;
}

// ========== Cloud Interior Density Function ==========
// If p is inside the cloud volume, use noise texture sampling to compute local density and apply Y-axis rotation
float cloudDensity(vec3 p)
{
    // If the point is not inside any cloud sphere, return a density of 0
    float distVal = sdCloud(p);
    if (distVal > 0.0)
    return 0.0;

    // Compute rotation angle (rotation speed can be adjusted)
    float angle = iTime * 0.05;
    float cosA = cos(angle);
    float sinA = sin(angle);

    // Rotate point p in the XZ plane
    float rx = p.x * cosA - p.z * sinA;
    float rz = p.x * sinA + p.z * cosA;
    vec3 rotatedP = vec3(rx, p.y, rz);

    // Sample noise texture (scaling factor 0.1 can be adjusted as needed)
    float noiseVal = texture(uNoiseTex, rotatedP.xz * 0.1).r;

    // Apply smoothstep function to create a soft transition effect
    return smoothstep(0.3, 1.0, noiseVal);
}

void main()
{
    // Map input texture coordinates to the range [-1, 1]
    vec2 uv = vTexCoord * 2.0 - 1.0;

    // ========== Compute Ray Origin and Direction ==========
    // The camera is fixed at a position in front of the bounding sphere (at twice the sphere's radius)
    vec3 ro = uBoundingSphereCenter + vec3(0.0, 0.0, uBoundingSphereRadius * 2.0);
    // Compute ray direction using a simple perspective projection
    vec3 rd = normalize(vec3(uv * uBoundingSphereRadius, -uBoundingSphereRadius * 2.0));

    // ========== Compute Ray-Sphere Intersection ==========
    vec3 c = uBoundingSphereCenter;
    float R = uBoundingSphereRadius;
    vec3 oc = ro - c;
    float b = dot(oc, rd);
    float c2 = dot(oc, oc) - R * R;
    float det = b * b - c2;
    if (det < 0.0) {
        // No intersection, output background color (gray)
        FragColor = vec4(0.6, 0.6, 0.6, 1.0);
        return;
    }
    float sqrtDet = sqrt(det);
    float t1 = -b - sqrtDet;
    float t2 = -b + sqrtDet;
    if (t2 < 0.0) {
        // The entire bounding sphere is behind the camera
        FragColor = vec4(0.6, 0.6, 0.6, 1.0);
        return;
    }
    float tNear = max(t1, 0.0);
    float tFar = t2;

    // ========== Ray Marching Parameters ==========
    const int STEPS = 64;
    float marchStep = (tFar - tNear) / float(STEPS);

    vec3 outColor = vec3(0.0);
    float transmittance = 1.0;

    // Base cloud color set to white
    vec3 fogColor = vec3(1.0);
    // Define an orange light source from the upper-right corner
    vec3 lightColor = vec3(1.0, 0.7, 0.5);
    // Light direction: from the upper-right direction (adjustable)
    vec3 lightDir = normalize(vec3(1.0, 1.0, -0.3));

    // Scattering and absorption coefficients (adjustable)
    float sigma_s = 2.0;
    float sigma_a = 0.2;

    // ========== Ray Marching Loop ==========
    for (int i = 0; i < STEPS; i++) {
        float tCurrent = tNear + float(i) * marchStep;
        vec3 pos = ro + rd * tCurrent;

        float dens = cloudDensity(pos);
        if (dens > 0.001) {
            // Simple shadow calculation
            float shadow = 1.0;
            vec3 lpos = pos;
            const float SHADOW_STEPS = 16.0;
            float stepSize = 0.05;
            for (float s = 0.0; s < SHADOW_STEPS; s += 1.0) {
                lpos += lightDir * stepSize;
                float dCloud = length(lpos - c) - R;
                if (dCloud > 0.0)
                break;
                float ds = cloudDensity(lpos);
                if (ds > 0.02) {
                    shadow *= exp(-ds * 0.3);
                    if (shadow < 0.01)
                    break;
                }
            }

            // Compute scattering: mix white (fogColor) and orange light (lightColor)
            // The mix parameter 0.3 controls the proportion of the orange component (adjustable)
            vec3 scattering = mix(fogColor, lightColor, 0.3) * shadow;
            vec3 stepColor = dens * sigma_s * scattering * transmittance * marchStep;
            outColor += stepColor;

            // Update transmittance (Beer-Lambert law)
            float absorb = dens * (sigma_a + sigma_s) * marchStep;
            transmittance *= exp(-absorb);
            if (transmittance < 0.001)
            break;
        }
    }

    // Final color: blend cloud color with background (gray background)
    vec3 backgroundColor = vec3(0.6);
    vec3 finalColor = outColor + backgroundColor * transmittance;
    FragColor = vec4(finalColor, 1.0);
}
