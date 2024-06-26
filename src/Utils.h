#pragma once
#include <filesystem>
#include <clap/ext/gui.h>
#include <qwindowdefs.h>
#include <QUrl>


struct clap_plugin_entry;


namespace ocp
{

enum class Status
{
    Inactive,
    OnError,
    OnHold,
    Starting,
    Running,
    StopRequested,
    Stopping
};

std::filesystem::path findBinaryInAppBundle(const std::filesystem::path& bundlePath);

void* clapHandleFromPath(const std::filesystem::path& path);

clap_plugin_entry* clapEntryFromPath(const std::filesystem::path& path);

clap_plugin_entry* clapEntryFromHandle(void* handle);

void releaseHandle(void* handle);

clap_window makeClapWindow(WId window);




}

