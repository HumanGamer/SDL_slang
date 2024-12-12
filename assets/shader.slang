Texture2D tex;
SamplerState smp;


[shader("vertex")]
float4 vertexMain(float4 position : POSITION) : SV_POSITION
{
    return position;
}