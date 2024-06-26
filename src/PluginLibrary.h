#pragma once
#include <filesystem>
#include <clap/plugin.h>
#include <QLibrary>


struct clap_plugin_entry;
struct clap_plugin_factory;


class PluginLibrary
{
  public:
    PluginLibrary();
    ~PluginLibrary();

    void loadFromPath(const std::filesystem::path& path);
    void unload();

    [[nodiscard]] std::vector<clap_plugin_descriptor> getAllPluginDescriptors();

    [[nodiscard]] clap_plugin_descriptor getPluginDescriptor(int index);
    [[nodiscard]] const clap_plugin* createPluginInstance(const clap_host* host,
        const clap_plugin_descriptor* descriptor);


    [[nodiscard]] const std::filesystem::path& path() { return m_path; }
    [[nodiscard]] bool isLoaded() const { return m_factory != nullptr; };

    [[nodiscard]] int count() const { return m_count; }

    void decreasePluginCount();


  private:
    std::filesystem::path m_path;

    void* m_handle = nullptr;
    clap_plugin_entry* m_entry = nullptr;
    const clap_plugin_factory* m_factory = nullptr;

    bool m_failed = false;
    int m_count = 0;
};
