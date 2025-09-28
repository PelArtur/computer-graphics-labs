cbuffer PixelConstantBuffer : register(b1)
{
	float4 iResolution;
	float iTime;
	float3 iTime_padding;
	float4 iMouse;
};


struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};


float3 hash3( float2 p )
{
    float3 q = float3( dot(p,float2(127.1,311.7)), 
                      dot(p,float2(269.5,183.3)), 
                      dot(p,float2(419.2,371.9)) );
    return frac(sin(q)*43758.5453);
}


float voronoise( in float2 p, float u, float v )
{
    float k = 1.0+63.0*pow(1.0-v,6.0);

    float2 i = floor(p);
    float2 f = frac(p);
    
    float2 a = float2(0.0,0.0);
    for( int y=-2; y<=2; y++ )
    for( int x=-2; x<=2; x++ )
    {
        float2  g = float2( (float)x, (float)y ); // Cast to float for math
        float3  o = hash3( i + g )*float3(u,u,1.0);
        float2  d = g - f + o.xy;
        float w = pow( 1.0-smoothstep(0.0,1.414,length(d)), k );
        a += float2(o.z*w,w);
    }
    
    return a.x/a.y;
}


float4 main( PSInput input ) : SV_TARGET
{
    float2 uv = input.uv;

    float2 p = 0.5 - 0.5*cos( iTime+float2(0.0,2.0) );
    
    if( iMouse.w > 0.001 ) p = float2(0.0,1.0) + float2(1.0,-1.0)*iMouse.xy/iResolution.xy;
    
    p = p*p*(3.0-2.0*p);
    p = p*p*(3.0-2.0*p);
    p = p*p*(3.0-2.0*p);
    
    float f = voronoise( 24.0*uv, p.x, p.y );
    
    return float4( f, f, f, 1.0 );
}
