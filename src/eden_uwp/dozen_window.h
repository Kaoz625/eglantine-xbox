// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <memory>
#include <windows.h>
#include <winrt/Windows.UI.Core.h>

#include "core/frontend/emu_window.h"
#include "core/frontend/graphics_context.h"

namespace EdenXbox {
/// The Vulkan renderer's window on the Xbox One S: Mesa's Dozen (Vulkan on D3D12, with the
/// shadps4-xbox DXBC back end) presents to the CoreWindow itself. The window hands Eden a Win32
/// surface carrying the CoreWindow's IUnknown *, which that Dozen takes as a CoreWindow swapchain.
class DozenWindow final : public Core::Frontend::EmuWindow {
public:
    DozenWindow(const winrt::Windows::UI::Core::CoreWindow& window, u32 width, u32 height);
    ~DozenWindow() override;
    std::unique_ptr<Core::Frontend::GraphicsContext> CreateSharedContext() const override;
    bool IsShown() const override;
    void OnFrameDisplayed() override;
    u64 FrameCount() const;

private:
    winrt::Windows::UI::Core::CoreWindow window;
    std::atomic<u64> frames{0};
};

/// Applies LocalState\nxbox_env.txt (KEY=VALUE lines) to the process environment, where Eden,
/// Dozen and Mesa read their switches. Mesa's OpenGL runtime applies it as well when it starts.
void ApplyEnvFile();

/// Vulkan through Dozen, unless nxbox_env.txt has NXBOX_RENDERER=opengl (NXbox's OpenGL on
/// Mesa d3d12, which needs Shader Model 6: Series X|S only). Call after ApplyEnvFile().
bool UseVulkanRenderer();
} // namespace EdenXbox
