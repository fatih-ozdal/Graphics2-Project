#version 430
/*
	File Name	: generic.vert
	Author		: Bora Yalciner
	Description	:

		Basic vertex transform shader...
		Just multiplies each vertex to MVP matrix
		and sends it to the rasterizer

		It supports custom per-vertex normals
		and texture (uv) coordinates.
*/

// Definitions
#define IN_POS			layout(location = 0)
#define IN_NORMAL		layout(location = 1)
#define IN_UV			layout(location = 2)
#define IN_COLOR		layout(location = 3)
#define IN_QUAD         layout(location = 6)

#define OUT_UV			layout(location = 0)
#define OUT_NORMAL		layout(location = 1)
#define OUT_COLOR		layout(location = 2)
#define OUT_AVG_LUM		layout(location = 3)

#define U_TRANSFORM_MODEL	layout(location = 0)
#define U_TRANSFORM_VIEW	layout(location = 1)
#define U_TRANSFORM_PROJ	layout(location = 2)
#define U_TRANSFORM_NORMAL	layout(location = 3)
#define	U_AVGLUM	layout(location = 83)

// Input
in IN_POS	 vec3 vPos;
in IN_NORMAL vec3 vNormal;
in IN_UV	 vec2 vUV;
in IN_QUAD     vec3 vQUAD ;

// Output
// This parameter goes to rasterizer
// This is mandatory since we are using modern pipeline
out gl_PerVertex {vec4 gl_Position;};

// These pass through to rasterizer and will be iterpolated at
// fragment positions
out OUT_UV		vec2 fUV;
out OUT_NORMAL	vec3 fNormal;
out OUT_AVG_LUM float avglum ;

// Uniforms
U_TRANSFORM_MODEL	uniform mat4 uModel;
U_TRANSFORM_VIEW	uniform mat4 uView;
U_TRANSFORM_PROJ	uniform mat4 uProjection;
U_TRANSFORM_NORMAL  uniform mat3 uNormalMatrix;
U_AVGLUM	uniform sampler2D uMipmapID ;

void main(void)
{

	// Vertex shaders have no implicit LOD, so the texture() bias overload is
	// unavailable under a core profile; sample an explicit mip with textureLod.
	avglum = exp(textureLod(uMipmapID,vec2(0.5,0.5),20.0).a);
    vec3 tmpquad = vQUAD ;
    fUV = vec2(((tmpquad.x)+1.0f)/2.0f,((tmpquad.y)+1.0f)/2.0f ) ; // bunlar vertex pozisyonları .  
    gl_Position = vec4(tmpquad.xyz,1.0f);
}