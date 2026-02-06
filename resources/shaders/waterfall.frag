#version 440

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    float rowOffset;       // Circular buffer offset (0.0-1.0)
    float refLevel;        // Reference level in dB
    float waterfallRange;  // Dynamic range in dB
    float padding;
};

layout(binding = 1) uniform sampler2D waterfallTexture;
layout(binding = 2) uniform sampler2D colorLut;

void main()
{
    // Apply circular buffer offset for scrolling
    // Newest row at top, oldest at bottom
    // rowOffset points to newest row, subtract vTexCoord.y to scroll down
    vec2 scrolledCoord = vec2(vTexCoord.x, fract(rowOffset - vTexCoord.y + 1.0));

    // Sample waterfall texture (R32F format - dB value in red channel)
    float db = texture(waterfallTexture, scrolledCoord).r;

    // Normalize dB value to 0-1 range for LUT lookup
    float maxDb = -20.0 + refLevel;
    float minDb = maxDb - waterfallRange;
    float normalized = clamp((db - minDb) / (maxDb - minDb), 0.0, 1.0);

    // Lookup color from 1D LUT texture
    // Use center of texel (0.5/256 offset) for accurate sampling
    fragColor = texture(colorLut, vec2(normalized, 0.5));
}
