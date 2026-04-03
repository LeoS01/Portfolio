using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Linq;

namespace TerrainGeneration{
    [RequireComponent(typeof(MeshFilter)), RequireComponent(typeof(MeshRenderer))]
    //generate a mesh
    //for now, this class ignores all vertex data for custom implementation except Positions & triangles
    public abstract class Procedural_Mesh : MonoBehaviour
    {
        [SerializeField] private bool flipNormals = true;               //<- sometimes our meshes normals might be flipped, so this can prove usefull
        protected Mesh mesh;                                            //<- variable to store our mesh into
        protected Vector3[] vertices;                                   //<- array to store our vertex data to
        protected int[] triangles;                                      //<- array to store our triangle-indices to

        [ContextMenu("Debug/RegenerateMesh")]
        public void Generate(){
            //v initialize our Mesh
            mesh = new Mesh();

            //v generate vertice and triangle data
            vertices = GenerateVertices();
            triangles = GenerateTriangles();

            //v set new data
            mesh.vertices = vertices;
            mesh.triangles = triangles;

            //https://answers.unity.com/questions/476810/flip-a-mesh-inside-out.html
            if(flipNormals) mesh.triangles = mesh.triangles.Reverse().ToArray();
            
            //v we can automatically recalculate our normals to make sure that everything is allright
            mesh.RecalculateNormals();    

            //v assign
            GetComponent<MeshFilter>().mesh = mesh;

            //v we execute this void for additional custom implementations
            OnMeshGenerated();
        }

        protected abstract Vector3[] GenerateVertices();          //<- generate how vertices are handles
        protected abstract int[] GenerateTriangles();            //<- generate how 
        protected abstract void OnMeshGenerated();              //<- additional Method for custom commands
    }

}
