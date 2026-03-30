### Description
Context: University.

The base-class 'Procedural-Mesh' initializes the generation of vertex-data (positions and indices).
'ProdecuralTerrain' places these vertices in a grid.
'ProceduralNoiseBasedTerrain' then modifies the Y-Positions based on a noise-value.

The noise-texture is read from the 'NoiseGenerator'.
The generation of noise-textures is done via the 'NoiseGenerator' compute-shader.