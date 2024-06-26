#pragma once
#include <QFileSystemWatcher>
#include <QTimer>


class RecursiveFileSystemWatcher final : public QObject
{
    Q_OBJECT


  public:
    explicit RecursiveFileSystemWatcher(const QString& path = {});

    void addPath(const QString& path);

    void clearPaths();

  signals:
    void directoryChanged(const QString& path);


  private:
    QFileSystemWatcher m_watcher;
    QTimer m_debounceTimer;

    QString m_lastChangePath;
};
