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
#define IN_DISTANCE		layout(location = 4)
#define IN_TOCAM	layout(location = 5)
#define IN_QUAD		layout(location = 6)
#define IN_REFLECTION		layout(location = 8)

// This output must match to the COLOR_ATTACHMENTi (where 'i' is this location)
#define OUT_FBO		layout(location = 0)
#define OUT_TEX		layout(location = 1)

// This must match GL_TEXTUREi (where 'i' is this binding)
#define T_ALBEDO	layout(binding = 0)
#define BASE_ALBEDO layout(binding = 1)
#define BASE_ROUGH layout(binding = 2)
#define HELIX_ALBEDO layout(binding = 3)
#define HELIX_ROUGH layout(binding = 4)
#define CHURCH layout(binding = 13)
#define SKY layout(binding = 14)
#define SUNSET layout(binding = 15)
#define T_RENDERED layout(binding = 17)

#define T_MORNING layout(binding =  27)


// This must match the first parameter of glUniform...() calls
#define U_MODE		layout(location = 0)
#define U_PMODE layout(location = 91 ) 
#define U_HDR layout(location = 24 ) 
#define U_INV layout(location = 61)
#define U_LEN layout(location = 54)
#define U_WID	layout(location = 55)
#define U_DEPTH layout(location = 56)
#define U_UP layout(location = 63)
#define U_GAZE layout(location = 64)
#define U_TEX layout(location = 73)
#define U_SAMPLER 	layout(location = 83)



// Input
in IN_UV	 vec2 fUV;
in IN_NORMAL vec3 fNormal;
in IN_DISTANCE vec3 fDistance;
in IN_TOCAM vec3 fCam;
out IN_REFLECTION vec3 fReflectionRay;
// Output
// This parameter goes to the framebuffer
out OUT_FBO vec4 fboColor;
out OUT_TEX vec4 texcolor; // texture

// Uniforms
U_MODE uniform uint uMode;
U_PMODE uniform uint uPmode;
U_HDR uniform uint uHDR ;
U_INV uniform mat4 uINV ;
U_LEN uniform uint uLEN ;
U_WID uniform uint uWOD ;
U_DEPTH uniform uint uDepth ; 
U_UP uniform vec3 upvec ;
U_GAZE uniform vec3 gazevec ;
U_TEX uniform uint  texno;

U_SAMPLER uniform uint sampler_id ;



// Textures
uniform T_ALBEDO sampler2D tAlbedo;
uniform BASE_ALBEDO sampler2D bAlbedo;
uniform BASE_ROUGH sampler2D bRough ;
uniform HELIX_ALBEDO sampler2D hAlbedo ;
uniform HELIX_ROUGH sampler2D hRough ;
uniform CHURCH sampler2D Tchurch ;
uniform SKY sampler2D Tsky ;
uniform SUNSET sampler2D Tsunset ;

uniform T_RENDERED sampler2D trendered;


uniform T_MORNING samplerCube t_morning_cube ; 




void main(void)
{




   




	


	uint pmode = uPmode ;
	uint ifhdr = uHDR;
	float luminance;
	switch(ifhdr){
	case 1:{
	
	switch(uPmode)
	{

		case 5:{



			vec2 current_frag = gl_FragCoord.xy ;
			float lateralbetwzerone = (current_frag.x-0.5f)/uWOD ;
			float verticalbetwzerone = (current_frag.y-0.5f)/uLEN ;
			// 1 0 arasıda ikisi de
			lateralbetwzerone  = lateralbetwzerone*2 - 1;
			verticalbetwzerone  = verticalbetwzerone*2 - 1;
			// şimdi -1 ve 1 arasında geldiler
			float depth = uDepth ;
			depth = 0.99f ; // güvenli olsun diye 1 fark koydum zaten orda kisme olmaz
			// elde bütün x y z noktaları var
			vec4 finalpoint ;
			finalpoint = vec4(lateralbetwzerone,verticalbetwzerone,depth,1.0f );
			finalpoint = uINV*finalpoint ;
			vec3 the_real_angle ; 
			the_real_angle = normalize(finalpoint.xyz) ;  // bu her bir fragment için hesapladığım açı.
			// finalpoint = finalpoint/finalpoint.z ;
			vec2 sonnn ;  
			if(texno !=  4){
			float garipsay = sqrt(finalpoint.x * finalpoint.x +finalpoint.z * finalpoint.z );
			float theta = atan(finalpoint.y/garipsay );
			// eski pi pi aralığı istiyosak böyle vermemiz gerekiyo


			float omega = atan(finalpoint.z,finalpoint.x);
			// galiba böyle olunca eski pi pi aralığında dönüyo 

			float pi = 3.1416f;
			float finalI = ((omega+pi)/(2*pi)) ;
			float finalJ = ((pi/2+theta)/(pi)) ;
			sonnn = vec2(finalI,finalJ);
			}



			switch(texno){
				case 0:{
					texcolor = texture(Tsunset , sonnn );
					break;
				}
				case 1:{
					texcolor = texture(Tsky , sonnn );
					break;
				}
				case 2:{
					texcolor = texture(Tchurch , sonnn );
					break;
				}
				case 3:{
					vec4 col1 = texture(Tsunset , sonnn );
					vec4 col2 = texture(Tsky , sonnn );
					texcolor = mix(col1 , col2 , 0.5f );
					break;
				}case 4:{

					texcolor  = texture(t_morning_cube,the_real_angle) ;  

					break ; 
				}
				default:{
					texcolor = texture(Tsunset , sonnn );
					break;
				}

			}
			
			break;
		}
		default: texcolor = vec4(0.9f,0.9f,0.1f,1.0f); break;
	}

	luminance = dot(texcolor.xyz,vec3(0.2126f,0.7152f,0.0722));
	texcolor.w= log(luminance);


	break ;
	}	
	default:
	{
	switch(uPmode)
	{
		// Pure Red
		case 0: fboColor = vec4(1, 0, 0, 1); break;
		// Vertex Normals. Normal axes by definition is between [-1, 1])
		// Color is in between [0, 1]) so we adjust here for that
		//case 1: fboColor = vec4((fNormal + 1) * 0.5, 1); break;
		case 1: fboColor = texture(bAlbedo, fUV) ; break;
		// UV
		case 2: fboColor = texture(hAlbedo, fUV) ; break;
		// Texture Mapping without shading.
		case 3: fboColor = vec4(1); break;
		// If mode is wrong, put pure white.
		case 5:
			fboColor = texture(Tsunset , fUV );
		default: fboColor = vec4(1); break;
	}
	break;
	}
	}
}