//----------------------------------------------------------------------------------
// File:        UIData.h
// SDK Version: 2.0
// Email:       StreamlineSupport@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------
#pragma once

// Donut
#include <donut/engine/Scene.h>
#include <donut/render/TemporalAntiAliasingPass.h>
#include <donut/render/SsaoPass.h>
#include <donut/render/SkyPass.h>
#include <donut/render/ToneMappingPasses.h>
#include <nvrhi/nvrhi.h>
#include "NVWrapper.h"

/// <summary>
/// This enum describes the available anti-aliasing modes. These can be toggled from the UI
/// </summary>
enum class AntiAliasingMode {
    NONE,
    TEMPORAL,
    DLSS,
};

/// <summary>
/// This enum describes the Dynamic Resolution mode used in-game
/// </summary>
enum class RenderingResolutionMode {
    FIXED,
    DYNAMIC,
    COUNT
};

struct UIData
{
    // General
    nvrhi::GraphicsAPI                  GraphicsAPI = nvrhi::GraphicsAPI::D3D12;
    bool                                EnableAnimations = true;
    float                               AnimationSpeed = 1.;
    bool                                EnableVsync = false;
    bool                                VisualiseBuffers = false;
    float                               CpuLoad = 0;
    int                                 GpuLoad = 0;
    donut::math::int2                   Resolution = { 0,0 };
    bool                                Resolution_changed = false;
    bool                                MouseOverUI = false;

    uint32_t getNViewports() const { return (uint32_t)BackBufferExtents.size(); }
    sl::Extent getExtent(uint32_t fullWidth, uint32_t fullHeight, uint32_t uV);
private:
    friend class UIRenderer;
    std::vector<sl::Extent>             BackBufferExtents{};
public:

    // SSAO
    bool                                EnableSsao = true;
    donut::render::SsaoParameters       SsaoParams;

    // Tonemapping
    bool                                 EnableToneMapping = true;
    donut::render::ToneMappingParameters ToneMappingParams;

    // Sky
    bool                                EnableProceduralSky = true;
    donut::render::SkyParameters        SkyParams;
    float                               AmbientIntensity = .2f;

    // Antialising (+TAA)
    AntiAliasingMode                                   AAMode = AntiAliasingMode::NONE;
    donut::render::TemporalAntiAliasingJitter          TemporalAntiAliasingJitter = donut::render::TemporalAntiAliasingJitter::MSAA;
    donut::render::TemporalAntiAliasingParameters      TemporalAntiAliasingParams;

    // Bloom
    bool                                EnableBloom = true;
    float                               BloomSigma = 32.f;
    float                               BloomAlpha = 0.05f;

    // Shadows
    bool                                EnableShadows = true;
    float                               CsmExponent = 4.f;
#ifdef STREAMLINE_FEATURE_DLSS_RR
    // ray tracing on
    int                                RayTracing_Mode = 1;
    
    // DLSS RR specific parameters
    bool                                DLSSRR_Supported = false;
    bool                                DLSSRREnabled   = false;
    sl::DLSSMode                        DLSSRR_Mode = sl::DLSSMode::eOff;
    sl::DLSSDPreset                     DLSSRR_Preset = sl::DLSSDPreset::eDefault;
    float                               DLSSRR_Sharpness = 0.f;
    sl::DLSSDPreset                     DLSSRR_presets[static_cast<int>(sl::DLSSMode::eCount)] = {};
    sl::DLSSDPreset                     DLSSRR_last_presets[static_cast<int>(sl::DLSSMode::eCount)] = {};
    bool UIData::DLSSRRPresetsChanged()
    {
        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
        {
            if (DLSSRR_presets[i] != DLSSRR_last_presets[i])
                return true;
        }
        return false;
    };
    bool UIData::DLSSRRPresetsAnyNonDefault()
    {
        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
        {
            if (DLSSRR_presets[i] != sl::DLSSDPreset::eDefault)
                return true;
        }
        return false;
    };
    void UIData::DLSSRRPresetsUpdate()
    {
        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
            DLSSRR_last_presets[i] = DLSSRR_presets[i];
    };
    void UIData::DLSSRRPresetsReset()
    {
        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
            DLSSRR_last_presets[i] = DLSSRR_presets[i] = sl::DLSSDPreset::eDefault;
    };
#endif // STREAMLINE_FEATURE_DLSS_RR

    // DLSS specific parameters
    float                               DLSS_Sharpness = 0.f;
    bool                                DLSS_Supported = false;
    sl::DLSSMode                        DLSS_Mode = sl::DLSSMode::eOff;
    RenderingResolutionMode             DLSS_Resolution_Mode = RenderingResolutionMode::FIXED;
    bool                                DLSS_Dynamic_Res_change = true;
    AntiAliasingMode                    DLSS_Last_AA = AntiAliasingMode::NONE;
    bool                                DLSS_DebugShowFullRenderingBuffer = false;
    bool                                DLSS_lodbias_useoveride = false;

