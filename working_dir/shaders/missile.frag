    #version 430


    #define IN_UV       layout(location = 0)
    #define IN_NORMAL   layout(location = 1)
    #define IN_DISTANCE layout(location = 4)
    #define IN_TOCAM    layout(location = 5)
    #define IN_WORLDPOS layout(location = 6)
    #define IN_CHECK layout(location =  7)


    #define OUT_FBO     layout(location = 0)
    #define OUT_TEX     layout(location = 1)

    #define U_HDR       layout(location = 24)

    in IN_UV vec2 fUV;
    in IN_NORMAL vec3 fNormal;
    in IN_DISTANCE vec3 fDistance;
    in IN_TOCAM vec3 fCam;
    in IN_WORLDPOS vec3 fWorldPos;
    in IN_CHECK float check ;  

    out OUT_FBO vec4 fboColor;
    out OUT_TEX vec4 texcolor;

    U_HDR uniform uint uHDR;


    layout(binding = 22) uniform sampler2D tAlbedo;
    layout(binding = 25) uniform sampler2D tRoughness;
    layout(binding = 23) uniform sampler2D tMetallic;
    layout(binding = 24) uniform sampler2D tNormalMap;
    layout(binding = 27) uniform samplerCube tEnvMap;




    void main(void) 
    {


        vec4 albedoColor = texture(tAlbedo, fUV);
        float roughness = texture(tRoughness, fUV).r;
        float metallic = texture(tMetallic, fUV).r;


        vec3 N = fNormal;  
        vec3 comingvec = -normalize(fDistance); 
        vec3 halfvec = normalize(fCam + comingvec);


        float diffuse = 0.5 * max(0.0, dot(N, comingvec));
        float specalpha = mix(200.0, 3.0, roughness); 
        float directlight = pow(max(0.0, dot(N, halfvec)), specalpha);


        vec3 R = reflect(-fCam, N);
        vec3 envColor = texture(tEnvMap, R).rgb;
        float reflectivity = (1.0 - roughness) * mix(0.2, 1.0, metallic);


        vec3 finalColor = albedoColor.rgb * diffuse 
                        + 2.0 * directlight * mix(vec3(0.9, 0.8, 0.7), albedoColor.rgb, metallic) 
                        + albedoColor.rgb * 0.05 
                        + envColor * reflectivity;


        if (albedoColor.g > 0.7 && albedoColor.r < 0.1) {
            finalColor += albedoColor.rgb * 1.5;
        }

        vec4 computedColor = vec4(finalColor, albedoColor.a);

    
        if (uHDR == 1) {
            texcolor = computedColor;
            
            float luminance = dot(texcolor.xyz, vec3(0.2126f, 0.7152f, 0.0722f));
            texcolor.w = log(luminance);
        } else {
            fboColor = computedColor;
        }
    }