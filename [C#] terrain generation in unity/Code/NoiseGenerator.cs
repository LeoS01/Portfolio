using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public enum NoiseType{
    SIMPLE,
    PERLIN,
    VORONOI,
}

//v noise generator through a compute shader
public class NoiseGenerator : MonoBehaviour
{
    //v parralel struct in NoiseGeneration_CS.compute
    public struct NoiseData{
        public float value;
        public Vector2 uv;
        public float offset;
        public float scale;
    };
    [SerializeField] private ComputeShader noiseShader;

    //v Get Noise pixel at coordinates X&y
    public Texture2D GenerateNoise(Vector2Int resolution, float offset, float scale, NoiseType type){
        RenderTexture texture = new RenderTexture(Mathf.Abs(resolution.x), Mathf.Abs(resolution.y), 1);
        texture.filterMode = FilterMode.Point;
        texture.enableRandomWrite = true;
        texture.Create();

        int kernel = 0;
        switch(type){
            case NoiseType.SIMPLE:
                kernel = 0;
            break;

            case NoiseType.VORONOI:
                kernel = 1;
            break;

            case NoiseType.PERLIN:
                kernel = 2;
            break;
        }

        noiseShader.SetFloat("offset", offset);
        noiseShader.SetFloat("scale", scale);
        noiseShader.SetTexture(kernel, "noiseTex", texture);
        noiseShader.Dispatch(kernel, texture.width / 8, texture.height / 8, 1);
        return ToTex2D(texture);
    }

    //https://stackoverflow.com/questions/44264468/convert-rendertexture-to-texture2d
    Texture2D ToTex2D(RenderTexture rTex)
    {
        Texture2D tex = new Texture2D(rTex.width, rTex.height, TextureFormat.RGB24, false);
        // ReadPixels looks at the active RenderTexture.
        RenderTexture.active = rTex;
        tex.ReadPixels(new Rect(0, 0, rTex.width, rTex.height), 0, 0);
        tex.Apply();
        return tex;
    }
}
