//----------------------------------------------------------------------------------
#include "FGSR_FgSample.h"

namespace fgsr_fg_sample
{
namespace dx
{
void ConfigureDeviceStartup(donut::app::DeviceCreationParameters& deviceParams, bool fgEnabledFromCmdLine);
void OnFgModeChanged(bool fgEnabled, const std::function<void()>& syncFgConstants);
} // namespace dx

namespace vk
{
void ConfigureDeviceStartup(donut::app::DeviceCreationParameters& deviceParams, bool fgEnabledFromCmdLine);
void OnFgModeChanged(bool fgEnabled, const std::function<void()>& syncFgConstants);
void SetSwapchainRecreatePostCallback(std::function<void()> callback);
void ProcessPendingFrameStart(donut::app::DeviceManager* deviceManager);
} // namespace vk

void ConfigureDeviceStartup(nvrhi::GraphicsAPI api, donut::app::DeviceCreationParameters& deviceParams, bool fgEnabledFromCmdLine)
{
    if (api == nvrhi::GraphicsAPI::VULKAN)
    {
        vk::ConfigureDeviceStartup(deviceParams, fgEnabledFromCmdLine);
    }
    else
    {
        dx::ConfigureDeviceStartup(deviceParams, fgEnabledFromCmdLine);
    }
}

void OnFgModeChanged(nvrhi::GraphicsAPI api, bool fgEnabled, const std::function<void()>& syncFgConstants)
{
    if (api == nvrhi::GraphicsAPI::VULKAN)
    {
        vk::OnFgModeChanged(fgEnabled, syncFgConstants);
    }
    else
    {
        dx::OnFgModeChanged(fgEnabled, syncFgConstants);
    }
}

void SetSwapchainRecreatePostCallback(std::function<void()> callback)
{
    vk::SetSwapchainRecreatePostCallback(std::move(callback));
}

void ProcessPendingFrameStart(donut::app::DeviceManager* deviceManager)
{
    vk::ProcessPendingFrameStart(deviceManager);
}

} // namespace fgsr_fg_sample
