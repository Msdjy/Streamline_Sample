/*
* Unjittered Depth + Motion Vectors Pass
* For FGSR - outputs depth and MV without TAA jitter
*/

#ifndef UNJITTERED_DEPTH_MV_CB_H
#define UNJITTERED_DEPTH_MV_CB_H

#define UNJITTERED_SPACE_MATERIAL 0
#define UNJITTERED_BINDING_MATERIAL_DIFFUSE_TEXTURE 0
#define UNJITTERED_BINDING_MATERIAL_OPACITY_TEXTURE 1
#define UNJITTERED_BINDING_MATERIAL_CONSTANTS 0

#define UNJITTERED_SPACE_INPUT 1
#define UNJITTERED_BINDING_PUSH_CONSTANTS 1
#define UNJITTERED_BINDING_INSTANCE_BUFFER 10
#define UNJITTERED_BINDING_VERTEX_BUFFER 11

#define UNJITTERED_SPACE_VIEW 2
#define UNJITTERED_BINDING_VIEW_CONSTANTS 2
#define UNJITTERED_BINDING_MATERIAL_SAMPLER 0

struct UnjitteredDepthMVConstants
{
    float4x4    matWorldToClip;         // Unjittered current frame matrix
    float4x4    matPrevWorldToClip;     // Unjittered previous frame matrix
    float2      viewportSize;           // For MV calculation
    float2      padding;
};

struct UnjitteredPushConstants
{
    uint        startInstanceLocation;
    uint        startVertexLocation;
    uint        positionOffset;
    uint        prevPositionOffset;
    uint        texCoordOffset;
    uint        padding[3];
};

#endif // UNJITTERED_DEPTH_MV_CB_H
