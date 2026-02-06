#version 440

layout(location = 0) in vec2 position;
layout(location = 0) out vec2 vTexCoord;

void main()
{
    // Convert from -1,1 NDC to 0,1 texture coordinates
    // Flip Y so that row 0 is at top (waterfall scrolls down)
    vTexCoord = vec2(position.x * 0.5 + 0.5, 1.0 - (position.y * 0.5 + 0.5));
    gl_Position = vec4(position, 0.0, 1.0);
}
