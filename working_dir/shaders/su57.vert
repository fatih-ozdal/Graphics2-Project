#version 430

// Match your exact definitions
#define IN_POS      layout(location = 0)
#define IN_NORMAL   layout(location = 1)
#define IN_UV       layout(location = 2)

#define OUT_UV       layout(location = 0)
#define OUT_NORMAL   layout(location = 1)
#define OUT_DISTANCE layout(location = 4)
#define OUT_TOCAM    layout(location = 5)
#define OUT_WORLDPOS layout(location = 6) 

#define U_TRANSFORM_MODEL   layout(location = 0)
#define U_TRANSFORM_VIEW    layout(location = 1)
#define U_TRANSFORM_PROJ    layout(location = 2)
#define U_TRANSFORM_NORMAL  layout(location = 3)
#define U_CAM               layout(location = 69)

in IN_POS vec3 vPos;
in IN_NORMAL vec3 vNormal;
in IN_UV vec2 vUV;

out gl_PerVertex {vec4 gl_Position;};

out OUT_UV vec2 fUV;
out OUT_NORMAL vec3 fNormal;
out OUT_DISTANCE vec3 fDistance;
out OUT_TOCAM vec3 fCam;
out OUT_WORLDPOS vec3 fWorldPos;

U_TRANSFORM_MODEL uniform mat4 uModel;
U_TRANSFORM_VIEW uniform mat4 uView;
U_TRANSFORM_PROJ uniform mat4 uProjection;
U_TRANSFORM_NORMAL uniform mat3 uNormalMatrix;
U_CAM uniform vec3 uCam;

void main(void)
{
    fUV = vUV;
    fNormal = normalize(uNormalMatrix * vNormal);
    
    vec4 worldPos = uModel * vec4(vPos, 1.0f);
    fWorldPos = worldPos.xyz;
    
    gl_Position = uProjection * uView * worldPos;
    
    fDistance = vec3(0.0f, -1.0f, 0.0f); 
    fCam = normalize(uCam - worldPos.xyz);
}