    // DLSS Flag Override (优先于自动检测)
    // -1=使用自动检测, 0=强制关闭, 1=强制开启
    int                                 DLSS_OverrideIsHDR = -1;       // Override IsHDR flag
    int                                 DLSS_OverrideMVJittered = -1;  // Override MVJittered flag
    float                               DLSS_lodbias_overide = 0.f;
    bool                                DLSS_always_use_extents = false;
    sl::DLSSPreset                      DLSS_presets[static_cast<int>(sl::DLSSMode::eCount)] = {};
    sl::DLSSPreset                      DLSS_last_presets[static_cast<int>(sl::DLSSMode::eCount)] = {};
    bool DLSSPresetsChanged()
    {
        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
        {
            if (DLSS_presets[i] != DLSS_last_presets[i])
                return true;
        }
        return false;
    };
    bool DLSSPresetsAnyNonDefault()
    {
        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
        {
            if (DLSS_presets[i] != sl::DLSSPreset::eDefault)
                return true;
        }
        return false;
    };
    void DLSSPresetsUpdate()
    {
        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
            DLSS_last_presets[i] = DLSS_presets[i];
    };
    void DLSSPresetsReset()
    {
        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
            DLSS_last_presets[i] = DLSS_presets[i] = sl::DLSSPreset::eDefault;
    };

    // NIS specific parameters
    bool                                NIS_Supported = false;
    sl::NISMode                         NIS_Mode = sl::NISMode::eOff;
    float                               NIS_Sharpness = 0.5f;

    // ========================================
    // Global Jitter Override (独立，可覆盖默认行为)
    // ========================================
    bool   Global_Jitter_Override = true;     // 是否覆盖默认 jitter 行为
    bool   Global_UseJitter = true;           // 覆盖时：是否使用 jitter
    bool   Global_TestJitter = false;         // 覆盖时：使用测试 jitter
    float  Global_TestJitterX = 0.25f;
    float  Global_TestJitterY = 0.25f;

    // ========================================
    // Global Unjittered Pass (DLSS/FGSR 通用)
    // ========================================
    bool   Global_UseUnjitteredPass = true;   // 使用单独的 unjittered pass 生成 depth/MV

    // ========================================
    // Global HDR/LDR Input Mode (DLSS/FGSR 通用)
    // ========================================
    bool   Global_UseHDRInput = true;         // true=HDR输入(默认), false=LDR输入(先540p ToneMap再上采样)

#ifdef STREAMLINE_FEATURE_FGSR_SR
    // FGSR_SR specific parameters
    bool                                FGSR_SR_Supported = false;

    // 主模式: 0=Off, 1=FirstFrameUpsampleBlend, 2=EveryFrameUpsampleBlend
    sl::FGSR_SRMode                     FGSR_SR_Mode = sl::FGSR_SRMode::eOff;

    // 通用选项
    bool                                FGSR_SR_UseTRT = true;        // 是否使用 TRT 上采样
    int                                 FGSR_SR_TRTBackend = 0;       // TRT 后端: 0=CS, 1=CUDA
    int                                 FGSR_SR_ScaleFactor = 2;      // 放大倍率: 1=1x, 2=2x, 4=4x

    // DLSS对齐选项
    bool                                FGSR_SR_UseLodBias = true;    // 使用和DLSS一样的LOD Bias
    // UseHDRInput 已移到 Global_UseHDRInput

    // Blend 选项
    bool                                FGSR_SR_UseNewBlendLogic = true;  // Blend shader 新逻辑开关
    int                                 FGSR_SR_DebugOutput = 0;      // 0=正常, 1=color, 2=mv, 3=depth

    // Jitter Fix 选项
    bool                                FGSR_SR_UseDepthMVJitterFix = false;      // Depth/MV Jitter 修复 (当前帧+历史帧)
    bool                                FGSR_SR_DoJitterFixBeforeUp = false;      // Color 上采样前修复 (JitterResample)
    bool                                FGSR_SR_UseColorJitterFix = true;         // Color 在 Blend 内部修复

    // EveryFrameUpsampleBlend 模式的步骤开关
    bool                                FGSR_SR_DoUpsample = true;
    bool                                FGSR_SR_DoBlend = true;

    // 2x 模型选择
    bool                                FGSR_SR_UseOur2xModel = false;  // false=原版, true=我们的

    // Halton 2x 多模型模式 (测试用，16个模型根据 jitter 点选择)
    bool                                FGSR_SR_UseHalton2xModels = false;

    // 计算出的上采样模式 (供内部使用)
    sl::FGSR_UpsampleMode               FGSR_SR_UpsampleMode = sl::FGSR_UpsampleMode::eShader;
#endif

    // DeepDVC specific parameters
    bool                                DeepDVC_Supported = false;
    sl::DeepDVCMode                     DeepDVC_Mode = sl::DeepDVCMode::eOff;
    float                               DeepDVC_Intensity = 0.5f;
    float                               DeepDVC_SaturationBoost = 0.75f;
    uint64_t                            DeepDVC_VRAM = 0;

    // LATENCY specific parameters
    bool                                REFLEX_Supported = false;
    bool                                REFLEX_LowLatencyAvailable = false;
    int                                 REFLEX_Mode = static_cast<int>(sl::ReflexMode::eOff);
    int                                 REFLEX_CapedFPS = 0;
    std::string                         REFLEX_Stats = "";

    // DLFG specific parameters
    bool                                DLSSG_Supported = false;
    sl::DLSSGMode                       DLSSG_mode = sl::DLSSGMode::eOff;
    int                                 DLSSG_numFrames = 2;
    int                                 DLSSG_numFramesMaxMultiplier = 4;
    float                               DLSSG_fps = 0;
    size_t                              DLSSG_memory = 0;
    std::string                         DLSSG_status = "";
    bool                                DLSSG_cleanup_needed = false;

    // Latewarp
    bool                                Latewarp_Supported = false;
    int                                 Latewarp_active = 0;
    bool                                Latewarp_cleanup_needed = false;

};

