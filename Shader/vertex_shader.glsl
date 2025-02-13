#version 330 core

layout(location = 0) in vec3 aPos;
out vec2 vTexCoord;

void main()
{
    // Map the [-1,1]^2 vertex coordinates to screen space
    gl_Position = vec4(aPos, 1.0);
    // Pass the XY coordinates to the fragment shader for UV computation
    vTexCoord = aPos.xy * 0.5 + 0.5;
}