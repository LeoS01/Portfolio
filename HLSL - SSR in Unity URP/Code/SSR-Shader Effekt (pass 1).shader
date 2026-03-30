Shader "Hidden/URP_SSR"
{
    Properties
    {
        _MainTex ("Texture", 2D) = "white" {}
        _Intensity("Reflection Intensity", Range(0, 1)) = .4
        [Toggle] _Debug("Debug", float) = 0
        [Toggle] _edge("Use Edge Falloff", float) = 1

        _normalDST_T("Normal DST Threshhold", float) = .1
        _normalDST_TClose("Normal DST Threshhold close intensity", float) = 0
        _normalDST_TFar("Normal DST Threshhold far intensity", float) = 3
        _normalDST_Threshhold("Normal DST distance reference", float) = 30

        _UVDiffThreshhold("UV difference threshhold", float) = .01
        _StepSize("Step Size", float) = .1
        _NoiseInten("Noise Intensity", float) = 1
        _RotationInfluence("Rotation Influence", Range(0, 1)) = 0
        _RotDeg("Rotation Degrees", Range(0, 180)) = 0
        _DotSafe("Dot safe threshhold", Range(-1, 1)) = -.75

        _distantLength("Distant length influence", float) = .5

        _distantLengthClose("Distant length close", float) = 0
        _distantLengthFar("Distant length far", float) = 2
        _distantLengthRef("Distant length ref", float) = 30


        _biAMT("Bi-searched accuracy falloff", Range(0.01, 0.99)) = .5
    }
    SubShader
    {
        // V SSR V
        // If metallic values are very low, no reflection is calculated.
        // First a ray is sent from the world-position to the reflected viewDirection(based on frame-normal in world-space)
        // The Raymarching searches for the UV-Coordinates of the hit.
        // If no Hit is found, the default Input UV is returned - resulting in no visible reflection.
        // If a hit is registered, we step through the ray on a few smaller iterations to get more accurate results
        // The final UV is then returned and later used to sample the Frame as a reflection.
        // No culling or depth

        Cull Off ZWrite Off ZTest Always

        Pass
        {
            HLSLPROGRAM
            #pragma vertex vert
            #pragma fragment frag

            //v utility and helper files
            #include "Assets/URPGbufferUtils.hlsl"
            #include "Assets/Helper.hlsl"
            #include "Assets/URPUtils.hlsl"

            //v define some values
            #define STEPS 40
            #define BISTEPS 5

            //v uniforms & Maintex
            uniform real3 _camFWD;
            sampler2D _MainTex;
            uniform real4 _MainTex_TexelSize;

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


            real _StepSize;
            real _normalDST_T;
            real _UVDiffThreshhold;
            real _NoiseInten;

            real _biAMT;

            real _normalDST_TClose;
            real _normalDST_TFar;
            real _normalDST_Threshhold;

            //v additional search ray for hit to get more accurate UV result
            real2 BiSearchedUV(real3 startPos, real2 uvInput, real2 initialUV, real3 hitPos, real3 hitDir){

                //v base variables
                real3 iterativePosition = hitPos;
                real amount = .5f;
                real dir = -1;
                real2 biUV = uvInput;
                real thisActualDST = 0;
                real thisDST = 0;

                //v threshhold savers
                real lastHitDST, lastActualDST;

                for(int i = 0; i < BISTEPS; i++){
                    //v iterative
                    iterativePosition += hitDir * dir * amount * _StepSize;

                    //v hit data
                    thisDST = length(iterativePosition - _WorldSpaceCameraPos);
                    real2 thisUV = WorldToScreenUV(iterativePosition);
                    thisUV = lerp(real2(1 - thisUV.x, 1 - thisUV.y), thisUV, unity_OrthoParams.w);      //<- uv's are inverted in perspective (?)
                    thisActualDST = length(WorldPos(thisUV) - _WorldSpaceCameraPos);

                    //v NO hit? (we know guaranteed that input gives hit!)
                    if(thisDST > thisActualDST){
                        biUV = thisUV;
                        continue;
                    }
                    else{
                        //HIT       
                        //less amount and invert direction     
                        amount *= _biAMT;
                        dir *= -1;

                        //V and save other relevant data
                        lastActualDST = thisDST;
                        lastHitDST = thisActualDST;
                        hitPos = iterativePosition;
                    }

                }

                //If the ray position is very far away from the actual position, maybe its just far behind an object 
                real normalThreshhold = lerp(0, _normalDST_TFar, (length(startPos - hitPos) * _normalDST_TClose) / _normalDST_Threshhold);
                if(lastActualDST - lastHitDST < _normalDST_T + normalThreshhold) return initialUV;

                //v otherwise its a successfull reflection
                return biUV;
            }

            real _distantLength;
            real _distantLengthClose;
            real _distantLengthFar;
            real _distantLengthRef;
            real _edge;

            //v this is the Main SSR Method were we raymarch to return the hit-uv coordinates
            real2 RayMarchedUV(real3 startPos, real3 dir, real2 ogUV, real wsLength, out real isReflected){
                real dstIntensity = lerp(_distantLengthClose, _distantLengthFar, wsLength / _distantLengthRef);
                real3 nDir = dir * wsLength * (_distantLength * dstIntensity);
                dir = nDir;

                real3 pos = startPos + (NormalOrientedHemisphere(ogUV, normalize(dir), 1) * random01(startPos.xy + startPos.xz)) * _NoiseInten;
                real2 uv = ogUV;   
                isReflected = 0;

                //raymarch
                for(int i = 0; i < STEPS; i++){
                    //v iterate over position
                    pos += dir * _StepSize;

                    //v calculate relevant data
                    real thisDST = length(pos - _WorldSpaceCameraPos);
                    real2 thisUV = WorldToScreenUV(pos);
                    thisUV = lerp(real2(1 - thisUV.x, 1 - thisUV.y), thisUV, unity_OrthoParams.w);      //<- uv's are inverted in perspective (?)
                    real thisActualDST = length(WorldPos(thisUV) - _WorldSpaceCameraPos);

                    //v check for hit
                    if((thisDST > thisActualDST)){

                        //v check if the new hit is still in the bounds of the frame
                        if(thisUV.x <= 1.0f && thisUV.y <= 1.0f && thisUV.x > 0 && thisUV.y > 0){

                            //v check for uv threshhold to avoid self-reflections
                            if(length(thisUV - ogUV) < _UVDiffThreshhold) continue;

                            //v same calculation as in BiSearchedUV - we want to avoid artifacts with this!
                            real normalThreshhold = lerp(0, _normalDST_TFar, (length(startPos - pos) * _normalDST_TClose) / _normalDST_Threshhold);
                            if(thisActualDST - thisDST < _normalDST_T + normalThreshhold) continue;;

                            //HIT
                            uv = BiSearchedUV(startPos, thisUV, ogUV, pos, dir);
                            if(all(uv != ogUV))isReflected = 1;
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
            real _DotSafe;
            real _Debug;
            
            real4 frag (v2f i) : SV_Target
            {
                real metallicNess = SampleGBufferMetallicness(i.uv);
                real4 col = tex2D(_MainTex, i.uv);

                //v if metallicness is low we can save some performance by returning now
                if(metallicNess <= 0.1f){
                    return col;
                }
                else{     
                    //v gather all relevant data   
                    real frameSmoothness = SampleGBufferSmoothness(i.uv);
                    real3 wsp = WorldPos(i.uv);
                    real3 wsn = SampleGBufferNormals(i.uv);
                    real3 orthoDir = normalize(reflect(_camFWD, wsn));

                    real3 wsdir = wsp - _WorldSpaceCameraPos;
                    real wsLength = length(wsdir);
                    wsdir = lerp(normalize(wsdir), _camFWD, unity_OrthoParams.w);

                    real3 perspectiveDir = normalize(reflect(wsn, normalize(_WorldSpaceCameraPos - wsp)));
                    perspectiveDir.y = 1 - perspectiveDir.y;


                    //v calculate directions
                    real3 finalDir = lerp(perspectiveDir, orthoDir, unity_OrthoParams.w);
                    real3 savedLastDir = finalDir;
                    finalDir = lerp(finalDir, NormalOrientedHemisphere(i.uv, _RotDeg, finalDir), (1 - frameSmoothness) * _RotationInfluence);
                    finalDir = lerp(finalDir, savedLastDir, frameSmoothness);
                    
                    //v direction faces camera so this can be ignored- the ray will never hit anything visible
                    real dirDot = dot(wsdir, finalDir);
                    if(dirDot < _DotSafe) return col;
                
                    //v execute raymarching
                    real didHit;                                                                        //<- we save if hit in the alpha channel for later composition
                    real2 rmUV = RayMarchedUV(wsp, finalDir, real2(i.uv.x, i.uv.y), wsLength, didHit);           
                    real4 reflection = lerp(col, tex2D(_MainTex, rmUV), _Intensity);

                    //v we want to fade out based on hit distance (close to screen-edge = more fade)
                    real edgeFalloff = (1 - length(rmUV - real2(.5f, .5f)) * 2);
                    edgeFalloff = lerp(1, edgeFalloff, _edge);

                    real4 result = lerp(lerp(col, real4(0, 0, 0, 0), _Debug), reflection, didHit * edgeFalloff);       
                    saturate(result);
                    return real4(result.xyz, didHit);       //<- hit in alpha channel to avoid sampling blurred pixels later in composition
                }
            }
            ENDHLSL
        }
    }
}
