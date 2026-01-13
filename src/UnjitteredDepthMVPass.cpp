/*
* Unjittered Depth + Motion Vectors Pass Implementation
*/

#include "UnjitteredDepthMVPass.h"
#include <donut/engine/ShaderFactory.h>
#include <donut/engine/SceneTypes.h>
#include <donut/engine/CommonRenderPasses.h>
#include <donut/engine/View.h>
#include <donut/engine/MaterialBindingCache.h>
#include <nvrhi/utils.h>
#include <utility>

using namespace donut::math;
#include "unjittered_depth_mv_cb.h"

using namespace donut::engine;
using namespace donut::render;

UnjitteredDepthMVPass::UnjitteredDepthMVPass(
    nvrhi::IDevice* device,
    std::shared_ptr<CommonRenderPasses> commonPasses)
    : m_Device(device)
    , m_CommonPasses(std::move(commonPasses))
{
    m_IsDX11 = m_Device->getGraphicsAPI() == nvrhi::GraphicsAPI::D3D11;
}

void UnjitteredDepthMVPass::Init(ShaderFactory& shaderFactory, const CreateParameters& params)
{
    m_TrackLiveness = params.trackLiveness;

    // Compile shaders
    std::vector<ShaderMacro> macros;

    m_VertexShader = shaderFactory.CreateShader(
        "app/unjittered_depth_mv_vs.hlsl", "main",
        nullptr, nvrhi::ShaderType::Vertex);

    macros.clear();
    macros.push_back(ShaderMacro("ALPHA_TESTED", "0"));
    m_PixelShader = shaderFactory.CreateShader(
        "app/unjittered_depth_mv_ps.hlsl", "main",
        &macros, nvrhi::ShaderType::Pixel);

    macros.clear();
    macros.push_back(ShaderMacro("ALPHA_TESTED", "1"));
    m_PixelShaderAlphaTested = shaderFactory.CreateShader(
        "app/unjittered_depth_mv_ps.hlsl", "main",
        &macros, nvrhi::ShaderType::Pixel);

    m_InputBindingLayout = CreateInputBindingLayout();

    if (params.materialBindings)
        m_MaterialBindings = params.materialBindings;
    else
        m_MaterialBindings = CreateMaterialBindingCache(*m_CommonPasses);

    m_ConstantBuffer = m_Device->createBuffer(nvrhi::utils::CreateVolatileConstantBufferDesc(
        sizeof(UnjitteredDepthMVConstants), "UnjitteredDepthMVConstants", params.numConstantBufferVersions));

    CreateViewBindings(m_ViewBindingLayout, m_ViewBindingSet, params);
}

void UnjitteredDepthMVPass::ResetBindingCache()
{
    m_MaterialBindings->Clear();
    m_InputBindingSets.clear();
}

void UnjitteredDepthMVPass::SetUnjitteredMatrices(
    const float4x4& matWorldToClip,
    const float4x4& matPrevWorldToClip,
    const float2& viewportSize)
{
    m_MatWorldToClip = matWorldToClip;
    m_MatPrevWorldToClip = matPrevWorldToClip;
    m_ViewportSize = viewportSize;
}

void UnjitteredDepthMVPass::CreateViewBindings(nvrhi::BindingLayoutHandle& layout, nvrhi::BindingSetHandle& set, const CreateParameters& params)
{
    auto bindingLayoutDesc = nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel)
        .setRegisterSpace(m_IsDX11 ? 0 : UNJITTERED_SPACE_VIEW)
        .setRegisterSpaceIsDescriptorSet(!m_IsDX11)
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(UNJITTERED_BINDING_VIEW_CONSTANTS))
        .addItem(nvrhi::BindingLayoutItem::Sampler(UNJITTERED_BINDING_MATERIAL_SAMPLER));

    layout = m_Device->createBindingLayout(bindingLayoutDesc);

    auto bindingSetDesc = nvrhi::BindingSetDesc()
        .setTrackLiveness(params.trackLiveness)
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(UNJITTERED_BINDING_VIEW_CONSTANTS, m_ConstantBuffer))
        .addItem(nvrhi::BindingSetItem::Sampler(UNJITTERED_BINDING_MATERIAL_SAMPLER,
            m_CommonPasses->m_AnisotropicWrapSampler));

    set = m_Device->createBindingSet(bindingSetDesc, layout);
}

