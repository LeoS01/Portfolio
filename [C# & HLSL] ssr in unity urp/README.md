### Description
Context: Personal project.

This Screen-Space-Reflections implementation is intended for Unity's universal-renderpipeline.
It works by raymarching through the viewspace via reflected normals~ thus being dependent on reading the depth- and g-buffer.

Overall, the effect has 3 passes:
1 - Reflections are collected via raymarching (readable in 'Code').
2 - A Gaussian-blur is run over the reflections, to reduce overall noise.
3 - The final result is composed.


### References
SSR Inspired after: https://www.youtube.com/watch?v=K2rs7K4y_sY
'Denoised Rendering Feature' inspired after: https://valeriomarty.medium.com/raymarched-volumetric-lighting-in-unity-urp-e7bc84d31604
Example-Textures from: https://ambientcg.com, https://www.textures.com/library