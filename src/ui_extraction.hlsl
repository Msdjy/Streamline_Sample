// UI Extraction Compute Shader
// UE-style alpha threshold method: extract UI pixels based on backbuffer alpha
// Assumes: scene pixels have alpha = 0, UI pixels have alpha > 0

Texture2D<float4> t_Backbuffer : register(t0);      // Backbuffer after UI rendering
RWTexture2D<float4> u_ExtractedUI : register(u0);   // Output: extracted UI with alpha

cbuffer Constants : register(b0)
{
    uint2 dimensions;
    float alphaThreshold;  // Threshold for alpha detection (typically 0.0)
    float padding;
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= dimensions.x || DTid.y >= dimensions.y)
        return;

    uint2 pixelPos = DTid.xy;

    float4 colorAlpha = t_Backbuffer[pixelPos];

    // UE method: if alpha > threshold, this is UI pixel, keep it; otherwise transparent
    if (colorAlpha.a > alphaThreshold)
    {
        u_ExtractedUI[pixelPos] = colorAlpha;
    }
    else
    {
        u_ExtractedUI[pixelPos] = float4(0.0, 0.0, 0.0, 0.0);
    }
}