std::shared_ptr<MaterialBindingCache> UnjitteredDepthMVPass::CreateMaterialBindingCache(CommonRenderPasses& commonPasses)
{
    std::vector<MaterialResourceBinding> materialBindings = {
        { MaterialResource::DiffuseTexture, UNJITTERED_BINDING_MATERIAL_DIFFUSE_TEXTURE },
        { MaterialResource::OpacityTexture, UNJITTERED_BINDING_MATERIAL_OPACITY_TEXTURE },
        { MaterialResource::ConstantBuffer, UNJITTERED_BINDING_MATERIAL_CONSTANTS }
    };

    return std::make_shared<MaterialBindingCache>(
        m_Device,
        nvrhi::ShaderType::Pixel,
        /* registerSpace = */ m_IsDX11 ? 0 : UNJITTERED_SPACE_MATERIAL,
        /* registerSpaceIsDescriptorSet = */ !m_IsDX11,
        materialBindings,
        commonPasses.m_AnisotropicWrapSampler,
        commonPasses.m_GrayTexture,
        commonPasses.m_BlackTexture);
}

nvrhi::GraphicsPipelineHandle UnjitteredDepthMVPass::CreateGraphicsPipeline(PipelineKey key, nvrhi::IFramebuffer* framebuffer)
{
    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.inputLayout = nullptr; // Using buffer loads
    pipelineDesc.VS = m_VertexShader;
    pipelineDesc.PS = key.bits.alphaTested ? m_PixelShaderAlphaTested : m_PixelShader;
    pipelineDesc.renderState.rasterState.frontCounterClockwise = key.bits.frontCounterClockwise;
    pipelineDesc.renderState.rasterState.cullMode = key.bits.cullMode;
    pipelineDesc.renderState.depthStencilState.depthFunc = key.bits.reverseDepth
        ? nvrhi::ComparisonFunc::GreaterOrEqual
        : nvrhi::ComparisonFunc::LessOrEqual;

    pipelineDesc.bindingLayouts = { m_ViewBindingLayout };

    if (key.bits.alphaTested)
    {
        pipelineDesc.bindingLayouts.push_back(m_MaterialBindings->GetLayout());
    }

    pipelineDesc.bindingLayouts.push_back(m_InputBindingLayout);

    return m_Device->createGraphicsPipeline(pipelineDesc, framebuffer);
}

nvrhi::BindingLayoutHandle UnjitteredDepthMVPass::CreateInputBindingLayout()
{
    auto bindingLayoutDesc = nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Vertex)
        .setRegisterSpace(m_IsDX11 ? 0 : UNJITTERED_SPACE_INPUT)
        .setRegisterSpaceIsDescriptorSet(!m_IsDX11)
        .addItem(m_IsDX11
            ? nvrhi::BindingLayoutItem::RawBuffer_SRV(UNJITTERED_BINDING_INSTANCE_BUFFER)
            : nvrhi::BindingLayoutItem::StructuredBuffer_SRV(UNJITTERED_BINDING_INSTANCE_BUFFER))
        .addItem(nvrhi::BindingLayoutItem::RawBuffer_SRV(UNJITTERED_BINDING_VERTEX_BUFFER))
        .addItem(nvrhi::BindingLayoutItem::PushConstants(UNJITTERED_BINDING_PUSH_CONSTANTS, sizeof(UnjitteredPushConstants)));

    return m_Device->createBindingLayout(bindingLayoutDesc);
}

nvrhi::BindingSetHandle UnjitteredDepthMVPass::CreateInputBindingSet(const BufferGroup* bufferGroup)
{
    auto bindingSetDesc = nvrhi::BindingSetDesc()
        .addItem(m_IsDX11
            ? nvrhi::BindingSetItem::RawBuffer_SRV(UNJITTERED_BINDING_INSTANCE_BUFFER, bufferGroup->instanceBuffer)
            : nvrhi::BindingSetItem::StructuredBuffer_SRV(UNJITTERED_BINDING_INSTANCE_BUFFER, bufferGroup->instanceBuffer))
        .addItem(nvrhi::BindingSetItem::RawBuffer_SRV(UNJITTERED_BINDING_VERTEX_BUFFER, bufferGroup->vertexBuffer))
        .addItem(nvrhi::BindingSetItem::PushConstants(UNJITTERED_BINDING_PUSH_CONSTANTS, sizeof(UnjitteredPushConstants)));

    return m_Device->createBindingSet(bindingSetDesc, m_InputBindingLayout);
}

