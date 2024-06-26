#pragma once
#include <QString>
#include <QList>
#include <QMetaType>


class PluginInfo
{
    Q_GADGET
    Q_PROPERTY(QString path     READ path     CONSTANT)
    Q_PROPERTY(QString vendor   READ vendor   CONSTANT)
    Q_PROPERTY(int index        READ index    CONSTANT)
    Q_PROPERTY(QString name     READ name     CONSTANT)
    Q_PROPERTY(QString version  READ version  CONSTANT)
    Q_PROPERTY(QString features READ features CONSTANT)


  public:
    [[nodiscard]] QString path() const { return m_path; }
    void setPath(const QString& path) { m_path = path; }

    [[nodiscard]] QString vendor() const { return m_vendor; }
    void setVendor(const QString& vendor) { m_vendor = vendor; }

    [[nodiscard]] int index() const { return m_index; }
    void setIndex(const int index) { m_index = index; }

    [[nodiscard]] QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    [[nodiscard]] QString version() const { return m_version; }
    void setVersion(const QString& version) { m_version = version; }

    [[nodiscard]] QString features() const { return m_features; };
    void setFeatures(QString&& features) { m_features = features; }


  private:
    QString m_path;
    QString m_vendor;
    int m_index = 0;
    QString m_name;
    QString m_version;
    QString m_features;
};

Q_DECLARE_METATYPE(PluginInfo)

