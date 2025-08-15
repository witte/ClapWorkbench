#include "App.h"
#include <QBuffer>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSettings>
#include "Utils.h"
#include "Nodes/ChannelStrip.h"
#include "Nodes/PluginHost.h"
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
}

bool App::isNewSession() const
{
    return m_currentSessionPath.isEmpty();
}

void App::newSession()
{
    m_audioEngine.loadSession("");

    m_currentSessionPath = "";
    m_mainView.setTitle("Clap Workbench - new session");
}

void App::saveSession()
{
    saveSession(m_currentSessionPath);
}

void App::saveSession(const QString& path)
{
    QJsonArray channelStripsArray;

    for (const auto* channel : m_audioEngine.channelStrips())
    {
        QJsonObject channelObject = channel->getState();
        channelStripsArray.append(channelObject);
    }

    const QJsonDocument jsonDoc(channelStripsArray);
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
    m_audioEngine.loadSession(path);

    m_currentSessionPath = path;
    m_mainView.setTitle(getViewTitleFromSessionName(m_currentSessionPath));
}

void App::openPluginBrowserWindow(Node* channelStrip, Node* pluginHostToLoadInto)
{
    auto* pluginBrowserView = new QQuickView(&m_qmlEngine, nullptr);
    pluginBrowserView->setColor(Qt::transparent);
    pluginBrowserView->setFlags(pluginBrowserView->flags() | Qt::FramelessWindowHint | Qt::Window | Qt::WindowStaysOnTopHint);
    pluginBrowserView->setInitialProperties({
        { "channelStrip", QVariant::fromValue(channelStrip) },
        { "plugin", QVariant::fromValue(pluginHostToLoadInto) }
    });
    pluginBrowserView->loadFromModule("ClapWorkbench", "PluginBrowserWindow");
    pluginBrowserView->show();

    connect(pluginBrowserView, &QQuickView::closing, this, [pluginBrowserView]()
    {
        pluginBrowserView->deleteLater();
    });

    connect(this, &QGuiApplication::focusWindowChanged, this, [pluginBrowserView](QWindow* newFocusWindow)
    {
        if (newFocusWindow == pluginBrowserView)
            return;

        pluginBrowserView->close();
    }, Qt::SingleShotConnection);
}
