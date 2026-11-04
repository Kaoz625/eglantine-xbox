// SPDX-License-Identifier: GPL-3.0-or-later
#include "eden_uwp/diagnostic.h"
#include "eden_uwp/dozen_window.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <winrt/Windows.Storage.h>

namespace EdenXbox {
DozenWindow::DozenWindow(const winrt::Windows::UI::Core::CoreWindow& window_, u32 width,
                         u32 height)
    : window{window_} {
    // Mesa logs to stderr and a UWP app has none: Dozen's C runtime takes stderr from the
    // process's standard error handle when it loads, so point that at LocalState\mesa.log
    // (shared: readable while the game runs).
    const std::wstring log_path =
        std::wstring{winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path()} +
        L"\\mesa.log";
    const HANDLE mesa_log = CreateFile2(log_path.c_str(), GENERIC_WRITE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE, CREATE_ALWAYS, nullptr);
    if (mesa_log != INVALID_HANDLE_VALUE) {
        SetStdHandle(STD_ERROR_HANDLE, mesa_log);
    }
    window_info.type = Core::Frontend::WindowSystemType::Windows;
    window_info.render_surface = winrt::get_abi(window);
    window_info.render_surface_scale = 1.0f;
    UpdateCurrentFramebufferLayout(width, height);
    Diagnostic("RENDERER vulkan (Dozen + DXBC) " + std::to_string(width) + "x" +
               std::to_string(height));
}

DozenWindow::~DozenWindow() = default;

std::unique_ptr<Core::Frontend::GraphicsContext> DozenWindow::CreateSharedContext() const {
    // Vulkan needs no GL-style context; the base class is a complete no-op.
    return std::make_unique<Core::Frontend::GraphicsContext>();
}

bool DozenWindow::IsShown() const {
    return true;
}

void DozenWindow::OnFrameDisplayed() {
    frames.fetch_add(1, std::memory_order_relaxed);
}

u64 DozenWindow::FrameCount() const {
    return frames.load(std::memory_order_relaxed);
}

void ApplyEnvFile() {
    const std::filesystem::path env_file =
        std::filesystem::path(winrt::to_string(
            winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path())) /
        "nxbox_env.txt";
    std::ifstream env_in(env_file);
    std::string line;
    while (std::getline(env_in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }
        const auto eq = line.find('=');
        if (line.empty() || line[0] == '#' || eq == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);
        _putenv_s(key.c_str(), value.c_str());
        Diagnostic("ENV " + key + "=" + value);
    }
}

bool UseVulkanRenderer() {
    const char* renderer = std::getenv("NXBOX_RENDERER");
    return renderer == nullptr || std::string_view{renderer} != "opengl";
}
} // namespace EdenXbox
