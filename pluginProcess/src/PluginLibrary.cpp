#include "PluginLibrary.h"
#include <iostream>
#include "clap/entry.h"
#include "clap/factory/plugin-factory.h"
#include "Utils.h"


PluginLibrary::PluginLibrary() = default;
PluginLibrary::~PluginLibrary() = default;


void PluginLibrary::loadFromPath(const std::filesystem::path& path)
{
    std::cout << "loading library for:" << path.c_str() << std::endl;

    const auto realPath = ocp::findBinaryInAppBundle(path);
    auto* newHandle = ocp::clapHandleFromPath(path);
    auto* newEntry = ocp::clapEntryFromHandle(newHandle);

    if (!newEntry)
    {
        std::cout << "no entry :(" << std::endl;
        m_failed = true;

        return;
    }

    newEntry->init(path.c_str());
    auto* newFactory = static_cast<const clap_plugin_factory*>(newEntry->get_factory(CLAP_PLUGIN_FACTORY_ID));
    if (!newFactory)
    {
        std::cout << "no factory :(" << m_path.c_str() << std::endl;
        newEntry->deinit();
        m_failed = true;

        return;
    }

    m_path = path;
    m_handle = newHandle;
    m_entry = newEntry;
    m_factory = newFactory;
    m_failed = false;
}

void PluginLibrary::unload()
{
    if (!m_entry)
    {
        std::cout << "PluginLibrary already unloaded:" << m_path.c_str() << std::endl;
        return;
    }

    m_entry->deinit();
    ocp::releaseHandle(m_handle);

    m_factory = nullptr;
    m_entry = nullptr;
    m_handle = nullptr;
}

std::vector<clap_plugin_descriptor> PluginLibrary::getAllPluginDescriptors()
{
    std::vector<clap_plugin_descriptor> descriptors;

    const bool shouldUnloadLibraryAfter = !isLoaded();

    if (!isLoaded())
        loadFromPath(m_path);

    const auto pluginCount = m_factory->get_plugin_count(m_factory);
    if (pluginCount <= 0)
    {
        std::cout << "no plugins in factory :(" << m_path.c_str() << std::endl;
        return {};
    }

    for (uint32_t pluginIndex = 0; pluginIndex < pluginCount; ++pluginIndex)
    {
        const auto pluginDescriptor = m_factory->get_plugin_descriptor(m_factory, pluginIndex);
        if (!pluginDescriptor)
        {
            std::cout << "plugin has no descriptor :(" << m_path.c_str() << ", index" << pluginIndex << std::endl;
            continue;
        }

        descriptors.push_back(*pluginDescriptor);
    }

    if (shouldUnloadLibraryAfter)
        unload();

    return descriptors;
}

clap_plugin_descriptor PluginLibrary::getPluginDescriptor(const int index)
{
    if (m_path.empty())
    {
        std::cout << "PluginLibrary has no path set" << std::endl;
        return {};
    }

    if (!isLoaded())
        loadFromPath(m_path);

    if (const auto count = static_cast<int>(m_factory->get_plugin_count(m_factory));
        index > count)
    {
        std::cout << "plugin index greater than count:" << count << std::endl;
        return {};
    }

    const auto desc = m_factory->get_plugin_descriptor(m_factory, index);
    if (!desc)
    {
        std::cout << "no plugin descriptor" << std::endl;
        return {};
    }

    if (!clap_version_is_compatible(desc->clap_version))
    {
        std::cout << "Incompatible clap version: "
            << "Plugin is " << desc->clap_version.major << "." << desc->clap_version.minor << "." << desc->clap_version.revision
            << "Host is " << CLAP_VERSION.major << "." << CLAP_VERSION.minor << "." << CLAP_VERSION.revision << std::endl;

        return {};
    }

    return *desc;
}

const clap_plugin* PluginLibrary::createPluginInstance(const clap_host* host, const clap_plugin_descriptor* descriptor)
{
    const auto plugin = m_factory->create_plugin(m_factory, host, descriptor->id);
    if (!plugin)
    {
        std::cout << "could not create the plugin with id: " << descriptor->id << std::endl;

        return nullptr;
    }

    m_count++;

    return plugin;
}

void PluginLibrary::decreasePluginCount()
{
    m_count--;

    if (m_count == 0)
    {
        unload();
    }
}
