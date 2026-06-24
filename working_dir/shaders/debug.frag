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
#define IN_HEIGHT		layout(location = 3)
#define IN_DISTANCE		layout(location = 4)
#define IN_TOCAM	layout(location = 5)
#define IN_WATER	layout(location = 8)



// This output must match to the COLOR_ATTACHMENTi (where 'i' is this location)
#define OUT_FBO		layout(location = 0)
#define OUT_TEX		layout(location = 1)

// This must match GL_TEXTUREi (where 'i' is this binding)
#define T_ALBEDO	layout(binding = 0)
#define	GRASS_DIFF 	layout(binding = 5)
#define GRASS_ROUGH layout(binding = 6)
#define ROCK_DIFF layout(binding = 7)
#define ROCK_ROUGH layout(binding = 8 )
#define SHORE_DIFF layout(binding = 9 )
#define SHORE_ROUGH layout(binding = 10 )
#define SNOW_DIFF layout(binding = 11 )
#define SNOW_ROUGH layout(binding = 12 )

#define CHURCH layout(binding = 13)
#define SKY layout(binding = 14)
#define SUNSET layout(binding = 15)
#define MID_MORNING layout(binding = 27)



// This must match the first parameter of glUniform...() calls
#define U_MODE		layout(location = 0)
#define U_HDR layout(location = 24 ) 
#define U_IFWATER layout(location = 99)
#define U_TEX layout(location = 73)

// Input
in IN_UV	 vec2 fUV;
in IN_NORMAL vec3 fNormal;
in IN_HEIGHT float fHeight;
in IN_DISTANCE vec3 fDistance;
in IN_TOCAM vec3 fCam;
in IN_WATER vec3 fWater;


// Output
// This parameter goes to the framebuffer
out OUT_FBO vec4 fboColor;
out OUT_TEX vec4 texcolor;

// Uniforms
U_IFWATER uniform int uifwater ;
U_MODE uniform uint uMode;
uniform float umaxheight;
uniform float uminheight;
U_HDR uniform uint uHDR ;

U_TEX uniform uint  texno;

// Textures

uniform GRASS_DIFF sampler2D gDiff ; 
uniform GRASS_ROUGH sampler2D gRough ; 
uniform ROCK_DIFF sampler2D rDiff ; 
uniform ROCK_ROUGH sampler2D rRough ; 
uniform SHORE_DIFF sampler2D sH_diff ; 
uniform SHORE_ROUGH sampler2D sH_rough ; 
uniform SNOW_DIFF sampler2D sN_diff ; 
uniform SNOW_ROUGH sampler2D sN_rough ; 
uniform T_ALBEDO sampler2D tAlbedo;

uniform SUNSET sampler2D Tsunset ;
uniform CHURCH sampler2D Tchurch ;
uniform SKY sampler2D Tsky ;
uniform MID_MORNING samplerCube Tmorning ;  

vec3 calculate_3d_vec(vec3  yeteerrrrrrr , vec3 fCam1 , vec3 fNormal1){

			normalize(fNormal1); 
			vec3 neewcam = -fCam1; 
			vec3 tobesbtracted =neewcam-dot(neewcam,fNormal1)*fNormal1; 

			vec3 noktadan_giden = normalize(neewcam-2*tobesbtracted);
			vec3 direction = -noktadan_giden ; 
			return direction ;





}


vec2 calculate_texture_position(vec3  yeteerrrrrrr , vec3 fCam1 , vec3 fNormal1){



			normalize(fNormal1); 
			vec3 neewcam = -fCam1; 
			vec3 tobesbtracted =neewcam-dot(neewcam,fNormal1)*fNormal1; 

			vec3 noktadan_giden = normalize(neewcam-2*tobesbtracted);
			vec3 direction = -noktadan_giden ;
			vec3 finalpoint = direction ;

			float garipsay = sqrt(finalpoint.x * finalpoint.x +finalpoint.z * finalpoint.z );
			float theta = atan(finalpoint.y/garipsay );



			float omega = atan(finalpoint.z,finalpoint.x);


			float pi = 3.1416f;
			float finalI = ((omega+pi)/(2*pi)) ;
			float finalJ = ((pi/2+theta)/(pi)) ;
			vec2 sonnn = vec2(finalI,finalJ);
			return sonnn ;

} 



