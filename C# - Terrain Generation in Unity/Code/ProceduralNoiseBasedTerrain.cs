using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Dream.Manifest;

namespace TerrainGeneration{
    //This class generates a terrain based on Noise
    //(altough it is not bound to be based on perlin-noise)
    public class ProceduralNoiseBasedTerrain : ProceduralTerrain
    {
        //A terrain-noise struct to allow for easy noise controls
        [System.Serializable]
        public struct TerrainNoise{
            public UnityEngine.UI.RawImage debugRenderer;       //<- draw the Texture potentially to the UI
            public NoiseType type;                              //<- of which type is the noise?
            public float intensity;                             //<- how intense is it?
            public float size;                                  //<- how big is it?
            private Texture2D noiseTex;                         //<- private member to read & write to
            
            //v Set the noise texture
            public void SetNoiseTexture(Texture2D texture){
                noiseTex = texture;
                if(debugRenderer != null) debugRenderer.texture = texture;
            }

            //v read from the noise texutre
            public float ReadNoiseTexture(int x, int y){
                //v noise textures should have the same value throughout all channels in this implementation
                return noiseTex.GetPixel(x, y).r;
            }
        }
        
        [SerializeField] private bool scatterPrefabs = true;                                //<- do we want to scatter gameObjects on the terrain?
        [SerializeField, Range(0, 1)] private float occupiedPrefabPercentage = .1f;         //<- ^if so, what percentage of vertices carry a prefab?
        [SerializeField] private GameObject spawnPrefab;                                    //<- ^and which prefab should we place?
        [SerializeField, Range(0, 1)] private float maxPrefabScaleInfluence = .6f;          //<- ^with what percentage of randomized scale?
        [SerializeField] private TerrainNoise[] noises;                                     //<- array to allow for multiple layers
        [SerializeField] private float size = 1;                                            //<- base size of the mesh
        [SerializeField] private NoiseGenerator generator;                                  //<- dedicated noise generator
        [SerializeField] private MeshCollider collider;
        public static TileWorld[,] tiles;
        public static List<TileWorld> allTiles = new List<TileWorld>();
        public static Utility.ArgumentelessDelegate onGenerated;

        //v In Start of our base class the mesh is generated.
        //v In Awake we fill the nosie structs with data from our generator
        private void Awake(){
            if(collider == null) collider = GetComponent<MeshCollider>();
            for (int i = 0; i < noises.Length; i++)
            {
                noises[i].SetNoiseTexture(
                    generator.GenerateNoise(dimensions,         //<- generate noise texture
                        Random.Range(-900000.0f, 900000.0f),    //<- with large random offset
                        noises[i].size,                         //<- in size
                        noises[i].type)                         //<- of type
                );
            }
            tiles = new TileWorld[dimensions.x+1, dimensions.y+1];
        }

        //v we generate vertices based on our noise textures
        protected override Vector3 OnTerrainVertexGenerated(int x, int y)
        {
            //v position is in object space
            Vector3 position = Vector3.zero;

            //v ffset this vertex on the y and z axis based on index to generate a grid
            position += new Vector3(x, 0, y) * size;

            //v this will carry our noise-value
            float yPos = 0;                    

            //v we loop through all the noises
            for (int i = 0; i < noises.Length; i++)
            {
                //v and apply them as a value
                yPos += noises[i].ReadNoiseTexture(x, y) * noises[i].intensity * size;
            }

            //v and then apply them to the local position
            position.y = yPos;


            //GENERATE TILE
            tiles[x, y] = new TileWorld(TypeTile.EMPTY, position);
            allTiles.Add(tiles[x,y]);
            //END TILE GEN

            //v before returning it
            return position;
        }

        //v Color application
        protected override Color OnTerrainVertexColor(int x, int y, Vector3 position)
        {
            //v we just read from our gradient with the y-position as our input (with a multiplier for better control)
            return colorGradient.Evaluate(position.y * colorSweetSpot);
        }

        //v we want to add to OnMeshGenerated with our prefab-scattering potential
        protected override void OnMeshGenerated()
        {
            base.OnMeshGenerated();
            if(scatterPrefabs) ScatterPrefabs();
            if(collider != null) collider.sharedMesh = mesh;
            onGenerated?.Invoke();
        }

        //v prefab-placement
        private void ScatterPrefabs(){

            //v first we initialize a list with all potential vertex positions
            List<Vector3> positions = new List<Vector3>();
            positions.AddRange(mesh.vertices);

            //the we calculate the count of prefabs we want
            int count = Mathf.RoundToInt(positions.Count * occupiedPrefabPercentage);
            for (int i = 0; i < count; i++)
            {
                //v choose a position
                Vector3 position = positions[Random.Range(0, positions.Count)];
                
                //v spawning with random rotation, spawned in World-space
                if(spawnPrefab != null){
                    GameObject spawned = Instantiate(spawnPrefab, transform.TransformPoint(position), 
                    Quaternion.Euler(new Vector3(Random.Range(0, 360), Random.Range(0, 360), Random.Range(0, 360))));
                    spawned.transform.forward = transform.up;     //<- correcting alignment  

                    //v apply random scale
                    float scaleInfluence = Random.Range(1 - maxPrefabScaleInfluence, 1 + maxPrefabScaleInfluence);
                    spawned.transform.localScale = spawned.transform.localScale * scaleInfluence;

                    //v parent the gameObject to this transform
                    spawned.transform.parent = transform;
                }  



                //v dont spawn a gameObject twice at the same position
                positions.Remove(position);
            }
        }

        protected void OnDrawGizmos(){
            Gizmos.color = Color.red;
            Gizmos.DrawLine(transform.position, transform.position + new Vector3(dimensions.x * size, 0, 0));
            Gizmos.DrawLine(transform.position, transform.position + new Vector3(0, 0, dimensions.y * size));
        }

    }

}
