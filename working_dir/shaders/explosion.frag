#version 430

layout(location = 0) out vec4 fboColor;   // non-HDR: straight to the screen
layout(location = 1) out vec4 texcolor;   // HDR: into the tonemap MRT

layout(location = 24) uniform uint uHDR;
layout(binding  = 16) uniform sampler2D uSprite;

in vec2 fUV;

void main()
{
    vec4 c = texture(uSprite, fUV);
    if (c.a < 0.02) discard;                 // background was keyed transparent

    // Premultiplied color for additive blending (GL_ONE, GL_ONE): gives soft
    // edges and is independent of the output alpha -- which matters because in
    // HDR the .w channel carries log-luminance, not coverage.
    vec3 col = c.rgb * c.a;

    if (uHDR == 1u) {
        texcolor = vec4(col, 0.0);           // additive: w = 0 leaves luminance untouched
    } else {
        fboColor = vec4(col, 1.0);
    }
}
