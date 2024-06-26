#pragma once
#include <QQmlApplicationEngine>
#include <QQuickView>
#include "AudioEngine.h"


class QQuickWindowContainer;
class PluginQuickView;
class PluginHost;


class App final : public QGuiApplication
{
    Q_OBJECT
    Q_PROPERTY(PluginHost* mainWindowPlugin READ mainWindowPlugin WRITE setMainWindowPlugin NOTIFY mainWindowPluginChanged)

  public:
    static App* instance();

    App(int& argc, char** argv);
    ~App() override;
    App(App&) = delete;
    App(App&&) = delete;
    App(const App&) = delete;
    App(const App&&) = delete;

    QQmlApplicationEngine& getQmlEngine() { return m_qmlEngine; };

    [[nodiscard]] PluginHost* mainWindowPlugin() const;
    void setMainWindowPlugin(PluginHost* plugin);


  signals:
    void mainWindowPluginChanged();


  public slots:
    void informLoadFinished() const;

    bool isNewSession() const;
    void newSession();
    void saveSession();
    void saveSession(const QString& path);
    void loadSession(const QString& path);

    void openPluginBrowserWindow(PluginHost* pluginHostToLoadInto = nullptr);

    void load(PluginHost* plugin, const QString& path, int pluginIndex);

    void setAllFloatingWindowsVisibility(bool visible);


  private:
    QString m_currentSessionPath;

    AudioEngine m_audioEngine;
    QQmlApplicationEngine m_qmlEngine;
    QQuickView m_mainView{&m_qmlEngine, nullptr};

    PluginHost* m_mainWindowPlugin = nullptr;

    std::chrono::time_point<std::chrono::high_resolution_clock> timeStart;
};
