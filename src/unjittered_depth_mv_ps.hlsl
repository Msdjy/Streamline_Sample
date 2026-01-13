/*
* Unjittered Depth + Motion Vectors Pixel Shader
* Outputs motion vectors without TAA jitter for FGSR
* Depth is written automatically via SV_Position.z
*/

#pragma pack_matrix(row_major)

#include "unjittered_depth_mv_cb.h"
#include <donut/shaders/material_cb.h>
#include <donut/shaders/scene_material.hlsli>
#include <donut/shaders/binding_helpers.hlsli>

DECLARE_CBUFFER(UnjitteredDepthMVConstants, g_Unjittered, UNJITTERED_BINDING_VIEW_CONSTANTS, UNJITTERED_SPACE_VIEW);
DECLARE_CBUFFER(MaterialConstants, g_Material, UNJITTERED_BINDING_MATERIAL_CONSTANTS, UNJITTERED_SPACE_MATERIAL);

Texture2D t_BaseOrDiffuse       : REGISTER_SRV(UNJITTERED_BINDING_MATERIAL_DIFFUSE_TEXTURE, UNJITTERED_SPACE_MATERIAL);
Texture2D t_Opacity             : REGISTER_SRV(UNJITTERED_BINDING_MATERIAL_OPACITY_TEXTURE, UNJITTERED_SPACE_MATERIAL);
SamplerState s_MaterialSampler  : REGISTER_SAMPLER(UNJITTERED_BINDING_MATERIAL_SAMPLER, UNJITTERED_SPACE_VIEW);

void main(
    in float4 i_position : SV_Position,
    in float4 i_prevClipPos : PREV_CLIP_POS,
    in float2 i_texCoord : TEXCOORD,
    out float2 o_motionVector : SV_Target0
)
{
    // Alpha test for transparency
#if ALPHA_TESTED
    MaterialTextureSample textures = DefaultMaterialTextures();
    if ((g_Material.flags & MaterialFlags_UseBaseOrDiffuseTexture) != 0)
        textures.baseOrDiffuse = t_BaseOrDiffuse.Sample(s_MaterialSampler, i_texCoord);
    if ((g_Material.flags & MaterialFlags_UseOpacityTexture) != 0)
        textures.opacity = t_Opacity.Sample(s_MaterialSampler, i_texCoord).r;

    MaterialSample materialSample = EvaluateSceneMaterial(float3(1, 0, 0), float4(0, 1, 0, 0), g_Material, textures);
    clip(materialSample.opacity - g_Material.alphaCutoff);
#endif

    // Calculate motion vector (unjittered)
    // Current position is at i_position.xy (screen space)
    // Previous position needs to be converted from clip to screen space

    if (i_prevClipPos.w <= 0)
    {
        o_motionVector = float2(0, 0);
        return;
    }

    float2 prevNDC = i_prevClipPos.xy / i_prevClipPos.w;

    // Convert from NDC [-1,1] to screen space [0, viewportSize]
    float2 prevScreenPos;
    prevScreenPos.x = (prevNDC.x * 0.5 + 0.5) * g_Unjittered.viewportSize.x;
    prevScreenPos.y = (1.0 - (prevNDC.y * 0.5 + 0.5)) * g_Unjittered.viewportSize.y;

    // Motion vector = previous position - current position
    // This matches the convention used by DLSS/FGSR
    o_motionVector = prevScreenPos - i_position.xy;
}
