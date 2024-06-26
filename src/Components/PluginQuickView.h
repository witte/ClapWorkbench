#pragma once
#include <QQuickView>


class QQmlApplicationEngine;
class PluginHost;


class PluginQuickView final : public QQuickView
{
    Q_OBJECT


  public:
    explicit PluginQuickView(QQmlApplicationEngine& engine, PluginHost& plugin);
    ~PluginQuickView() override;

    [[nodiscard]] PluginHost* plugin() const { return &m_plugin; }


  public slots:
    void addKeyPressListener(QQuickWindow* windowToListenTo);


  private:
    PluginHost& m_plugin;

    void closeEvent(QCloseEvent *) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
};


