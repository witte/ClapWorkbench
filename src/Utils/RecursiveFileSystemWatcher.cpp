#include "RecursiveFileSystemWatcher.h"
#include <QDir>


namespace
{

void getAllDirsRecursively(QStringList& dirsList, const QString& dirPath, const int level = 0)
{
    if (level == 4)
    {
        qWarning() << "Watching only up to 3 levels, path" << dirPath << "won't be watched.";
        return;
    }

    dirsList << dirPath;

    QDir dir(dirPath);
    dir.setFilter(QDir::Dirs | QDir::NoSymLinks | QDir::NoDot | QDir::NoDotDot);

    for (const auto& entry : dir.entryInfoList())
        getAllDirsRecursively(dirsList, entry.canonicalFilePath(), level + 1);
};

}


RecursiveFileSystemWatcher::RecursiveFileSystemWatcher(const QString& path)
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_watcher, [this](const QString& path)
    {
        m_lastChangePath = path;
        m_debounceTimer.start();
    });

    connect(&m_debounceTimer, &QTimer::timeout, this, [this]()
    {
        emit directoryChanged(m_lastChangePath);
    });

    if (!path.isEmpty())
        addPath(path);
}

void RecursiveFileSystemWatcher::addPath(const QString& path)
{
    for (const auto& dir : m_watcher.directories())
    {
        if (path == dir)
        {
            qInfo() << "RecursiveFileSystemWatcher path already added: " << path;
            return;
        }
    }

    qDebug() << "RecursiveFileSystemWatcher adding path: " << path;

    QStringList dirsList;
    getAllDirsRecursively(dirsList, path);

    m_watcher.addPaths(dirsList);

    m_debounceTimer.setInterval(50);
    m_debounceTimer.setSingleShot(true);
}

void RecursiveFileSystemWatcher::clearPaths()
{
    m_watcher.removePaths(m_watcher.directories());
}
