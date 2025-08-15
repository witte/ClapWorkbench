#pragma once
#include <QQmlApplicationEngine>
#include <QQuickView>
#include "AudioEngine.h"


class App final : public QGuiApplication
{
    Q_OBJECT

  public:
    static App* instance();

    App(int& argc, char** argv);
    ~App() override;
    App(App&) = delete;
    App(App&&) = delete;
    App(const App&) = delete;
    App(const App&&) = delete;

    QQmlApplicationEngine& getQmlEngine() { return m_qmlEngine; };


  signals:
    void mainWindowPluginChanged();


  public slots:
    bool isNewSession() const;
    void newSession();
    void saveSession();
    void saveSession(const QString& path);
    void loadSession(const QString& path);

    void openPluginBrowserWindow(Node* channelStrip, Node* pluginHostToLoadInto);


  private:
    QString m_currentSessionPath;

    AudioEngine m_audioEngine;
    QQmlApplicationEngine m_qmlEngine;
    QQuickView m_mainView{&m_qmlEngine, nullptr};
};
