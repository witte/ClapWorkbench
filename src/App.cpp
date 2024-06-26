#include "App.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QFontDatabase>
#include <QQuickWindow>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QBuffer>
#include <QFileInfo>
#include "Utils.h"
#include "PluginHost.h"
#include "AudioEngine.h"
#include "Components/PluginQuickView.h"


QString getViewTitleFromSessionName(const QString& filePath)
{
    const QFileInfo fileInfo(filePath);
    QString title = "Clap Workbench - ";
    title += fileInfo.completeBaseName();

    return title;
}

App* m_instance = nullptr;

App* App::instance()
{
    return m_instance;
}

App::App(int& argc, char** argv) : QGuiApplication(argc, argv)
{
    timeStart = std::chrono::high_resolution_clock::now();

    m_instance = this;

    connect(&m_qmlEngine, &QQmlApplicationEngine::objectCreationFailed,
        this, []() { exit(-1); },
        Qt::QueuedConnection
    );

    if (const int fontId = QFontDatabase::addApplicationFont(
        ":/qml/Components/icons.ttf");
        fontId == -1)
    {
        qWarning() << "Failed to load icons font, expect some odds things to appear";
    }

    const QSettings settings{"witte", "ClapWorkbench"};

    constexpr int width = 1200;
    constexpr int height = 800;

    const QRect screenGeometry = primaryScreen()->availableGeometry();
    const int x = (screenGeometry.width()  - width) / 2;
    const int y = (screenGeometry.height() - height) / 2;

    m_qmlEngine.rootContext()->setContextProperty("app", this);
    m_qmlEngine.rootContext()->setContextProperty("audioEngine", &m_audioEngine);

    connect(&m_pluginWatcher, &RecursiveFileSystemWatcher::directoryChanged, &m_pluginWatcher, [this](const QString& path)
    {
        qDebug() << "directoryChanged:" << path;
        for (auto* plugin : m_audioEngine.plugins())
        {
            if (!path.startsWith(plugin->path()))
                continue;

            qDebug() << "reloading:" << plugin->path();
            m_audioEngine.pluginManager()->unload(*plugin, true);
            m_audioEngine.load(plugin, plugin->path(), static_cast<int>(plugin->index()));
        }
    });

    connect(&m_audioEngine, &AudioEngine::pluginHostReloaded, this, [this](PluginHost* pluginHost)
    {
        qDebug() << "AudioEngine::pluginHostReloaded" << pluginHost->name();

        if (pluginHost != m_mainWindowPlugin && !pluginHost->isFloatingWindowOpen())
        {
            pluginHost->setIsFloatingWindowOpen(true);
            return;
        }

        emit mainWindowPluginChanged();
    });

    connect(&m_audioEngine, &AudioEngine::pluginHostRemoved, this, [this](const PluginHost* pluginHost)
    {
        if (pluginHost != m_mainWindowPlugin)
            return;

        m_mainWindowPlugin = nullptr;
        emit mainWindowPluginChanged();
    });


    // on macOS you'll see warnings:
    // "CoreAnimation: setting `contents' on CAMetalLayer may result in undefined behavior"
    // It's a known macOS bug that hasn't caused any real problems so far, besides the
    // annoying warnings themselves.
    // https://bugreports.qt.io/browse/QTBUG-122280
    m_mainView.loadFromModule("ClapWorkbench", "Main");

    connect(&m_mainView, &QQuickView::closing, this, []() { exit(); });

    const auto geometry = settings.value("mainWindow/geometry",
        QRect{x, y, width, height}).toRect();

    m_mainView.setGeometry(geometry);
    m_mainView.setMinimumWidth(480);
    m_mainView.setMinimumHeight(320);

    if (const auto lastLoadedSession = settings.value("lastLoadedSession").toString();
        !lastLoadedSession.isEmpty())
    {
        loadSession(lastLoadedSession);
    }

    const bool isEngineRunning = settings.value("audioEngine/isRunning", false).toBool();
    m_audioEngine.setIsRunning(isEngineRunning);

    m_mainView.show();
}

