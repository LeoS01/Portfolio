using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

//v a denoised pass takes in 3 shaders through materials:
//1.) The initial effect
//2.) The blur Pass
//3.) The Composition Pass
public class DenoisedRenderingFeature : ScriptableRendererFeature
{   
#region UTILS
    //V simple enum to track the current Stage
    public enum DebugStage{
        FULL = 0,
        ORIGINAL = 1,
        EFFECT = 2,
        BLUR = 3,
    }

    //v enum to decide for the downsamplung
    public enum DownSample{
        //v the value equals to the downscale-percentage
        DISABLED = 0,
        QUARTER = 25,
        HALF = 50,
        TRHEEQUARTER = 75,
    }

    [System.Serializable]
    //simple struct to hold all relevant data
    public struct DenoiseData{
        public Material effect, blur, composition;
        public DownSample downsample;
        public DebugStage stage;
        public string globalTextureName;
        public int idOffset;
        public bool additionalBlurIteration;
    }
#endregion

    //v use this to denoise your effects
    class DenoisedFeature : ScriptableRenderPass
    {
        //v initialization 
        public DenoisedFeature(DenoiseData data){
            this.data = data;
        }
        private DenoiseData data;
        public void Init(ScriptableRenderer renderer){
            rendererReference = renderer;

            cbID = renderer.cameraColorTarget;
        }

        //v other variables
        private ScriptableRenderer rendererReference;
        private RenderTargetIdentifier cbID;                    //<- color buffer Identifier for faster access
        private RenderTargetHandle rbTEMP, rbTEMP2, rbTEMP3;    //<- intermediate textures

        //v intermediate textures
        public override void OnCameraSetup(CommandBuffer cmd, ref RenderingData renderingData)
        {
            //v init
            RenderTextureDescriptor descriptor = renderingData.cameraData.cameraTargetDescriptor;
            descriptor.colorFormat = RenderTextureFormat.ARGB32;

            //set ids
            int n = data.idOffset * 3;
            rbTEMP.id = 0 + n;
            rbTEMP2.id = 1 + n;
            rbTEMP3.id = 2 + n;
            
            //downsample
            float amount = 100 - (int)data.downsample;
            amount *= .01f;

            descriptor.width = Mathf.RoundToInt(descriptor.width * amount);
            descriptor.height = Mathf.RoundToInt(descriptor.height * amount);

            //set in cmd
            cmd.GetTemporaryRT(rbTEMP.id, descriptor, FilterMode.Bilinear);
            cmd.GetTemporaryRT(rbTEMP2.id, descriptor, FilterMode.Bilinear);
            cmd.GetTemporaryRT(rbTEMP3.id, renderingData.cameraData.cameraTargetDescriptor, FilterMode.Point);          //<- do not downsample to keep frame resolution

            //config
            ConfigureTarget(rbTEMP.Identifier());                   
            ConfigureTarget(rbTEMP2.Identifier());                   
            ConfigureTarget(rbTEMP3.Identifier());                  

            //clear
            ConfigureClear(ClearFlag.Color, Color.black);           
        }

        //v post process
        public override void Execute(ScriptableRenderContext context, ref RenderingData renderingData)
        {
            if(data.stage == DebugStage.ORIGINAL) return;

            //v get (references) to all important variables
            CommandBuffer cmd = CommandBufferPool.Get();
            cmd.Clear();

            //blits
            cmd.Blit(cbID, rbTEMP.Identifier(), data.effect);                   //< og - 1

            if(data.additionalBlurIteration){
                //v some specific cases for debug stages in combination with 2 pass blur are applied
                if(data.stage == DebugStage.EFFECT){
                    cmd.Blit(rbTEMP.Identifier(), cbID);
                    context.ExecuteCommandBuffer(cmd);
                    CommandBufferPool.Release(cmd);
                    return;
                } 

                //v blits
                cmd.Blit(rbTEMP.Identifier(), rbTEMP2.Identifier(), data.blur); //< 1 - 2
                cmd.Blit(rbTEMP2.Identifier(), rbTEMP.Identifier(), data.blur); //< 2 - 1  

                //v other early-return case
                if(data.stage == DebugStage.BLUR){
                    cmd.Blit(rbTEMP.Identifier(), cbID);
                    context.ExecuteCommandBuffer(cmd);
                    CommandBufferPool.Release(cmd);
                    return;
                } 
                cmd.SetGlobalTexture(data.globalTextureName, rbTEMP.Identifier()); //<- (set texture for composition)
            }
            else{
                cmd.Blit(rbTEMP.Identifier(), rbTEMP2.Identifier(), data.blur);     //< 1 - 2

                cmd.SetGlobalTexture(data.globalTextureName, rbTEMP2.Identifier()); //(set texture for composition)
            }
            

            cmd.Blit(cbID, rbTEMP3.Identifier(), data.composition);             //original - 3

            //decide the actual frame output for debugging
            switch(data.stage){
                case DebugStage.FULL:
                    cmd.Blit(rbTEMP3.Identifier(), cbID);                       //< 3 - original
                break;
                
                case DebugStage.EFFECT:
                    cmd.Blit(rbTEMP.Identifier(), cbID);                        //< 1 - original
                break;

                case DebugStage.BLUR:
                    cmd.Blit(rbTEMP2.Identifier(), cbID);                       //< 2 - original
                break;
            }

            //V execute instructions
            context.ExecuteCommandBuffer(cmd);

            //v clean up
            CommandBufferPool.Release(cmd);
        }
    }

    private DenoisedFeature m_ScriptablePass;
    //v customizeable variables
    public DenoiseData denoiseData;
    public RenderPassEvent insertionPoint = RenderPassEvent.BeforeRenderingPostProcessing;

    public override void Create()
    {
        m_ScriptablePass = new DenoisedFeature(denoiseData);

        // Configures where the render pass should be injected.
        m_ScriptablePass.renderPassEvent = insertionPoint;
    }

    public override void AddRenderPasses(ScriptableRenderer renderer, ref RenderingData renderingData)
    {
        renderer.EnqueuePass(m_ScriptablePass);
        m_ScriptablePass.Init(renderer);   
    }
}


