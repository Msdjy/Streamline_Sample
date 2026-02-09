// UI Extraction Compute Shader
// Diff-based method: detect UI pixels by comparing backbuffer (with UI) against PreUIColor (without UI).
// This avoids depending on alpha channel, which is unreliable due to ImGui's blend state
// writing alpha=0 for fully opaque UI (srcBlendAlpha=InvSrcAlpha, destBlendAlpha=Zero).

Texture2D<float4> t_Backbuffer : register(t0);      // Backbuffer after UI rendering (scene + UI)
Texture2D<float4> t_PreUIColor : register(t1);       // Scene without UI (PreUIColor)
RWTexture2D<float4> u_ExtractedUI : register(u0);   // Output: extracted UI with alpha

cbuffer Constants : register(b0)
{
    uint2 dimensions;
    float colorDiffThreshold;  // RGB difference threshold for UI detection
    float padding;
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= dimensions.x || DTid.y >= dimensions.y)
        return;

    uint2 pixelPos = DTid.xy;

    float4 withUI = t_Backbuffer[pixelPos];
    float4 withoutUI = t_PreUIColor[pixelPos];

    // Detect UI by RGB difference between backbuffer (with UI) and PreUIColor (without UI)
    float3 diff = abs(withUI.rgb - withoutUI.rgb);
    float maxDiff = max(diff.r, max(diff.g, diff.b));

    if (maxDiff > colorDiffThreshold)
    {
        // This pixel was modified by UI rendering - extract it with full opacity
        u_ExtractedUI[pixelPos] = float4(withUI.rgb, 1.0);
    }
    else
    {
        // No UI here - output transparent
        u_ExtractedUI[pixelPos] = float4(0.0, 0.0, 0.0, 0.0);
    }
}
