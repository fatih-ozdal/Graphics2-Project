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
#define IN_TOCAM	layout(location = 5) 
#define IN_QUAD		layout(location = 6)

#define OUT_UV			layout(location = 0)
#define OUT_NORMAL		layout(location = 1)
#define OUT_COLOR		layout(location = 2)
#define OUT_DISTANCE	layout(location = 4)
#define OUT_TOCAM	layout(location = 5)
#define OUT_REFLECTION	layout(location = 8)


#define U_TRANSFORM_MODEL	layout(location = 0)
#define U_TRANSFORM_VIEW	layout(location = 1)
#define U_TRANSFORM_PROJ	layout(location = 2)
#define U_TRANSFORM_NORMAL	layout(location = 3)
#define U_PMODE layout(location = 91 )
#define U_HDR layout(location = 24)
#define U_CAM layout(location = 69)
#define U_TEX layout(location = 73)

// Input
in IN_POS	 vec3 vPos;
in IN_NORMAL vec3 vNormal;
in IN_UV	 vec2 vUV;
in IN_QUAD	 vec3 myquad;


// Output
// This parameter goes to rasterizer
// This is mandatory since we are using modern pipeline
out gl_PerVertex {vec4 gl_Position;};

// These pass through to rasterizer and will be iterpolated at
// fragment positions
out OUT_UV		vec2 fUV;
out OUT_NORMAL	vec3 fNormal;
out OUT_DISTANCE vec3 fDistance;
out OUT_TOCAM vec3 fCam;
out OUT_REFLECTION vec3 fReflectionRay ;

// Uniforms
U_TRANSFORM_MODEL	uniform mat4 uModel;
U_TRANSFORM_VIEW	uniform mat4 uView;
U_TRANSFORM_PROJ	uniform mat4 uProjection;
U_TRANSFORM_NORMAL  uniform mat3 uNormalMatrix;
U_CAM uniform vec3 uCam ;



U_PMODE uniform uint uPmode;
U_HDR uniform uint Uhdr;

U_TEX uniform uint  texno;


vec2 calculate_texture_position(vec3  yeteerrrrrrr , vec3 fCam1 , vec3 fNormal1){

			// fcam kameradan objeye giden açı
			//
			normalize(fNormal1); 
			vec3 neewcam = -fCam1; // objeden kameraya dönen
			vec3 tobesbtracted =neewcam-dot(neewcam,fNormal1)*fNormal1; // bunu iki kez çıkarcaz newcandan
			// ki yansıma açısını bulalım
			vec3 noktadan_giden = normalize(neewcam-2*tobesbtracted);
			vec3 direction = -noktadan_giden ;
			vec3 finalpoint = direction ;
			// bu nokta istediğimiz nokta
			float garipsay = sqrt(finalpoint.x * finalpoint.x +finalpoint.z * finalpoint.z );
			float theta = atan(finalpoint.y/garipsay );
			// eski pi pi aralığı istiyosak böyle vermemiz gerekiyo


			float omega = atan(finalpoint.z,finalpoint.x);
			// galiba böyle olunca eski pi pi aralığında dönüyo 

			float pi = 3.1416f;
			float finalI = ((omega+pi)/(2*pi)) ;
			float finalJ = ((pi/2+theta)/(pi)) ;
			vec2 sonnn = vec2(finalI,finalJ);
			return sonnn ;

} // frag shaderda çok yer kaplıyor.



vec3 calculate_3d_vec( vec3 fCam1 , vec3 fNormal1){


			normalize(fNormal1); 
			vec3 neewcam = -fCam1; 
			vec3 tobesbtracted =neewcam-dot(neewcam,fNormal1)*fNormal1;

			vec3 noktadan_giden = normalize(neewcam-2*tobesbtracted);
			vec3 direction = -noktadan_giden ;
			vec3 finalpoint = direction ;
			return normalize(finalpoint)  ;  


} 






void main(void)
{
	if(uPmode != 5){
	fUV = vUV;
	fNormal = normalize(uNormalMatrix * vNormal);
	vec4 tmpvec = uModel * vec4(vPos.xyz, 1.0f);
	gl_Position = uProjection * uView * uModel * vec4(vPos.xyz, 1.0f);
	fDistance = vec3(0.0f,-1.0f,0.0f);
	fCam = normalize(uCam-vec3(tmpvec.x , tmpvec.y ,tmpvec.z));
	if(uPmode == 3 && texno != 4){
		fUV = calculate_texture_position(tmpvec.xyz , fCam , fNormal);
	}else if(texno == 4 && uPmode == 3 ){

	}
	}else{
	gl_Position = vec4(myquad.xy,0.9999f ,1.0f);
	fUV = vec2(myquad.xy); // bu heralde uçağın kokpitteki reflectance'ı içindi niye fUV'la göndermişim anlamadım heralde fUV kullanılmıyo fragda
	fDistance = vec3(0.0f,-1.0f,0.0f);
	fCam =  -vec3(0.0f , 0.0f ,0.0f);
	}
}