#pragma once
#include <filesystem>
#include <QObject>
#include "PluginInfo.h"


class QSettings;
class RecursiveFileSystemWatcher;
class PluginLibrary;
class PluginHost;


class PluginManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString pathsToScan READ pathsToScan WRITE setPathsToScan NOTIFY pathsToScanChanged)
    Q_PROPERTY(bool hasFoundPlugins READ hasFoundPlugins NOTIFY hasFoundPluginsChanged)
    Q_PROPERTY(QList<PluginInfo> availablePlugins READ availablePlugins NOTIFY availablePluginsChanged)


  public:
    static PluginManager* instance();
    ~PluginManager() override;
    PluginManager(PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager(const PluginManager&) = delete;
    PluginManager(const PluginManager&&) = delete;

    bool load(PluginHost& caller, const QString& path, uint32_t pluginIndex);
    void unload(PluginHost& pluginHost, bool forceLibraryUnload = false);

    [[nodiscard]] QString pathsToScan() const;
    void setPathsToScan(const QString& newPathsToScan);

    [[nodiscard]] bool hasFoundPlugins() const;

    [[nodiscard]] QList<PluginInfo> availablePlugins();


  public slots:
    void rescanPluginPaths();


  signals:
    void pathsToScanChanged() const;
    void hasFoundPluginsChanged() const;
    void availablePluginsChanged() const;


  private:
    PluginManager();
    std::unique_ptr<QSettings> m_settings;
    QString m_pathsToScan;
    QList<PluginInfo> m_availablePluginsList;

    std::unordered_map<std::filesystem::path, PluginLibrary> m_handles;

    void scanPluginPaths();
};
