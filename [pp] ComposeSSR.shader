Shader "Unlit/ComposeSSR"
{
    Properties
    {
        [HideInInspector]_MainTex ("Texture", 2D) = "white" {}
        _Intensity("Final Intensity", Range(0, 1)) = 1
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

            #include "Assets/URPGbufferUtils.hlsl"

            struct appdata
            {
                real4 vertex : POSITION;
                real2 uv : TEXCOORD0;
            };

            struct v2f
            {
                real2 uv : TEXCOORD0;
                real4 vertex : SV_POSITION;
            };

            sampler2D _MainTex;
            real4 _MainTex_ST;

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = TransformObjectToHClip(v.vertex);
                o.uv = TRANSFORM_TEX(v.uv, _MainTex);
                return o;
            }

            uniform real _ssrKey;
            uniform sampler2D _ReflectionSSR;
            real _Intensity;
            real4 frag (v2f i) : SV_Target
            {
                real4 col = tex2D(_MainTex, i.uv);
                real4 reflection = tex2D(_ReflectionSSR, i.uv);
                real t = SampleGBufferMetallicness(i.uv);
                t = saturate(t);
                t -= 0.05f;
                real4 refProduct = reflection;
                refProduct = lerp(refProduct, refProduct * real4(SampleGBufferAlbedo(i.uv), 1.0f), t);

                real4 final = lerp(col, refProduct, t * _Intensity * reflection.a);
                //final = lerp(col, final, step(i.uv.x, _ssrKey));
                return real4(final.xyz, 1.0f);
            }
            ENDHLSL
        }
    }
}
