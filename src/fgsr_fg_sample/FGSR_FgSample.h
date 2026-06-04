//----------------------------------------------------------------------------------
// FGSR Frame Generation — Sample-side WSI / UI glue (DX12 baseline vs Vulkan).
// DX: sync sl.fgsr_fg constants only (b3c1b38 path). VK: swapchain recreate + env.
//----------------------------------------------------------------------------------
#pragma once

#include <donut/app/DeviceManager.h>
#include <functional>
#include <nvrhi/nvrhi.h>

namespace fgsr_fg_sample
{

constexpr uint32_t kFgVkMaxFramesInFlight = 6;
constexpr uint32_t kDefaultVkMaxFramesInFlight = 3;

// main.cpp before CreateDeviceManager.
void ConfigureDeviceStartup(nvrhi::GraphicsAPI api, donut::app::DeviceCreationParameters& deviceParams, bool fgEnabledFromCmdLine);

// UI / -fgsrfg: API-specific FG toggle (DX never unloads plugins or recreates swapchain).
void OnFgModeChanged(nvrhi::GraphicsAPI api, bool fgEnabled, const std::function<void()>& syncFgConstants);

void SetSwapchainRecreatePostCallback(std::function<void()> callback);

// Start of frame: Vulkan may recreate swapchain after FG UI toggle.
void ProcessPendingFrameStart(donut::app::DeviceManager* deviceManager);

} // namespace fgsr_fg_sample
