#pragma once
#include <filesystem>
#include <unordered_map>
#include <vector>


class RecursiveFileSystemWatcher;
class PluginLibrary;
class PluginHost;


class PluginManager final
{
  public:
    explicit PluginManager();
    ~PluginManager();
    PluginManager(PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager(const PluginManager&) = delete;
    PluginManager(const PluginManager&&) = delete;

    bool load(PluginHost& caller, std::string_view path, uint32_t pluginIndex);
    void unload(PluginHost& pluginHost, bool forceLibraryUnload = false);

  private:
    std::unordered_map<std::filesystem::path, PluginLibrary> m_handles;
};
