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
#define IN_POS			layout(location = 4)
#define IN_NORMAL		layout(location = 5)
#define IN_UV			layout(location = 2)
#define IN_COLOR		layout(location = 3)
#define IN_WATER		layout(location = 8) // bence gerek  yok şu anki hali de güzel



#define OUT_UV			layout(location = 0)
#define OUT_NORMAL		layout(location = 1)
#define OUT_COLOR		layout(location = 2)
#define OUT_HEIGHT		layout(location = 3)
#define OUT_DISTANCE	layout(location = 4)
#define OUT_TOCAM	layout(location = 5)
#define OUT_WATER	layout(location = 8)



#define U_TRANSFORM_MODEL	layout(location = 0)
#define U_TRANSFORM_VIEW	layout(location = 1)
#define U_TRANSFORM_PROJ	layout(location = 2)
#define U_TRANSFORM_NORMAL	layout(location = 3)
#define U_POINT_LIGHT		layout(location = 4 )
#define U_ANGLE				layout(location = 32)
#define U_ANGLE1				layout(location = 77)
#define U_TRANSFORM_MODEL_UNSCALED	layout(location = 33)
#define U_NORMAL_UNSCALED	layout(location = 34)
#define U_CAM	layout(location = 69)
#define U_IFWATER layout(location = 99)
#define U_TEX layout(location = 73)

// Input
in IN_POS	 vec3 vPos;
in IN_NORMAL vec3 vNormal;
in IN_UV	 vec2 vUV;
in IN_WATER vec3 vWater ;


// Output
// This parameter goes to rasterizer
// This is mandatory since we are using modern pipeline
out gl_PerVertex {vec4 gl_Position;};

// These pass through to rasterizer and will be iterpolated at
// fragment positions
out OUT_UV		vec2 fUV;
out OUT_NORMAL	vec3 fNormal;
out OUT_HEIGHT float fHeight;
out OUT_DISTANCE vec3 fDistance;
out OUT_TOCAM vec3 fCam;
out OUT_WATER vec3 fWater;



// Uniforms
U_TRANSFORM_MODEL	uniform mat4 uModel;
U_TRANSFORM_VIEW	uniform mat4 uView;
U_TRANSFORM_PROJ	uniform mat4 uProjection;
U_TRANSFORM_NORMAL  uniform mat3 uNormalMatrix;
U_ANGLE	uniform float uAngle;
U_ANGLE1	uniform float uAngle1;
U_POINT_LIGHT uniform vec3 uLightPos ;
U_TRANSFORM_MODEL_UNSCALED uniform mat4 uUnscaled ;
U_NORMAL_UNSCALED uniform mat3 uNormalUnscaled ;
U_CAM uniform vec3 uCam ;
U_IFWATER uniform int uifwater ;
U_TEX uniform uint  texno;









void main(void)
{
	if(uifwater==0 ){
	if(vPos.y < 0.0001f ){


	gl_Position = vec4(2.0,2.0,2.0,1.0);
	fHeight = -50.0f ;

	return ;

	}
	fUV = vUV;
	fNormal = normalize(uNormalMatrix * vNormal);
	fHeight = vPos.y;
	// Rasterizer
	vec4 tmpvec = uModel * vec4(vPos.xyz, 1.0f);
	gl_Position = uProjection * uView *tmpvec ;
	
	// fDistance = uLightPos-vec3(tmpvec.x,tmpvec.y,tmpvec.z); // Bunu iptal ettim
	fDistance = vec3(0.0f,-1.0f,0.0f); // Bu cisimden objeye olan ışının yönü
	fCam = normalize(uCam-vec3(tmpvec.x , tmpvec.y ,tmpvec.z));

	}else{
		// burası dalga için ben zaten unscaled olanları dalga için kullanıyoruym 
	float final_angle = (uAngle+vWater.x) ; // çarpınca dalganın frekansı değişiyodu hoşuma gitmedi ben topladım. Böyle sanki aynı dalga devam ediyo gibi
	float wave_cos =cos(final_angle)/2.5f;
	float wave_sin =sin(final_angle)/2.5f;
	float final_angle_2 = (uAngle1+vWater.x) ; // ikinic dalgayı ekledim
	float wave_cos_2 =cos(final_angle_2)/5.0f;
	float wave_sin_2 = 2*sin(final_angle_2)/5.0f;
	vec3 nevPos ;
	float ys = +wave_sin+wave_sin_2;
	nevPos.y = -wave_cos-wave_cos_2 ;
	nevPos.x = vWater.x ;
	nevPos.z = vWater.z ;
	vec3 nevNormal ;
	nevNormal =  -normalize(vec3(ys,-1.0f,0.0f));


	fUV = vUV;
	fNormal = normalize(uNormalUnscaled * nevNormal); // dalganın nomrali 
	fHeight = 0.0f ; // burda zaten 0ları aldığımızdan ben heighti 0 olarak yolladım dalganın heighti clampleme için kullanıolmayacak
	// Rasterizer
	vec4 tmpvec = uUnscaled * vec4(nevPos.xyz, 1.0f);
	gl_Position = uProjection * uView *tmpvec ;
	
	// fDistance = uLightPos-vec3(tmpvec.x,tmpvec.y,tmpvec.z); // Bunu iptal ettim
	fDistance = vec3(0.0f,-1.0f,0.0f); // Bu cisimden objeye olan ışının yönü
	fCam = normalize(uCam-vec3(tmpvec.x , tmpvec.y ,tmpvec.z)); // bu dalga

	return ;

	}
	
	
}
