// HLSL Translation of Warp Effect Shader

// Constant Buffer (Must be bound to b0, contains only the necessary uniforms)
cbuffer PixelConstantBuffer : register(b2)
{
    float4 iResolution; // (width, height, 1.0/width, 1.0/height)
    float iTime; // Time since application start
    float3 padding_time; // 12 bytes of padding
};

// Vertex Shader Output Structure (Standard for a full-screen quad effect)
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0; // UVs range from 0 to 1
};

//====================================================================
// Original Shader Code Translation
//====================================================================

static const float2x2 m = float2x2(0.80, -0.60,
                                    0.60, 0.80);
// Note on 'm' initialization: HLSL matrices are column-major by default.
// GLSL (vec2 p = m * p) implies row-major layout, or the vector is a column vector
// on the left. Since the GLSL original uses `p = m * p`, we must ensure the 
// matrix is transposed or initialized correctly for HLSL's `p = mul(p, m)`.
// Here, we've initialized it with the columns transposed to match the GLSL math flow:
// GLSL: (0.80, 0.60) in row 1, (-0.60, 0.80) in row 2
// HLSL: (0.80, 0.60) in col 1, (-0.60, 0.80) in col 2 (which is the transpose)
// If the multiplication is written as `p = mul(p, m)` the matrix must be row-major in C++ or transposed here. 
// We will stick to `p = mul(p, m)` in the FBM functions.

float noise(in float2 p)
{
    return sin(p.x) * sin(p.y);
}

float fbm4(float2 p)
{
    // float f = 0.0;
    // f += 0.5000*noise( p ); p = mul(p, m)*2.02;
    // f += 0.2500*noise( p ); p = mul(p, m)*2.03;
    // f += 0.1250*noise( p ); p = mul(p, m)*2.01;
    // f += 0.0625*noise( p );
    
    // Using simple unrolled version which is often faster in modern HLSL/DX
    float f = 0.0;
    f += 0.5000 * noise(p);
    p = mul(p, m) * 2.02;
    f += 0.2500 * noise(p);
    p = mul(p, m) * 2.03;
    f += 0.1250 * noise(p);
    p = mul(p, m) * 2.01;
    f += 0.0625 * noise(p);

    return f / 0.9375;
}

float fbm6(float2 p)
{
    float f = 0.0;
    f += 0.500000 * (0.5 + 0.5 * noise(p));
    p = mul(p, m) * 2.02;
    f += 0.250000 * (0.5 + 0.5 * noise(p));
    p = mul(p, m) * 2.03;
    f += 0.125000 * (0.5 + 0.5 * noise(p));
    p = mul(p, m) * 2.01;
    f += 0.062500 * (0.5 + 0.5 * noise(p));
    p = mul(p, m) * 2.04;
    f += 0.031250 * (0.5 + 0.5 * noise(p));
    p = mul(p, m) * 2.01;
    f += 0.015625 * (0.5 + 0.5 * noise(p));
    return f / 0.96875;
}

float2 fbm4_2(float2 p)
{
    return float2(fbm4(p), fbm4(p + float2(7.8, 0.0))); // Added 0.0 to make it explicit float2
}

float2 fbm6_2(float2 p)
{
    return float2(fbm6(p + float2(16.8, 0.0)), fbm6(p + float2(11.5, 0.0)));
}

//====================================================================

float func(float2 q, out float4 ron)
{
    // Added 0.0 for explicit float2
    q += 0.03 * sin(float2(0.27, 0.23) * iTime + length(q) * float2(4.1, 4.3));

    float2 o = fbm4_2(0.9 * q);

    o += 0.04 * sin(float2(0.12, 0.14) * iTime + length(o));

    float2 n = fbm6_2(3.0 * o);

    ron = float4(o.x, o.y, n.x, n.y); // Unroll vec4(o, n) to float4(o.x, o.y, n.x, n.y)

    float f = 0.5 + 0.5 * fbm4(1.8 * q + 6.0 * n);

    return lerp(f, f * f * f * 3.5, f * abs(n.x)); // GLSL mix -> HLSL lerp
}

float4 main(PSInput input) : SV_TARGET
{
    // Convert UV (0-1) to normalized screen coordinates (-aspect, aspect)
    // The GLSL formula: vec2 p = (2.0*fragCoord-iResolution.xy)/iResolution.y;
    float2 fragCoord = input.uv * iResolution.xy;
    float2 p = (2.0 * fragCoord - iResolution.xy) / iResolution.y;
    
    // Calculate epsilon for manual derivative
    float e = 2.0 / iResolution.y;

    float4 on = float4(0.0, 0.0, 0.0, 0.0);
    float f = func(p, on);

    float3 col = float3(0.0, 0.0, 0.0);
    col = lerp(float3(0.2, 0.1, 0.4), float3(0.3, 0.05, 0.05), f);
    col = lerp(col, float3(0.9, 0.9, 0.9), dot(on.zw, on.zw));
    col = lerp(col, float3(0.4, 0.3, 0.3), 0.2 + 0.5 * on.y * on.y);
    col = lerp(col, float3(0.0, 0.2, 0.4), 0.5 * smoothstep(1.2, 1.3, abs(on.z) + abs(on.w)));
    col = saturate(col * f * 2.0); // clamp(x, 0.0, 1.0) -> saturate(x)
    
// The compiler error X3511 is NOT related to the manual derivative logic (this is NOT a loop).
// The manual derivative is a simple, non-looping block of code.
// The derivatives used here (dFdx, dFdy) are what caused the *previous* shader's X3570 warning, but here
// the manual derivative is often preferred. We'll use the manual one for stability.
#if 0 
    // gpu derivatives - bad quality, but fast
	float3 nor = normalize( float3( ddx(f)*iResolution.x, 6.0, ddy(f)*iResolution.y ) ); // dFdx/dFdy -> ddx/ddy
#else
    // manual derivatives - better quality, but slower
    float4 kk;
    float3 nor = normalize(float3(func(p + float2(e, 0.0), kk) - f,
                                    2.0 * e,
                                    func(p + float2(0.0, e), kk) - f));
#endif
    
    float3 lig = normalize(float3(0.9, 0.2, -0.4));
    float dif = saturate(0.3 + 0.7 * dot(nor, lig));
    float3 lin = float3(0.70, 0.90, 0.95) * (nor.y * 0.5 + 0.5) + float3(0.15, 0.10, 0.05) * dif;
    col *= 1.2 * lin;
    col = 1.0 - col;
    col = 1.1 * col * col;
    
    return float4(col, 1.0);
}