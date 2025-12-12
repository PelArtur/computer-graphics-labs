cbuffer perObjectBuffer : register(b0)
{
    row_major float4x4 wvpMatrix;  //World View Projection matrix
    row_major float4x4 worldMatrix;
};


struct VS_INPUT
{
    float3 inPos      : POSITION;
    float2 inTexCoord : TEXCOORD;
    float3 inNormal   : NORMAL;
    
    float4 instanceMatRow0 : INSTANCE_MAT0;
    float4 instanceMatRow1 : INSTANCE_MAT1;
    float4 instanceMatRow2 : INSTANCE_MAT2;
    float4 instanceMatRow3 : INSTANCE_MAT3;
};


struct VS_OUTPUT
{
    float4 outPosition : SV_POSITION;
    float2 outTexCoord : TEXCOORD;
    float3 outNormal   : NORMAL;
    float3 outWorldPos : WORLD_POSITION;
};


VS_OUTPUT main(VS_INPUT input)
{   
    VS_OUTPUT output;
    
    float4x4 instanceWorldMat = float4x4(
        input.instanceMatRow0,
        input.instanceMatRow1,
        input.instanceMatRow2,
        input.instanceMatRow3
    );
    
    //float4 worldPos = mul(float4(input.inPos, 1.0f), instanceWorldMat);
    
    //output.outPosition = mul(worldPos, wvpMatrix);
    output.outPosition = mul(float4(input.inPos, 1.0f), wvpMatrix);
    output.outTexCoord = input.inTexCoord;
    output.outNormal   = normalize(mul(float4(input.inNormal, 0.0f), worldMatrix));
    output.outWorldPos = mul(float4(input.inPos, 1.0f), worldMatrix);
    return output;
}
