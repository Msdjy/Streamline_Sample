//----------------------------------------------------------------------------------
#include "FGSR_FgSample.h"

#include <donut/core/log.h>
#include <stdlib.h>
#ifdef _WIN32
#include <Windows.h>
#endif

#if USE_SL
#include <sl.h>
#include <sl_consts.h>
#ifdef STREAMLINE_FEATURE_FGSR_SR
#include <sl_fgsr_sr.h>
#endif
#endif

namespace fgsr_fg_sample
{
namespace vk
{

inline bool g_swapchainRecreatePending = false;
inline bool g_deferPluginSyncAfterRecreate = false;
inline std::function<void()> g_postRecreateCallback;

static void setFgDoublePresentEnabled(bool enabled)
{
#if defined(STREAMLINE_FEATURE_FGSR_FG)
    const char* value = enabled ? "1" : "0";
#ifdef _WIN32
    SetEnvironmentVariableA("FGSR_FG_VK_DOUBLE_PRESENT", value);
#endif
    _putenv_s("FGSR_FG_VK_DOUBLE_PRESENT", value);
    donut::log::info("FGSR VK Sample: FGSR_FG_VK_DOUBLE_PRESENT=%s", value);
#else
    (void)enabled;
#endif
}

void ConfigureDeviceStartup(donut::app::DeviceCreationParameters& deviceParams, bool fgEnabledFromCmdLine)
{
#if defined(STREAMLINE_FEATURE_FGSR_FG)
    if (!fgEnabledFromCmdLine)
    {
        setFgDoublePresentEnabled(false);
        donut::log::info(
            "FGSR VK Sample [FG Off startup]: swapChainBufferCount=3, FGSR_FG_VK_DOUBLE_PRESENT=0");
    }
    else
    {
        setFgDoublePresentEnabled(true);
    }
    if (fgEnabledFromCmdLine && deviceParams.maxFramesInFlight < kFgVkMaxFramesInFlight)
    {
        deviceParams.maxFramesInFlight = kFgVkMaxFramesInFlight;
        donut::log::info("FGSR VK Sample: FG active WSI maxFramesInFlight=%u", deviceParams.maxFramesInFlight);
    }
    donut::log::info(
        "FGSR VK Sample: maxFramesInFlight=%u, swapChainBufferCount=%u, vsync=%s",
        deviceParams.maxFramesInFlight, deviceParams.swapChainBufferCount,
        deviceParams.vsyncEnabled ? "on" : "off");
#else
    (void)deviceParams;
    (void)fgEnabledFromCmdLine;
#endif
}

void OnFgModeChanged(bool fgEnabled, const std::function<void()>& syncFgConstants)
{
#if defined(STREAMLINE_FEATURE_FGSR_FG)
    setFgDoublePresentEnabled(fgEnabled);
#endif

#if defined(STREAMLINE_FEATURE_FGSR_FG) && defined(STREAMLINE_FEATURE_FGSR_SR) && USE_SL
    slSetFeatureLoaded(sl::kFeatureFGSR_SR, !fgEnabled);
    donut::log::info("FGSR VK Sample: FG %s -> slSetFeatureLoaded(kFeatureFGSR_SR, %s)",
        fgEnabled ? "On" : "Off", fgEnabled ? "false" : "true");
#endif

    g_swapchainRecreatePending = true;
    g_deferPluginSyncAfterRecreate = true;
    donut::log::info("FGSR VK Sample: queued swapchain recreate (deferPluginSync=true)");
}

void SetSwapchainRecreatePostCallback(std::function<void()> callback)
{
    g_postRecreateCallback = std::move(callback);
}

void ProcessPendingFrameStart(donut::app::DeviceManager* deviceManager)
{
#if defined(STREAMLINE_FEATURE_FGSR_FG)
    if (!g_swapchainRecreatePending || !deviceManager)
    {
        return;
    }
    if (deviceManager->GetGraphicsAPI() != nvrhi::GraphicsAPI::VULKAN)
    {
        return;
    }
    g_swapchainRecreatePending = false;

    const bool fgEnvActive = []() {
        const char* env = std::getenv("FGSR_FG_VK_DOUBLE_PRESENT");
#ifdef _WIN32
        char winEnv[16]{};
        const DWORD envLen = GetEnvironmentVariableA("FGSR_FG_VK_DOUBLE_PRESENT", winEnv, sizeof(winEnv));
        return (envLen > 0) ? (winEnv[0] == '1') : (env && env[0] == '1');
#else
        return env && env[0] == '1';
#endif
    }();

    donut::log::info("FGSR VK Sample: recreating swapchain (FGSR_FG_VK_DOUBLE_PRESENT=%s)",
        fgEnvActive ? "1" : "0");

    deviceManager->DrainGpuFramesInFlight();

    if (fgEnvActive)
    {
        deviceManager->EnsureVulkanFgWsiDepth(kFgVkMaxFramesInFlight);
    }
    else
    {
        deviceManager->SetMaxFramesInFlight(kDefaultVkMaxFramesInFlight);
    }

    deviceManager->RecreateSwapChain();

    if (g_deferPluginSyncAfterRecreate)
    {
        g_deferPluginSyncAfterRecreate = false;
        if (g_postRecreateCallback)
        {
            g_postRecreateCallback();
        }
    }
#endif
}

} // namespace vk
} // namespace fgsr_fg_sample