App::~App()
{
    QSettings settings{"witte", "ClapWorkbench"};
    const auto geomm = m_mainView.geometry();

    settings.setValue("mainWindow/geometry", geomm);
    settings.setValue("audioEngine/isRunning", m_audioEngine.isRunning());
    settings.setValue("lastLoadedSession", m_currentSessionPath);

    uint32_t mainWindowPluginIndex = 0;
    if (!m_audioEngine.plugins().empty())
        mainWindowPluginIndex = m_audioEngine.plugins()[0]->index();

    settings.setValue("audioEngine/pluginIndex", mainWindowPluginIndex);
}

PluginHost* App::mainWindowPlugin() const
{
    return m_mainWindowPlugin;
}

void App::setMainWindowPlugin(PluginHost* plugin)
{
    if (plugin)
        plugin->setIsFloatingWindowOpen(false);

    m_mainWindowPlugin = plugin;
    emit mainWindowPluginChanged();
}

void App::informLoadFinished() const
{
    const std::chrono::time_point<std::chrono::high_resolution_clock> timeEnd = std::chrono::high_resolution_clock::now();
    const auto count = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart);

    qDebug() << "startup time in ms:" << count.count();
}

bool App::isNewSession() const
{
    return m_currentSessionPath.isEmpty();
}

void App::newSession()
{
    setMainWindowPlugin(nullptr);
    m_audioEngine.load({});

    m_currentSessionPath = "";
    m_mainView.setTitle("Clap Workbench - new session");
}

void App::saveSession()
{
    saveSession(m_currentSessionPath);
}

void App::saveSession(const QString& path)
{
    QJsonArray pluginStatesArray;

    for (const auto* pluginHost : m_audioEngine.plugins())
    {
        if (!pluginHost)
            continue;

        QFile tempFile;
        QBuffer buffer(&tempFile);
        buffer.open(QIODevice::WriteOnly);

        QJsonObject pluginStateJson;
        pluginStateJson["path"] = pluginHost->path();
        pluginStateJson["index"] = static_cast<int>(pluginHost->index());
        pluginStateJson["name"] = pluginHost->name();
        pluginStateJson["stateData"] = pluginHost->getState();
        pluginStatesArray.append(pluginStateJson);
    }

    const QJsonDocument jsonDoc(pluginStatesArray);
    QFile file(path.startsWith("file://")? path.mid(7) : path);
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "Failed to open file for writing:" << path;
        return;
    }

    file.write(jsonDoc.toJson());

    m_currentSessionPath = path;
    m_mainView.setTitle(getViewTitleFromSessionName(m_currentSessionPath));
}

void App::loadSession(const QString& path)
{
    QFile file(path.startsWith("file://")? path.mid(7) : path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open file for reading:" << path;
        return;
    }

    const QByteArray jsonData = file.readAll();
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    const QJsonArray pluginStatesArray = jsonDoc.array();

    QList<std::tuple<QString, int, QString>> sessionPlugins;

    for (const auto& value : pluginStatesArray)
    {
        QJsonObject pluginStateJson = value.toObject();

        m_pluginWatcher.addPath(pluginStateJson["path"].toString());

        sessionPlugins.push_back(
        {
            pluginStateJson["path"].toString(),
            pluginStateJson["index"].toInt(),
            pluginStateJson["stateData"].toString()
        });
    }

    setMainWindowPlugin(nullptr);
    m_audioEngine.load(std::move(sessionPlugins));

    if (!m_audioEngine.plugins().isEmpty())
        setMainWindowPlugin(m_audioEngine.plugins().first());

    m_currentSessionPath = path;
    m_mainView.setTitle(getViewTitleFromSessionName(m_currentSessionPath));
}

void App::setAllFloatingWindowsVisibility(const bool visible)
{
    for (const auto& plugin : m_audioEngine.plugins())
    {
        plugin->setIsFloatingWindowVisible(visible);
    }
}
