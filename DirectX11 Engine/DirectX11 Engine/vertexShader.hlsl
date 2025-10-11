cbuffer mycBuffer : register(b0)
{
    row_major float4x4 mat;
};


struct VS_INPUT
{
    float3 inPos : POSITION;
    float2 inTexCoord : TEXCOORD;
    
    float4 instanceMatRow0 : INSTANCE_MAT0;
    float4 instanceMatRow1 : INSTANCE_MAT1;
    float4 instanceMatRow2 : INSTANCE_MAT2;
    float4 instanceMatRow3 : INSTANCE_MAT3;
};


struct VS_OUTPUT
{
    float4 outPosition : SV_POSITION;
    float2 outTexCoord : TEXCOORD;
};


VS_OUTPUT main(VS_INPUT input)
{
    //VS_OUTPUT output;
    //float4 worldPos = float4(input.inPos + input.instanceOffset, 1.0f);
    //output.outPosition = mul(worldPos, mat);
    //output.outTexCoord = input.inTexCoord;
    //return output;
    
    VS_OUTPUT output;
    
    float4x4 instanceWorldMat = float4x4(
        input.instanceMatRow0,
        input.instanceMatRow1,
        input.instanceMatRow2,
        input.instanceMatRow3
    );
    
    float4 worldPos = mul(float4(input.inPos, 1.0f), instanceWorldMat);
    
    output.outPosition = mul(worldPos, mat);
    output.outTexCoord = input.inTexCoord;
    return output;
}
