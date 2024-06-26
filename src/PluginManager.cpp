#include "PluginManager.h"
#include "PluginHost.h"
#include "PluginLibrary.h"
#include <QSettings>
#include <QTimer>


namespace
{
constexpr auto* pathsToScanKey = "PluginBrowser/pathsToScan";


void addClapsFromDirectory(std::filesystem::path& path, std::vector<std::filesystem::path>& claps)
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

std::vector<std::filesystem::path> getInstalledClapPlugins(const QString& userPaths)
{
    std::vector<std::filesystem::path> claps;

    const char* home = std::getenv("HOME");
    auto path = std::filesystem::path{home}/"Library/Audio/Plug-Ins/CLAP";
    addClapsFromDirectory(path, claps);

    path = std::filesystem::path{"/Library/Audio/Plug-Ins/CLAP"};
    addClapsFromDirectory(path, claps);

    for (const auto pathsList = userPaths.split(';'); const auto& userPath : pathsList)
    {
        path = userPath.toStdString();
        addClapsFromDirectory(path, claps);
    }

    return claps;
}

QString validatePathsToScan(const QString& paths)
{
    auto pathsList = paths.split(';');

    const auto newBegin = std::ranges::remove_if(pathsList, [](const QString& path)
    {
        const auto exists = std::filesystem::exists(path.toStdString());
        if (!exists)
            qWarning() << "Cannot scan '" << path << "': path doesn't exist";

        return !exists;
    }).begin();

    const auto constNewEnd = static_cast<QList<QString>::const_iterator>(newBegin);
    pathsList.erase(constNewEnd, pathsList.cend());

    return pathsList.join(';');
}

PluginInfo pluginInfoFromDescriptor(const std::filesystem::path& path, int index,
    const clap_plugin_descriptor& descriptor)
{
    PluginInfo info;
    info.setPath(path.c_str());
    info.setIndex(index);

    info.setName(descriptor.name);
    info.setVendor(descriptor.vendor);
    info.setVersion(descriptor.version);

    QString pluginFeatures;
    for (const char* const* feature = descriptor.features; *feature != nullptr; ++feature)
    {
        if (pluginFeatures.isEmpty())
            pluginFeatures.append("(");
        else
            pluginFeatures.append(", ");

        pluginFeatures.append(*feature);

        pluginFeatures.push_back(*feature);
    }

    if (!pluginFeatures.isEmpty())
        pluginFeatures.append(")");

    info.setFeatures(std::move(pluginFeatures));

    return info;
}


}


PluginManager::PluginManager()
    : QObject{nullptr}
    , m_settings{std::make_unique<QSettings>("witte", "ClapWorkbench")}
    , m_pathsToScan{m_settings->value(pathsToScanKey, QString{}).toString()}
{}

PluginManager::~PluginManager()
{
    m_settings->setValue(pathsToScanKey, m_pathsToScan);
}


bool PluginManager::load(PluginHost& caller, const QString& path, const uint32_t pluginIndex)
{
    unload(caller);

    caller.m_pluginPath = path.startsWith("file://")? path.mid(7) : path;

    const std::filesystem::path pluginPath = caller.m_pluginPath.toStdString();

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
        qWarning() << "could not create plugin with id: " << descriptor.id;
        caller.m_status = ocp::Status::OnError;

        return false;
    }

    auto pluginProxy = std::make_unique<PluginProxy>(*plugin, caller);

    if (!pluginProxy->init())
    {
        qWarning() << "could not initialize plugin with id: " << descriptor.id;
        caller.m_status = ocp::Status::OnError;
        caller.m_plugin.reset();

        return false;
    }

    caller.m_name = descriptor.name;
    caller.m_plugin = std::move(pluginProxy);
    caller.m_parameterModel = std::make_unique<ParameterModel>(caller, *caller.m_plugin);
    caller.m_index = pluginIndex;

    emit caller.hostedPluginChanged();

    return true;
}

void PluginManager::unload(PluginHost& pluginHost, [[maybe_unused]] const bool forceLibraryUnload)
{
    if (!pluginHost.m_plugin)
        return;

    if (pluginHost.m_status >= ocp::Status::OnHold)
        pluginHost.deactivate();

    pluginHost.m_parameterModel.reset();

    if (pluginHost.m_plugin)
    {
        if (pluginHost.m_plugin->canUseGui())
            pluginHost.destroyGuiWindow();

        pluginHost.m_plugin->destroy();
        pluginHost.m_plugin.reset();
    }

    const auto& pluginPath = pluginHost.m_pluginPathAsPath;

    auto pluginIterator = m_handles.find(pluginPath);
    if (pluginIterator == m_handles.end())
    {
        qInfo() << "Plugin not found in PluginManager:" << pluginHost.name() << pluginHost.m_pluginPathAsPath.c_str();
        return;
    }

    if (forceLibraryUnload)
    {
        PluginLibrary& library = pluginIterator->second;
        library.decreasePluginCount();

        if (library.count() == 0)
            m_handles.erase(pluginIterator);
    }
    else
    {
        QTimer::singleShot(2000, this, [pluginIterator]()
        {
            PluginLibrary& library = pluginIterator->second;
            library.decreasePluginCount();
        });
    }
}

QString PluginManager::pathsToScan() const
{
    return m_pathsToScan;
}

void PluginManager::setPathsToScan(const QString& newPathsToScan)
{
    if (newPathsToScan == m_pathsToScan)
        return;

    m_pathsToScan = validatePathsToScan(newPathsToScan);
    emit pathsToScanChanged();
}

bool PluginManager::hasFoundPlugins() const
{
    return !m_availablePluginsList.isEmpty();
}

QList<PluginInfo> PluginManager::availablePlugins()
{
    if (m_availablePluginsList.isEmpty())
        scanPluginPaths();

    return m_availablePluginsList;
}

void PluginManager::rescanPluginPaths()
{
    qDebug() << "Rescanning plugin paths...";
    scanPluginPaths();

    emit hasFoundPluginsChanged();
    emit availablePluginsChanged();
}

void PluginManager::scanPluginPaths()
{
    m_availablePluginsList.clear();

    auto addPluginsToList = [](QList<PluginInfo>& list, PluginLibrary& library) -> void
    {
        const auto descriptors = library.getAllPluginDescriptors();

        for (uint32_t pluginIndex = 0; pluginIndex < descriptors.size(); ++pluginIndex)
        {
            PluginInfo info = pluginInfoFromDescriptor(library.path(), static_cast<int>(pluginIndex),
                descriptors[pluginIndex]);

            list.push_back(info);
        }
    };


    for (const auto installedPlugins = ::getInstalledClapPlugins(m_pathsToScan);
         const auto& pluginPath : installedPlugins)
    {
        if (const auto handlesIterator = m_handles.find(pluginPath); handlesIterator != m_handles.end())
        {
            PluginLibrary& library = handlesIterator->second;
            addPluginsToList(m_availablePluginsList, library);

            continue;
        }

        PluginLibrary library;
        library.loadFromPath(pluginPath);

        addPluginsToList(m_availablePluginsList, library);
        library.unload();
    }
}
