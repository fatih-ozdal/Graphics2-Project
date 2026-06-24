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
#define OUT_CHECK_ACTIVE layout(location = 7 )

//#define U_TRANSFORM_MODEL   layout(location = 0)
#define U_TRANSFORM_VIEW    layout(location = 1)
#define U_TRANSFORM_PROJ    layout(location = 2)
//#define U_TRANSFORM_NORMAL  layout(location = 3)
#define U_CAM               layout(location = 69)

in IN_POS vec3 vPos;
in IN_NORMAL vec3 vNormal;
in IN_UV vec2 vUV;

out gl_PerVertex {vec4 gl_Position;};

out OUT_UV vec2 fUV; // tex coord
out OUT_NORMAL vec3 fNormal;
out OUT_DISTANCE vec3 fDistance;
out OUT_TOCAM vec3 fCam;
out OUT_WORLDPOS vec3 fWorldPos;
out OUT_CHECK_ACTIVE float check ;     

// U_TRANSFORM_MODEL uniform mat4 uModel;
U_TRANSFORM_VIEW uniform mat4 uView;
U_TRANSFORM_PROJ uniform mat4 uProjection;
// U_TRANSFORM_NORMAL uniform mat3 uNormalMatrix;
U_CAM uniform vec3 uCam;

mat4 uModel ;   
mat3 uNormalMatrix ;  

struct Missile {
    mat4 model_matrix;
    mat4 quaternion_mat4;
    mat4 inversemodel_matrix; 
    vec4 bl_rad_ifboom_ifactive_timesinceboom;  
    vec4 position_speed;  
    vec4 initialpos; 
    vec4 direction;  
    vec4 surfacenormal ;  
};

layout(std430, binding = 9) buffer MissileBuffer {
    Missile missiles[];
};

mat4 translate(vec3 offset) {
    return mat4(
        vec4(1.0, 0.0 , 0.0 , 0.0) , // Column 1
        vec4(0.0, 1.0 , 0.0 , 0.0) , // Column 2
        vec4(0.0, 0.0 , 1.0 , 0.0) , // Column 3
        vec4(offset.x , offset.y , offset.z  , 1.0)         // Column 4 (Translation values)
    );
}

void main(void)
{
    Missile my_missile = missiles[gl_InstanceID]; 
        if(my_missile.bl_rad_ifboom_ifactive_timesinceboom.z != 3.0f){
        check = 32.0f ;  
        gl_Position = vec4(2,2,2,2); // garbage pozsiyon verince dışarı attı zaten 
        return ;   
    }
    if(my_missile.bl_rad_ifboom_ifactive_timesinceboom.y == 1.0f){
        check = 32.0f ;  
        gl_Position = vec4(2,2,2,0.0); // garbage pozsiyon dışarı attı 
        return ;  
    }

    uNormalMatrix =  mat3(my_missile.inversemodel_matrix) ;  
    mat4 translate_missile_to_origin =  translate( 5.0f*( - my_missile.direction.xyz ));   //
    uModel =   my_missile.model_matrix * translate_missile_to_origin   ;  

    fUV = vUV;

    
    vec4 worldPos = uModel * vec4(vPos, 1.0f);
 // kaçıncı instanceda olduğumuzu aldım 
    gl_Position = uProjection * uView * worldPos; // bunu galiba önceden dönmek gerekiyo

    fNormal = normalize(uNormalMatrix * vNormal);

    fWorldPos = worldPos.xyz;
    
    
    
    fDistance = vec3(0.0f, -1.0f, 0.0f); 
    fCam = normalize(uCam - worldPos.xyz);
}