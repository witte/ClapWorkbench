#include "Utils.h"
#include <iostream>
#include <vector>
#if __APPLE__
#include <dlfcn.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace ocp
{

std::filesystem::path findBinaryInAppBundle(const std::filesystem::path& bundlePath)
{
    const CFURLRef bundleURL = CFURLCreateFromFileSystemRepresentation(nullptr,
        reinterpret_cast<const UInt8*>(bundlePath.c_str()),
        static_cast<long>(bundlePath.string().length()), true);

    const CFURLRef binaryURL = CFBundleCopyExecutableURL(CFBundleCreate(nullptr, bundleURL));

    std::filesystem::path path{};
    if (char binaryPath[PATH_MAX];
        CFURLGetFileSystemRepresentation(binaryURL, true,
                                         reinterpret_cast<UInt8*>(binaryPath), PATH_MAX))
    {
        path = binaryPath;
    }

    CFRelease(binaryURL);
    CFRelease(bundleURL);

    return path;
}

void* clapHandleFromPath(const std::filesystem::path& path)
{
    const std::string _path = findBinaryInAppBundle(path);

    void* handle = dlopen(_path.c_str(), RTLD_LAZY);
    if (!handle)
    {
        std::cerr << "Failed to load bundle: " << dlerror() << std::endl;
        
        return {};
    }

    return handle;
}

clap_plugin_entry* clapEntryFromHandle(void* handle)
{
    const auto entry = static_cast<clap_plugin_entry*>(dlsym(handle, "clap_entry"));
    if (!entry)
    {
        std::cerr << "Failed to find symbol: " << dlerror() << std::endl;

        return {};
    }

    return entry;
}

void releaseHandle(void* handle)
{
    if (dlclose(handle) != 0)
        std::cerr << "Failed to unload bundle: " << dlerror() << std::endl;
}

clap_window makeClapWindow(WId window)
{
    clap_window w{};
#if defined(Q_OS_LINUX)
    w.api = CLAP_WINDOW_API_X11;
    w.x11 = window;
#elif defined(Q_OS_MACOS)
    w.api = CLAP_WINDOW_API_COCOA;
    w.cocoa = reinterpret_cast<clap_nsview>(window);
#elif defined(Q_OS_WIN)
    w.api = CLAP_WINDOW_API_WIN32;
    w.win32 = reinterpret_cast<clap_hwnd>(window);
#endif

    return w;
}


}
