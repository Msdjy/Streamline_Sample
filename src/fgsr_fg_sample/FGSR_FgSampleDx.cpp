//----------------------------------------------------------------------------------
#include "FGSR_FgSample.h"

#include <donut/core/log.h>

namespace fgsr_fg_sample
{

namespace dx
{

void ConfigureDeviceStartup(donut::app::DeviceCreationParameters& deviceParams, bool fgEnabledFromCmdLine)
{
    (void)deviceParams;
    (void)fgEnabledFromCmdLine;
    donut::log::info("FGSR DX Sample: FG uses 3 swapchain buffers, no recreate on UI toggle (b3c1b38 path)");
}

void OnFgModeChanged(bool fgEnabled, const std::function<void()>& syncFgConstants)
{
    if (syncFgConstants)
    {
        syncFgConstants();
    }
    donut::log::info("FGSR DX Sample: FG %s — plugin constants synced only", fgEnabled ? "On" : "Off");
}

} // namespace dx
} // namespace fgsr_fg_sample
