Shader "Hidden/URPSSR2"
{
    Properties
    {
        _MainTex ("Texture", 2D) = "white" {}
        _Intensity("Reflection Intensity", float) = .4
        _normalDST_T("Normal DST Threshhold", float) = .1
        _DepthThreshhold("Depth Hit Threshhold", float) = .1
        _UVDiffThreshhold("Uv difference threshhold", float) = .01
        _StepSize("Step Size", float) = .1
        _NoiseInten("Noise Intensity", float) = 1
        _RotationInfluence("Rotation Influence", Range(0, 1)) = 0
        _RotDeg("Rotation Degrees", Range(0, 180)) = 0
    }
    SubShader
    {
        // No culling or depth
        Cull Off ZWrite Off ZTest Always

        Pass
        {
            HLSLPROGRAM
            #pragma vertex vert
            #pragma fragment frag

            //v utility and helper files
            #include "Assets/URPGbufferUtils.hlsl"
            #include "Assets/URPUtils.hlsl"
            sampler2D _MainTex;
            uniform real4 _MainTex_TexelSize;

            //v define some values
            #define E 2.71828182846
            #define STEPS 40

            //v uniforms & Maintex
            uniform real3 _camFWD;

            //v default vertex shader
            struct appdata
            {
                real2 screenPos : TEXCOORD1;
                real4 vertex : POSITION;
                real2 uv : TEXCOORD0;
            };

            struct v2f
            {
                real2 screenPos : TEXCOORD1;
                real2 uv : TEXCOORD0;
                real4 vertex : SV_POSITION;
            };

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = TransformObjectToHClip(v.vertex);
                o.screenPos = ComputeScreenPos(mul(UNITY_MATRIX_MVP, v.vertex));
                o.uv = v.uv;
                return o;
            }
            //^ vertex shader end
            real2 WorldToScreenUV(real3 wsp){
                real4 projected = mul(_WorldToSreenUVMatrix, real4(wsp, 1.0f));
                real2 uv = (projected.xy / projected.w) * .5f + .5f;
                return uv;
            }

            real _StepSize;
            real _normalDST_T;
            real _DepthThreshhold;
            real _UVDiffThreshhold;
            real _NoiseInten;
            
            real2 RayMarchedUV(real3 startPos, real3 dir, real2 ogUV, out real isReflected){
                //v we want to slightly offset the ray based on a random value
                real3 pos = startPos + (dir * lerp(-1, 1, random01(ogUV)) * _NoiseInten);
                real2 uv = ogUV;   
                isReflected = 0;

                //raymarch
                for(int i = 0; i < STEPS; i++){
                    //v iterate over position
                    pos += normalize(dir) * _StepSize;

                    //v calculate relevant data
                    real thisDST = length(pos - _WorldSpaceCameraPos);
                    real2 thisUV = WorldToScreenUV(pos);
                    real thisActualDST = length(WorldPos(thisUV) - _WorldSpaceCameraPos);

                    //v check for hit
                    if((thisDST - thisActualDST) > _DepthThreshhold){

                        //If the ray position is very far away from the actual position, maybe it s just far behind an object 
                        if(thisDST - thisActualDST > _normalDST_T) continue;

                        //v check if the new hit is still in the bounds of the frame
                        if(thisUV.x <= 1.0f && thisUV.y <= 1.0f && thisUV.x > 0 && thisUV.y > 0){
                            //v check for uv threshhold to avoid self-reflections
                            if(length(thisUV - ogUV) >= _UVDiffThreshhold) continue;

                            //HIT
                            isReflected = 1;
                            uv = real2(thisUV.x, thisUV.y);
                            break;
                        } 
                    }
                }

                //v return either hit position or initial uv
                return uv;
            }

            real _Intensity;
            real _RotationInfluence;
            real _RotDeg;
            real4 frag (v2f i) : SV_Target
            {
                real metallicNess = SampleGBufferMetallicness(i.uv);

                //v if metallicness is low we can save some performance by returning in here
                if(metallicNess <= 0.1f){
                    return tex2D(_MainTex, i.uv);
                }
                else{     
                    //v gather all relevant data   
                    real frameSmoothness = SampleGBufferSmoothness(i.uv);
                    real3 wsp = WorldPos(i.uv);
                    real3 wsn = SampleGBufferNormals(i.uv);
                    real3 orthoDir = normalize(reflect(_camFWD, wsn));

                    //v calculate directions
                    real3 savedLastDir = orthoDir;
                    orthoDir = lerp(orthoDir, RandomizeDirection(i.uv, _RotDeg, orthoDir), _RotationInfluence + ((1 - frameSmoothness) * _RotationInfluence));
                    orthoDir = lerp(orthoDir, savedLastDir, frameSmoothness);
                
                    //v execute raymarching
                    real didHit;                                                            //<- we save if hit in the alpha channel for later composition
                    real2 rmUV = RayMarchedUV(wsp, orthoDir, real2(i.uv.x, i.uv.y), didHit);
                    real4 reflection = tex2D(_MainTex, rmUV);

                    //v based on the hit we want to decide for the final result
                    real4 result = lerp(tex2D(_MainTex, i.uv), reflection * _Intensity, didHit);
                    saturate(result);
                    return real4(result.xyz, didHit);                                       //<- hit is stored in alpha channel to avoid sampling the blurred pixel
                }
            }
            ENDHLSL
        }
    }
}
