/*
* Unjittered Depth + Motion Vectors Vertex Shader
* Transforms vertices without TAA jitter for FGSR
*/

#pragma pack_matrix(row_major)

#include "unjittered_depth_mv_cb.h"
#include <donut/shaders/bindless.h>
#include <donut/shaders/binding_helpers.hlsli>

DECLARE_CBUFFER(UnjitteredDepthMVConstants, g_Unjittered, UNJITTERED_BINDING_VIEW_CONSTANTS, UNJITTERED_SPACE_VIEW);

// Use a raw buffer on DX11 to avoid adding the StructuredBuffer flag to the instance buffer.
#ifdef TARGET_D3D11
ByteAddressBuffer t_Instances : REGISTER_SRV(UNJITTERED_BINDING_INSTANCE_BUFFER, UNJITTERED_SPACE_INPUT);
#else
StructuredBuffer<InstanceData> t_Instances : REGISTER_SRV(UNJITTERED_BINDING_INSTANCE_BUFFER, UNJITTERED_SPACE_INPUT);
#endif
ByteAddressBuffer t_Vertices : REGISTER_SRV(UNJITTERED_BINDING_VERTEX_BUFFER, UNJITTERED_SPACE_INPUT);

DECLARE_PUSH_CONSTANTS(UnjitteredPushConstants, g_Push, UNJITTERED_BINDING_PUSH_CONSTANTS, UNJITTERED_SPACE_INPUT);

void main(
    in uint i_vertex : SV_VertexID,
    in uint i_instance : SV_InstanceID,
    out float4 o_position : SV_Position,
    out float4 o_prevClipPos : PREV_CLIP_POS,
    out float2 o_texCoord : TEXCOORD
)
{
    i_instance += g_Push.startInstanceLocation;
    i_vertex += g_Push.startVertexLocation;

#ifdef TARGET_D3D11
    const InstanceData instance = LoadInstanceData(t_Instances, i_instance * c_SizeOfInstanceData);
#else
    const InstanceData instance = t_Instances[i_instance];
#endif

    float3 pos = asfloat(t_Vertices.Load3(g_Push.positionOffset + i_vertex * c_SizeOfPosition));
    float3 prevPos = asfloat(t_Vertices.Load3(g_Push.prevPositionOffset + i_vertex * c_SizeOfPosition));
    float2 texCoord = asfloat(t_Vertices.Load2(g_Push.texCoordOffset + i_vertex * c_SizeOfTexcoord));

    // Transform current position with unjittered matrix
    float3 worldPos = mul(instance.transform, float4(pos, 1.0));
    o_position = mul(float4(worldPos, 1.0), g_Unjittered.matWorldToClip);

    // Transform previous position with unjittered previous matrix
    float3 prevWorldPos = mul(instance.prevTransform, float4(prevPos, 1.0));
    o_prevClipPos = mul(float4(prevWorldPos, 1.0), g_Unjittered.matPrevWorldToClip);

    o_texCoord = texCoord;
}
