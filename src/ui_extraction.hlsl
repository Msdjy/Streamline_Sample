// UI Extraction Compute Shader
// Diff-based method: detect UI pixels by comparing backbuffer (with UI) against PreUIColor (without UI).
// This avoids depending on alpha channel, which is unreliable due to ImGui's blend state
// writing alpha=0 for fully opaque UI (srcBlendAlpha=InvSrcAlpha, destBlendAlpha=Zero).

Texture2D<float4> t_Backbuffer : register(t0);      // Backbuffer after UI rendering (scene + UI)
Texture2D<float4> t_PreUIColor : register(t1);       // Scene without UI (PreUIColor)
RWTexture2D<float4> u_ExtractedUI : register(u0);   // Output: extracted UI with alpha
RWByteAddressBuffer u_UIStats : register(u1);
// byte 0: extracted UI pixel count
// byte 4: backbuffer non-black pixel count
// byte 8: PreUIColor non-black pixel count
// byte 12: backbuffer alpha nonzero pixel count
// byte 16: PreUIColor alpha nonzero pixel count
// byte 20: exact RGB match pixel count
// byte 24: tiny RGB diff pixel count
// byte 28: threshold RGB diff pixel count
// byte 32: dispatch sentinel pixel count

cbuffer Constants : register(b0)
{
    uint2 dimensions;
    float colorDiffThreshold;  // RGB difference threshold for UI detection
    float padding;
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelPos = DTid.xy;
    uint originalValue;

    bool inBounds = DTid.x < dimensions.x && DTid.y < dimensions.y;

    if (inBounds)
    {
        float4 withUI = t_Backbuffer[pixelPos];
        float4 withoutUI = t_PreUIColor[pixelPos];

        // Detect UI by RGB difference between backbuffer (with UI) and PreUIColor (without UI)
        float3 diff = abs(withUI.rgb - withoutUI.rgb);
        float maxDiff = max(diff.r, max(diff.g, diff.b));
        float maxBack = max(abs(withUI.r), max(abs(withUI.g), abs(withUI.b)));
        float maxPreUI = max(abs(withoutUI.r), max(abs(withoutUI.g), abs(withoutUI.b)));

        if (maxBack > colorDiffThreshold)
        {
            u_UIStats.InterlockedAdd(4, 1, originalValue);
        }
        if (maxPreUI > colorDiffThreshold)
        {
            u_UIStats.InterlockedAdd(8, 1, originalValue);
        }
        if (abs(withUI.a) > colorDiffThreshold)
        {
            u_UIStats.InterlockedAdd(12, 1, originalValue);
        }
        if (abs(withoutUI.a) > colorDiffThreshold)
        {
            u_UIStats.InterlockedAdd(16, 1, originalValue);
        }
        if (maxDiff == 0.0)
        {
            u_UIStats.InterlockedAdd(20, 1, originalValue);
        }
        if (maxDiff > 0.0)
        {
            u_UIStats.InterlockedAdd(24, 1, originalValue);
        }

        if (maxDiff > colorDiffThreshold)
        {
            // This pixel was modified by UI rendering - extract it with full opacity
            u_ExtractedUI[pixelPos] = float4(withUI.rgb, 1.0);
            u_UIStats.InterlockedAdd(0, 1, originalValue);
            u_UIStats.InterlockedAdd(28, 1, originalValue);
        }
        else
        {
            // No UI here - output transparent
            u_ExtractedUI[pixelPos] = float4(0.0, 0.0, 0.0, 0.0);
        }
    }

    if (inBounds && DTid.x < 64 && DTid.y < 64)
    {
        u_UIStats.InterlockedAdd(32, 1, originalValue);
    }
}
