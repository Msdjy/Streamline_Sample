/*
* Unjittered Depth + Motion Vectors Pass
* Renders scene geometry with unjittered projection matrices
* to produce clean depth and motion vector outputs for FGSR
*/

#pragma once

#include <donut/engine/SceneTypes.h>
#include <donut/render/GeometryPasses.h>
#include <nvrhi/nvrhi.h>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace donut::engine
{
    class ShaderFactory;
    class CommonRenderPasses;
    class FramebufferFactory;
    class MaterialBindingCache;
    class ICompositeView;
    class IView;
}

class UnjitteredDepthMVPass : public donut::render::IGeometryPass
{
public:
    union PipelineKey
    {
        struct
        {
            nvrhi::RasterCullMode cullMode : 2;
            bool alphaTested : 1;
            bool frontCounterClockwise : 1;
            bool reverseDepth : 1;
        } bits;
        uint32_t value;

        static constexpr size_t Count = 1 << 5;
    };

    class Context : public donut::render::GeometryPassContext
    {
    public:
        nvrhi::BindingSetHandle inputBindingSet;
        PipelineKey keyTemplate;

        uint32_t positionOffset = 0;
        uint32_t prevPositionOffset = 0;
        uint32_t texCoordOffset = 0;

        Context()
        {
            keyTemplate.value = 0;
        }
    };

    struct CreateParameters
    {
        std::shared_ptr<donut::engine::MaterialBindingCache> materialBindings;
        bool trackLiveness = true;
        uint32_t numConstantBufferVersions = 16;
    };

protected:
    nvrhi::DeviceHandle m_Device;
    nvrhi::ShaderHandle m_VertexShader;
    nvrhi::ShaderHandle m_PixelShader;
    nvrhi::ShaderHandle m_PixelShaderAlphaTested;
    nvrhi::BindingLayoutHandle m_InputBindingLayout;
    nvrhi::BindingLayoutHandle m_ViewBindingLayout;
    nvrhi::BufferHandle m_ConstantBuffer;
    nvrhi::BindingSetHandle m_ViewBindingSet;
    nvrhi::GraphicsPipelineHandle m_Pipelines[PipelineKey::Count];
    std::mutex m_Mutex;

    bool m_IsDX11 = false;
    bool m_TrackLiveness = true;

    std::unordered_map<const donut::engine::BufferGroup*, nvrhi::BindingSetHandle> m_InputBindingSets;

    std::shared_ptr<donut::engine::CommonRenderPasses> m_CommonPasses;
    std::shared_ptr<donut::engine::MaterialBindingCache> m_MaterialBindings;

    // Store unjittered matrices for the pass
    donut::math::float4x4 m_MatWorldToClip;
    donut::math::float4x4 m_MatPrevWorldToClip;
    donut::math::float2 m_ViewportSize;

    nvrhi::BindingLayoutHandle CreateInputBindingLayout();
    nvrhi::BindingSetHandle CreateInputBindingSet(const donut::engine::BufferGroup* bufferGroup);
    void CreateViewBindings(nvrhi::BindingLayoutHandle& layout, nvrhi::BindingSetHandle& set, const CreateParameters& params);
    std::shared_ptr<donut::engine::MaterialBindingCache> CreateMaterialBindingCache(donut::engine::CommonRenderPasses& commonPasses);
    nvrhi::GraphicsPipelineHandle CreateGraphicsPipeline(PipelineKey key, nvrhi::IFramebuffer* framebuffer);
    nvrhi::BindingSetHandle GetOrCreateInputBindingSet(const donut::engine::BufferGroup* bufferGroup);

public:
    UnjitteredDepthMVPass(
        nvrhi::IDevice* device,
        std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses);

    void Init(
        donut::engine::ShaderFactory& shaderFactory,
        const CreateParameters& params);

    void ResetBindingCache();

    // Set the unjittered matrices before rendering
    void SetUnjitteredMatrices(
        const donut::math::float4x4& matWorldToClip,
        const donut::math::float4x4& matPrevWorldToClip,
        const donut::math::float2& viewportSize);

    // IGeometryPass implementation
    [[nodiscard]] donut::engine::ViewType::Enum GetSupportedViewTypes() const override;
    void SetupView(donut::render::GeometryPassContext& context, nvrhi::ICommandList* commandList, const donut::engine::IView* view, const donut::engine::IView* viewPrev) override;
    bool SetupMaterial(donut::render::GeometryPassContext& context, const donut::engine::Material* material, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state) override;
    void SetupInputBuffers(donut::render::GeometryPassContext& context, const donut::engine::BufferGroup* buffers, nvrhi::GraphicsState& state) override;
    void SetPushConstants(donut::render::GeometryPassContext& context, nvrhi::ICommandList* commandList, nvrhi::GraphicsState& state, nvrhi::DrawArguments& args) override;
};