void main(void)
{
	if(uifwater == 0){
	if(fHeight < -30.0f){
		discard;
	}
	float luminance;
	uint ifhdr = uHDR;
	switch(ifhdr){
	case 1:{

		vec3 comingvec = -normalize(fDistance);
		vec3 halfvec = normalize(vec3(fCam.x+comingvec.x,fCam.y+comingvec.y,fCam.z+comingvec.z));
		
		float heightRatio = clamp(fHeight / umaxheight,0.0,1.0);
		float diffuse = 0.5*max(0.0,dot(fNormal,comingvec));

		vec4 colLowMid  = vec4(0.02, 0.25, 0.16, 1.0);
		vec4 colBottom  = vec4(0.66, 1.00, 0.00, 1.0);
		vec4 colpeak = vec4(0.8, 0.8, 0.8, 1.0);
		vec4 colupmid    = vec4(0.58, 0.29, 0.00, 1.0);
		vec4 tercolor;
		vec4 reflectness;

		vec4 tex1 = texture2D(gDiff, fUV) ;
		vec4 tex2 = texture2D(gRough, fUV) ;
		vec4 tex3 = texture2D(rDiff, fUV);
		vec4 tex4 = texture2D(rRough, fUV);
		vec4 tex5 = texture2D(sH_diff, fUV);
		vec4 tex6 = texture2D(sH_rough, fUV);
		vec4 tex7 = texture2D(sN_diff, fUV);
		vec4 tex8 = texture2D(sN_rough, fUV);


		if(fHeight <0.01){
		reflectness = vec4(1.0f,1.0f,1.0f,1.0f);
		tercolor = vec4(0.1f ,0.7f ,0.9f , 1.0f);
		}
		else if(heightRatio < 0.20) { 
		float blend = smoothstep(0.1,0.2,heightRatio);
        tercolor = mix(tex5, tex1, blend);
		
		reflectness = mix(tex6, tex2, blend);
		}else if(heightRatio < 0.66) {
        float blend =  smoothstep(0.4,0.66,heightRatio);
		reflectness = mix(tex2, tex4, blend);
        tercolor = mix(tex1, tex3, blend);
		}else{
        float blend =  smoothstep(0.66,0.75,heightRatio);
        tercolor = mix(tex3, tex7, blend);
		reflectness = mix(tex5, tex8, blend);
		}
	float specalpha = mix(20,2,reflectness.r)	;
	if(fHeight <0.01){
		specalpha = 25.0f;
	}	
	float directlight = pow(max(0.0,dot(fNormal,halfvec)),specalpha);	
	texcolor = tercolor*diffuse + vec4(1.0, 1.0, 1.0, 0.0)*directlight*tercolor + tercolor*0.05 ;

	luminance = dot(texcolor.xyz,vec3(0.2126f,0.7152f,0.0722));
	texcolor.w= log(luminance+0.001);

    break;

	}
	default:{
	uint mode = uMode;
	switch(mode)
	{
		// Pure Red
		case 0:{
		vec3 comingvec = -normalize(fDistance);
		vec3 halfvec = normalize(vec3(fCam.x+comingvec.x,fCam.y+comingvec.y,fCam.z+comingvec.z));
		float directlight = pow(max(0.0,dot(fNormal,halfvec)),12);
		float heightRatio = clamp(fHeight / umaxheight,0.0,1.0);
		float diffuse = 0.5*max(0.0,dot(fNormal,comingvec));

		vec4 colLowMid  = vec4(0.02, 0.25, 0.16, 1.0);
		vec4 colBottom  = vec4(0.66, 1.00, 0.00, 1.0);
		vec4 colpeak = vec4(0.8, 0.8, 0.8, 1.0);
		vec4 colupmid    = vec4(0.58, 0.29, 0.00, 1.0);
		vec4 tercolor;
		if(fHeight <0.01){
		tercolor = vec4(0.1f ,0.7f ,0.9f , 1.0f);
		}
		else if(heightRatio < 0.25) { 
        float blend = heightRatio / 0.25;  // Blendleyince daha güzel oluyo
        tercolor = mix(colBottom, colLowMid, blend);
		}else if (heightRatio < 0.6) {
        float blend = (heightRatio - 0.25) / 0.35; 
        tercolor = mix(colLowMid, colupmid, blend);
		}else if (heightRatio < 0.85) {
        float blend = (heightRatio - 0.6) / 0.25; 
        tercolor = mix(colupmid, colpeak, blend);
		} else {
        float blend = (heightRatio - 0.85) / 0.15;
        vec4 colwhite = vec4(1.0, 1.0, 1.0, 1.0);
        tercolor = mix(colpeak, colwhite, blend); 
    }
	fboColor = tercolor*diffuse + vec4(1.0, 1.0, 1.0, 0.0) * directlight*tercolor + tercolor*0.2 ;
    break;
}


		case 1: fboColor = vec4((fNormal + 1) * 0.5, 1); break;
		// UV
		// bURDA isik yok
		case 2:{
		float heightRatio = clamp(fHeight / umaxheight,0.0,1.0);
		vec4 colLowMid  = vec4(0.02, 0.25, 0.16, 1.0);
		vec4 colBottom  = vec4(0.66, 1.00, 0.00, 1.0);
		vec4 colpeak = vec4(0.8, 0.8, 0.8, 1.0);
		vec4 colupmid    = vec4(0.58, 0.29, 0.00, 1.0);
		vec4 tercolor;
		if(fHeight <0.01){
		tercolor = vec4(0.1f ,0.7f ,0.9f , 1.0f);
		}
		else if (heightRatio < 0.25) { 
        float blend = heightRatio / 0.25;  // Blendleyince daha güzel oluyo
        tercolor = mix(colBottom, colLowMid, blend);
		}else if (heightRatio < 0.6) {
        float blend = (heightRatio - 0.25) / 0.35; 
        tercolor = mix(colLowMid, colupmid, blend);
		}else if (heightRatio < 0.85) {
        float blend = (heightRatio - 0.6) / 0.25; 
        tercolor = mix(colupmid, colpeak, blend);
		} else {
        float blend = (heightRatio - 0.85) / 0.15;
        vec4 colwhite = vec4(1.0, 1.0, 1.0, 1.0);
        tercolor = mix(colpeak, colwhite, blend); 
    }
	fboColor = tercolor  ;
    break;
	case 3:{


		vec3 comingvec = -normalize(fDistance);
		vec3 halfvec = normalize(vec3(fCam.x+comingvec.x,fCam.y+comingvec.y,fCam.z+comingvec.z));
		float directlight = pow(max(0.0,dot(fNormal,halfvec)),17);
		float heightRatio = clamp(fHeight / umaxheight,0.0,1.0);
		float diffuse = 0.5*max(0.0,dot(fNormal,comingvec));

		vec4 colLowMid  = vec4(0.02, 0.25, 0.16, 1.0);
		vec4 colBottom  = vec4(0.66, 1.00, 0.00, 1.0);
		vec4 colpeak = vec4(0.8, 0.8, 0.8, 1.0);
		vec4 colupmid    = vec4(0.58, 0.29, 0.00, 1.0);
		vec4 tercolor;
		vec4 reflectness;

		vec4 tex1 = texture2D(gDiff, fUV) ;
		vec4 tex2 = texture2D(gRough, fUV) ;
		vec4 tex3 = texture2D(rDiff, fUV);
		vec4 tex4 = texture2D(rRough, fUV);
		vec4 tex5 = texture2D(sH_diff, fUV);
		vec4 tex6 = texture2D(sH_rough, fUV);
		vec4 tex7 = texture2D(sN_diff, fUV);
		vec4 tex8 = texture2D(sN_rough, fUV);

		if(fHeight <0.01){
		tercolor = vec4(0.1f ,0.7f ,0.9f , 1.0f);
		}
		else if(heightRatio < 0.25) { 
        float blend = heightRatio / 0.25;  // Blendleyince daha güzel oluyo
        tercolor = mix(tex1, tex3, blend);
		reflectness = mix(tex2, tex4, blend);
		}else if (heightRatio < 0.6) {
        float blend = (heightRatio - 0.25) / 0.35; 
		reflectness = mix(tex4, tex6, blend);
        tercolor = mix(tex3, tex5, blend);
		}else if (heightRatio < 0.85) {
        float blend = (heightRatio - 0.6) / 0.25; 
        tercolor = mix(tex5, tex7, blend);
		reflectness = mix(tex6, tex8, blend);
		} else {
        float blend = (heightRatio - 0.85) / 0.15;
        vec4 colwhite = vec4(1.0, 1.0, 1.0, 1.0);
        tercolor = mix(tex8, colwhite, blend); 

		
    }
	fboColor = reflectness*tercolor*diffuse + reflectness*vec4(1.0, 1.0, 1.0, 0.0) * directlight*tercolor + tercolor*0.2 ;
    break;








	
	}



		}
		// If mode is wrong, put pure white.
		default: fboColor = vec4(1); break;
	}
	break ;
	}
	}
	}else{
	vec4 tercolor;
	vec4 reflectness;
	float luminance;

	reflectness = vec4(1.0f,1.0f,1.0f,1.0f);
	tercolor = vec4(0.1f ,0.7f ,0.9f , 1.0f);
	float specalpha = 75.0f;



	vec3 comingvec = -normalize(fDistance);
	vec3 halfvec = normalize(vec3(fCam.x+comingvec.x,fCam.y+comingvec.y,fCam.z+comingvec.z));
		
	float diffuse = 0.5*max(0.0,dot(fNormal,comingvec));

	float directlight = pow(max(0.0,dot(fNormal,halfvec)),specalpha);

	texcolor = tercolor*diffuse + 5*vec4(1.0, 1.0, 1.0, 0.0)*directlight*tercolor + tercolor*0.05 ;


	vec2 nfUV ;

	if(texno != 4){
	nfUV = calculate_texture_position(vec3(0.0,0.0,0.0),fCam.xyz,fNormal); // 4se zaten cubemapten bakıyoruz boşuna işleme gerek yok.  
	}
	vec4 texcolor1;

					switch(texno){
				case 0:{
					texcolor1 = texture2D(Tsunset , nfUV );
					break;
				}
				case 1:{
					texcolor1 = texture2D(Tsky , nfUV );
					break;
				}
				case 2:{
					texcolor1 = texture2D(Tchurch , nfUV );
					break;
				}
				case 3:{
					vec4 col1 = texture2D(Tsunset , nfUV );
					vec4 col2 = texture2D(Tsky , nfUV );
					texcolor1 = mix(col1 , col2 , 0.5f );
					break;
				}
				case 4: {

					vec3 nfUV1 ;

					nfUV1 = calculate_3d_vec(vec3(0.0,0.0,0.0),fCam.xyz,fNormal);
					vec4 intermediatervec = texture(Tmorning  , normalize(nfUV1));  
					texcolor1 = vec4(intermediatervec.x , intermediatervec.y , intermediatervec.z , intermediatervec.w);
					break ;   
				}
				default:{
					texcolor1 = texture2D(Tsunset , nfUV );
					break;
				}

			}
			texcolor = texcolor1*0.35 + texcolor*0.65;
			luminance = dot(texcolor.xyz,vec3(0.2126f,0.7152f,0.0722));
			texcolor.w= log(luminance+0.001);
	}
}
