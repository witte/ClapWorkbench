#pragma once
#include <filesystem>
#include <clap/ext/gui.h>


struct clap_plugin_entry;


namespace ocp
{

enum class Status
{
    Inactive,
    OnError,
    Activating,
    Stopped,
    Running,
    DeactivateRequested,
    Deactivating
};

std::filesystem::path findBinaryInAppBundle(const std::filesystem::path& bundlePath);

void* clapHandleFromPath(const std::filesystem::path& path);

clap_plugin_entry* clapEntryFromPath(const std::filesystem::path& path);

clap_plugin_entry* clapEntryFromHandle(void* handle);

void releaseHandle(void* handle);

clap_window makeClapWindow(void* window);

}
