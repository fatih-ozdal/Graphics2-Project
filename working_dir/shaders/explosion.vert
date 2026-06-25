#version 430

// Camera-facing billboard for the explosion sprite. No vertex attributes:
// the four corners come from gl_VertexID, drawn as a triangle strip.

layout(location = 1)  uniform mat4  uView;
layout(location = 2)  uniform mat4  uProjection;
layout(location = 70) uniform vec3  uCenter;      // explosion world position
layout(location = 71) uniform float uSize;        // world units per side
layout(location = 72) uniform int   uFrame;       // current frame, 0..count-1
layout(location = 74) uniform int   uFrameCount;  // frames in the sheet (4)

out gl_PerVertex { vec4 gl_Position; };
out vec2 fUV;

void main()
{
    // Triangle-strip quad corners: v0=(-,-) v1=(+,-) v2=(-,+) v3=(+,+)
    vec2 corner = vec2((gl_VertexID == 1 || gl_VertexID == 3) ?  0.5 : -0.5,
                       (gl_VertexID >= 2)                      ?  0.5 : -0.5);

    // Offset in view space so the quad always faces the camera.
    vec4 centerView = uView * vec4(uCenter, 1.0);
    vec3 posView    = centerView.xyz + vec3(corner * uSize, 0.0);
    gl_Position     = uProjection * vec4(posView, 1.0);

    // Sub-UV into the current frame (frames laid out horizontally).
    vec2 cuv = corner + 0.5;   // 0..1 within the quad
    fUV = vec2((float(uFrame) + cuv.x) / float(uFrameCount), cuv.y);
}