nvrhi::BindingSetHandle UnjitteredDepthMVPass::GetOrCreateInputBindingSet(const BufferGroup* bufferGroup)
{
    auto it = m_InputBindingSets.find(bufferGroup);
    if (it == m_InputBindingSets.end())
    {
        auto bindingSet = CreateInputBindingSet(bufferGroup);
        m_InputBindingSets[bufferGroup] = bindingSet;
        return bindingSet;
    }

    return it->second;
}

void UnjitteredDepthMVPass::SetPushConstants(
    GeometryPassContext& abstractContext,
    nvrhi::ICommandList* commandList,
    nvrhi::GraphicsState& state,
    nvrhi::DrawArguments& args)
{
    auto& context = static_cast<Context&>(abstractContext);

    UnjitteredPushConstants constants = {};
    constants.startInstanceLocation = args.startInstanceLocation;
    constants.startVertexLocation = args.startVertexLocation;
    constants.positionOffset = context.positionOffset;
    constants.prevPositionOffset = context.prevPositionOffset;
    constants.texCoordOffset = context.texCoordOffset;

    commandList->setPushConstants(&constants, sizeof(constants));

    args.startInstanceLocation = 0;
    args.startVertexLocation = 0;
}

ViewType::Enum UnjitteredDepthMVPass::GetSupportedViewTypes() const
{
    return ViewType::PLANAR;
}

void UnjitteredDepthMVPass::SetupView(GeometryPassContext& abstractContext, nvrhi::ICommandList* commandList, const IView* view, const IView* viewPrev)
{
    auto& context = static_cast<Context&>(abstractContext);

    // Use the pre-set unjittered matrices
    UnjitteredDepthMVConstants constants = {};
    constants.matWorldToClip = m_MatWorldToClip;
    constants.matPrevWorldToClip = m_MatPrevWorldToClip;
    constants.viewportSize = m_ViewportSize;

    commandList->writeBuffer(m_ConstantBuffer, &constants, sizeof(constants));

    context.keyTemplate.bits.frontCounterClockwise = view->IsMirrored();
    context.keyTemplate.bits.reverseDepth = view->IsReverseDepth();
}

bool UnjitteredDepthMVPass::SetupMaterial(GeometryPassContext& abstractContext, const Material* material, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state)
{
    auto& context = static_cast<Context&>(abstractContext);

    PipelineKey key = context.keyTemplate;
    key.bits.cullMode = cullMode;

    bool const hasBaseOrDiffuseTexture = material->baseOrDiffuseTexture
        && material->baseOrDiffuseTexture->texture
        && material->enableBaseOrDiffuseTexture;

    bool const hasOpacityTexture = material->opacityTexture
        && material->opacityTexture->texture
        && material->enableOpacityTexture;

    if (material->domain == MaterialDomain::AlphaTested && (hasBaseOrDiffuseTexture || hasOpacityTexture))
    {
        nvrhi::IBindingSet* materialBindingSet = m_MaterialBindings->GetMaterialBindingSet(material);

        if (!materialBindingSet)
            return false;

        state.bindings = { m_ViewBindingSet, materialBindingSet, context.inputBindingSet };
        key.bits.alphaTested = true;
    }
    else if (material->domain == MaterialDomain::Opaque)
    {
        state.bindings = { m_ViewBindingSet, context.inputBindingSet };
        key.bits.alphaTested = false;
    }
    else
    {
        return false;
    }

    nvrhi::GraphicsPipelineHandle& pipeline = m_Pipelines[key.value];

    if (!pipeline)
    {
        std::lock_guard<std::mutex> lockGuard(m_Mutex);

        if (!pipeline)
            pipeline = CreateGraphicsPipeline(key, state.framebuffer);

        if (!pipeline)
            return false;
    }

    assert(pipeline->getFramebufferInfo() == state.framebuffer->getFramebufferInfo());

    state.pipeline = pipeline;
    return true;
}

void UnjitteredDepthMVPass::SetupInputBuffers(GeometryPassContext& abstractContext, const BufferGroup* buffers, nvrhi::GraphicsState& state)
{
    auto& context = static_cast<Context&>(abstractContext);

    state.indexBuffer = { buffers->indexBuffer, nvrhi::Format::R32_UINT, 0 };

    context.inputBindingSet = GetOrCreateInputBindingSet(buffers);
    context.positionOffset = uint32_t(buffers->getVertexBufferRange(VertexAttribute::Position).byteOffset);
    context.prevPositionOffset = uint32_t(buffers->getVertexBufferRange(VertexAttribute::PrevPosition).byteOffset);
    context.texCoordOffset = uint32_t(buffers->getVertexBufferRange(VertexAttribute::TexCoord1).byteOffset);
}
