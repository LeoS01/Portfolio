using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace TerrainGeneration{
    //This is a base class for procedural terrain
    public abstract class ProceduralTerrain : Procedural_Mesh
    {
        [SerializeField] protected Vector2Int dimensions;               //<- how many vertices in X|Y?
        [SerializeField] protected Gradient colorGradient;              //<- color gradient used for vertex-colors
        [SerializeField] protected float colorSweetSpot = .5f;          //<- multiplier for improved color control
        [SerializeField] private bool applyVertexColors = true;         //<- turn off vertex-colors if we want to
        protected List<Color> vertexColors = new List<Color>();         //<- a list to store all vertex colors in to

        //v here we generate our meshes Vertices
        protected override Vector3[] GenerateVertices()
        {
            //v we create a list
            List<Vector3> vertices = new List<Vector3>();

            //v we loop through every vertex like a grid
            for (int x = 0; x <= dimensions.x; x++)
            {
                for (int y = 0; y <= dimensions.y; y++)
                {
                    //v generate and add our position
                    Vector3 position = OnTerrainVertexGenerated(x, y);
                    vertices.Add(position);

                    //v and our color
                    vertexColors.Add(OnTerrainVertexColor(x, y, position));
                }
            }

            //v and then return our vertices for the mesh
            return vertices.ToArray();
        }

        //v here we generate our trinagles
        protected override int[] GenerateTriangles()
        {
            //v we know that every quad has 2 triangles with 3 vertex-incides. So every quad has 6 overall indices
            int triAmount = dimensions.x * dimensions.y * 6;

            //v tracker-values
            int currentVertexIndex = 0;
            int currentTriIndex = 0;

            //v this is the array we will return from here
            int[] triangles = new int[triAmount];

            //v we loop through the entire grid
            for (int i = 0; i < dimensions.x; i++)
            {
                //v and apply our triangle-formula
                for (int n = 0; n < dimensions.y; n++)
                {
                    //v handle lower triangle
                    triangles[currentTriIndex] = currentVertexIndex;                                //0
                    triangles[currentTriIndex + 1] = currentVertexIndex + dimensions.y + 1;         //1
                    triangles[currentTriIndex + 2] = currentVertexIndex + 1;                        //2

                    //v handle upper triangle
                    triangles[currentTriIndex + 3] = currentVertexIndex + 1;                        //n
                    triangles[currentTriIndex + 4] = currentVertexIndex + dimensions.y + 1;         //n+r+1
                    triangles[currentTriIndex + 5] = currentVertexIndex + dimensions.y + 2;         //n+r+2

                    //v and then increase our counter by the appropriate values
                    currentVertexIndex++;
                    currentTriIndex += 6;   //<- 2 triangles per quad, 1 triangle has 3 indices, 2 * 3 = 6
                }
                currentVertexIndex++;
            }

            //v and finally return all triangles
            return triangles;
        }

        protected override void OnMeshGenerated()
        {
            //v application of vertex Colors after the mesh has been generated
            if(applyVertexColors) mesh.SetColors(vertexColors);
        }
        
        protected abstract Vector3 OnTerrainVertexGenerated(int x, int y);
        protected abstract Color OnTerrainVertexColor(int x, int y, Vector3 position);

    }

}
