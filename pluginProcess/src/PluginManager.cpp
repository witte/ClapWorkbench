#include "PluginManager.h"
#include <iostream>
#include <format>
#include "PluginHost.h"
#include "PluginLibrary.h"


namespace
{


void addClapsFromDirectory(std::filesystem::path&& path, std::vector<std::filesystem::path>& claps)
{
    try
    {
        for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(path))
        {
            if (dir_entry.path().extension().string() == ".clap")
            {
#if __APPLE__
                if (std::filesystem::is_directory(dir_entry.path()))
                {
                    claps.emplace_back(dir_entry.path());
                }
#else
                if (!std::filesystem::is_directory(dir_entry.path()))
                {
                    claps.emplace_back(dir_entry.path());
                }
#endif
            }
        }
    }
    catch (const std::filesystem::filesystem_error&)
    {
    }
}

std::vector<std::filesystem::path> getInstalledClapPlugins(const std::string_view userPaths)
{
    std::vector<std::filesystem::path> claps;

    const char* home = std::getenv("HOME");
    auto path = std::filesystem::path{home}/"Library/Audio/Plug-Ins/CLAP";
    addClapsFromDirectory(std::move(path), claps);

    addClapsFromDirectory("/Library/Audio/Plug-Ins/CLAP", claps);
    addClapsFromDirectory("/Users/witte/Work/clap/build-ClapSamplePlayer-Capt_Qt_6_6_1_macOS-Release", claps);
    addClapsFromDirectory("/Users/witte/Work/other/iPlug2/cmake-build-release", claps);
    addClapsFromDirectory("/Users/witte/Work/other/iPlug2/cmake-build-debug", claps);
    addClapsFromDirectory("/Users/witte/Tests/visage/cmake-build-release", claps);
    addClapsFromDirectory("/Users/witte/Work/clap/SamplePlayer/cmake-build-debug", claps);

    // for (const auto pathsList = userPaths.split(';'); const auto& userPath : pathsList)
    // {
    //     path = userPath.toStdString();
    //     addClapsFromDirectory(path, claps);
    // }

    return claps;
}

// QString validatePathsToScan(const QString& paths)
// {
//     auto pathsList = paths.split(';');
//
//     const auto newBegin = std::ranges::remove_if(pathsList, [](const QString& path)
//     {
//         const auto exists = std::filesystem::exists(path.toStdString());
//         if (!exists)
//             qWarning() << "Cannot scan '" << path << "': path doesn't exist";
//
//         return !exists;
//     }).begin();
//
//     const auto constNewEnd = static_cast<QList<QString>::const_iterator>(newBegin);
//     pathsList.erase(constNewEnd, pathsList.cend());
//
//     return pathsList.join(';');
// }

}


PluginManager::PluginManager() = default;
PluginManager::~PluginManager() = default;


bool PluginManager::load(PluginHost& caller, const std::string_view path, const uint32_t pluginIndex)
{
    unload(caller);

    caller.m_pluginPath = path;

    const std::filesystem::path pluginPath = caller.m_pluginPath;

    caller.m_pluginPathAsPath = pluginPath;


    auto pluginIterator = m_handles.find(pluginPath);
    if (pluginIterator == m_handles.end())
    {
        PluginLibrary library;
        library.loadFromPath(caller.m_pluginPathAsPath);
        pluginIterator = m_handles.emplace(pluginPath, std::move(library)).first;
    }

    PluginLibrary& library = pluginIterator->second;

    const auto descriptor = library.getPluginDescriptor(static_cast<int>(pluginIndex));
    const auto plugin = library.createPluginInstance(caller.clapHost(), &descriptor);
    if (!plugin)
    {
        std::cout << "could not create plugin with id: " << descriptor.id << std::endl;
        caller.m_status = ocp::Status::OnError;

        return false;
    }

    auto pluginProxy = std::make_unique<PluginProxy>(*plugin, caller);

    if (!pluginProxy->init())
    {
        std::cout << "could not initialize plugin with id: " << descriptor.id << std::endl;
        caller.m_plugin.reset();
        caller.m_status = ocp::Status::OnError;

        return false;
    }

    caller.m_name = descriptor.name;
    caller.m_plugin = std::move(pluginProxy);

    caller.m_index = pluginIndex;

    return true;
}

void PluginManager::unload(PluginHost& pluginHost, [[maybe_unused]] const bool forceLibraryUnload)
{
    if (!pluginHost.m_plugin)
        return;

    if (pluginHost.m_status >= ocp::Status::Stopped)
        pluginHost.deactivate();

    if (pluginHost.m_plugin)
    {
        if (pluginHost.m_plugin->canUseGui())
            pluginHost.destroyNativeGuiWindow();

        pluginHost.m_plugin->destroy();
        pluginHost.m_plugin.reset();
    }

    const auto& pluginPath = pluginHost.m_pluginPathAsPath;

    auto pluginIterator = m_handles.find(pluginPath);
    if (pluginIterator == m_handles.end())
    {
        std::cout << "Plugin not found in PluginManager:" << pluginHost.name() << pluginHost.m_pluginPathAsPath.c_str() << std::endl;
        return;
    }

    PluginLibrary& library = pluginIterator->second;
    library.decreasePluginCount();
}
