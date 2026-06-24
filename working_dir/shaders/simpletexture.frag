#version 430
/*
	File Name	: color.vert
	Author		: Bora Yalciner
	Description	:

		Basic fragment shader that just outputs
		color to the FBO
*/

// Definitions
// These locations must match between vertex/fragment shaders
#define IN_UV		layout(location = 0)
#define IN_NORMAL	layout(location = 1)
#define IN_COLOR	layout(location = 2)
#define IN_AVG_LUM	layout(location = 3)
#define IN_QUAD     layout(location = 6)


// This output must match to the COLOR_ATTACHMENTi (where 'i' is this location)
#define OUT_FBO		layout(location = 0)

// This must match GL_TEXTUREi (where 'i' is this binding)
#define T_ALBEDO	layout(binding = 0)
#define T_FBO       layout(binding = 1)
#define T_RENDERED  layout(binding = 2)

// This must match the first parameter of glUniform...() calls
#define U_MODE		layout(location = 0)
#define	U_AVGLUM	layout(location = 83)


// Input
in IN_UV	 vec2 fUV;
in IN_NORMAL vec3 fNormal;
in IN_AVG_LUM float avglum;

// Output
// This parameter goes to the framebuffer
out OUT_FBO vec4 fboColor;

// Uniforms
U_MODE uniform uint uMode;
U_AVGLUM uniform uint avglum_id ;


// Textures
uniform T_ALBEDO sampler2D tAlbedo;
uniform T_FBO sampler2D tfbo ;
uniform T_RENDERED sampler2D trendered;


void main(void)
{
	float logAvgLum = avglum;



	vec3 hdrvalues ;
	vec4 gethdr = texture(trendered, fUV) ;
    hdrvalues = texture(trendered, fUV).xyz ;

	float constant = 0.8 ;



	float l_pixel = dot(hdrvalues, vec3(0.2126, 0.7152, 0.0722));
	float exposure = (constant/logAvgLum)*l_pixel;

	exposure = (exposure/(exposure+1));
	vec3 tmcolor = clamp((exposure/l_pixel)*hdrvalues,vec3(0,0,0),vec3(1,1,1));
	vec3 newfragment = tmcolor; //  sigmoidal compression 
	float p = (1/2.2) ;
	vec3 final ;
	final.x = pow(newfragment.x,p); 
	final.y = pow(newfragment.y,p); 
	final.z = pow(newfragment.z,p); 
	fboColor = vec4(final,1.0f) ;

}