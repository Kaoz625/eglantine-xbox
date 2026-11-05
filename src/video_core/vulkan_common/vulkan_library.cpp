// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string>
#include <utility>
#ifdef NXBOX_UWP
#include <windows.h>
#endif

#include "common/dynamic_library.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "video_core/vulkan_common/vulkan_library.h"

namespace Vulkan {

std::shared_ptr<Common::DynamicLibrary> OpenLibrary(
    [[maybe_unused]] Core::Frontend::GraphicsContext* context) {
    LOG_DEBUG(Render_Vulkan, "Looking for a Vulkan library");
#if defined(__ANDROID__) && defined(ARCHITECTURE_arm64)
    // Android manages its Vulkan driver from the frontend.
    return context->GetDriverLibrary();
#else
    auto library = std::make_shared<Common::DynamicLibrary>();
#ifdef __APPLE__
    const auto libvulkan_filename =
        Common::FS::GetBundleDirectory() / "Contents/Frameworks/libvulkan.1.dylib";
    const auto libmoltenvk_filename =
        Common::FS::GetBundleDirectory() / "Contents/Frameworks/libMoltenVK.dylib";
    const char* library_paths[] = {std::getenv("LIBVULKAN_PATH"), libvulkan_filename.c_str(),
                                   libmoltenvk_filename.c_str()};
    // Check if a path to a specific Vulkan library has been specified.
    for (const auto& library_path : library_paths) {
        if (library_path && library->Open(library_path)) {
            break;
        }
    }
#else
#ifdef NXBOX_UWP
    // The Xbox has no Vulkan loader. Mesa's Dozen (Vulkan on D3D12) ships in the package, with the
    // shadps4-xbox DXBC back end the One S needs (its D3D12 takes Shader Model 5.1 only). Dozen
    // reads its options when it loads; the One S driver faults on buffer <-> depth copies.
    for (const auto& [name, value] : {std::pair{"DZN_DXBC", "1"}, std::pair{"DZN_NO_DEPTH_COPIES", "1"},
                                      std::pair{"DZN_DRED", "1"}}) {
        if (GetEnvironmentVariableA(name, nullptr, 0) == 0) {
            SetEnvironmentVariableA(name, value);
        }
    }
    if (library->Open("vulkan_dzn.dll")) {
        LOG_INFO(Render_Vulkan, "Vulkan: the packaged Dozen (vulkan_dzn.dll)");
        return library;
    }
    LOG_ERROR(Render_Vulkan, "vulkan_dzn.dll could not be loaded from the package");
#endif
    std::string filename = Common::DynamicLibrary::GetVersionedFilename("vulkan", 1);
    LOG_DEBUG(Render_Vulkan, "Trying Vulkan library: {}", filename);
    if (!library->Open(filename.c_str())) {
        // Android devices may not have libvulkan.so.1, only libvulkan.so.
        filename = Common::DynamicLibrary::GetVersionedFilename("vulkan");
        LOG_DEBUG(Render_Vulkan, "Trying Vulkan library (second attempt): {}", filename);
        void(library->Open(filename.c_str()));
    }
#endif
    return library;
#endif
}

} // namespace Vulkan
