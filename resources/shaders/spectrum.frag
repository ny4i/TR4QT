#version 440

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    vec4 glowColor;        // offset 0:  glow color (cyan: 0, 0.83, 1.0, 1.0)
    float glowIntensity;   // offset 16: glow brightness (0.8)
    float glowWidth;       // offset 20: glow falloff width (0.04)
    float spectrumHeight;  // offset 24: viewport height in pixels
    float binCount;        // offset 28: number of valid bins in texture
    vec2 viewportSize;     // offset 32: viewport size in pixels
    float textureWidth;    // offset 40: spectrum texture width (2048)
    float padding;         // offset 44: alignment padding
};

layout(binding = 1) uniform sampler2D spectrumData;    // R32F 2048x1 normalized amplitude
layout(binding = 2) uniform sampler2D spectrumColorLut; // RGBA8 256x1 color lookup

void main()
{
    float x = fragTexCoord.x;
    float y = fragTexCoord.y;  // 0 = top, 1 = bottom

    // Sample spectrum amplitude at this X position (normalized 0..1)
    float amplitude = texture(spectrumData, vec2(x, 0.5)).r;
    amplitude = clamp(amplitude, 0.0, 1.0);

    // Spectrum peak Y position (0 = top, 1 = bottom)
    // amplitude=1 means peak at top (y=0), amplitude=0 means peak at bottom (y=1)
    float peakY = 1.0 - amplitude;

    // Distance from this pixel to the peak edge
    float distFromPeak = y - peakY;

    // Below the peak: filled region
    if (distFromPeak >= 0.0) {
        // How far down from peak to bottom (0 at peak, 1 at bottom)
        float fillRatio = distFromPeak / max(1.0 - peakY, 0.001);

        // Color from LUT based on amplitude at this column
        vec4 lutColor = texture(spectrumColorLut, vec2(amplitude, 0.5));

        // Fade toward bottom (darken near noise floor)
        float bottomFade = 1.0 - fillRatio * 0.6;

        // Bright edge at peak, gradually darker below
        float edgeBright = 1.0 - fillRatio * 0.3;

        fragColor = vec4(lutColor.rgb * bottomFade * edgeBright, 1.0);
        return;
    }

    // Above the peak: glow region
    float glowDist = -distFromPeak;  // positive distance above peak
    float glowFalloff = exp(-glowDist / max(glowWidth, 0.001));

    // Only show glow if amplitude is significant
    float glowAlpha = glowFalloff * glowIntensity * amplitude;

    if (glowAlpha < 0.01) {
        discard;
    }

    fragColor = vec4(glowColor.rgb, glowAlpha);
}
