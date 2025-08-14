#pragma once
#include "clap/helpers/event-list.hh"
#include "clap/process.h"
#include <QObject>
#include <qqmlintegration.h>
#include <vector>


enum class S : std::uint8_t
{
    Inactive,
    OnError,
    Stopped,
    Starting,
    Running,
    StopRequested,
    Stopping,
};

struct Status
{
    S status = S::Inactive;

    bool isBypassed = false;
    bool isProcessed = false;
};


class Node : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool isByPassed READ isByPassed WRITE setIsByPassed NOTIFY isByPassedChanged)

  public:
    explicit Node(QObject* parent = nullptr);
    ~Node() override;

    [[nodiscard]] QString name() const;
    void setName(const QString& name);

    [[nodiscard]] bool isByPassed() const;
    void setIsByPassed(bool newValue);

    virtual void setPorts(int numInputs, float** inputs, int numOutputs, float** outputs) = 0;
    virtual void activate(std::int32_t sampleRate, std::int32_t blockSize) = 0;
    virtual void deactivate() = 0;

    virtual void startProcessing() = 0;
    virtual void stopProcessing() = 0;

    virtual void processNoteRawMidi(int sampleOffset, const std::vector<unsigned char>& data) = 0;
    virtual void process() = 0;

    virtual QJsonObject getState() const = 0;
    virtual void loadState(const QJsonObject& stateToLoad) const = 0;

    std::atomic<Status> status;
    clap_process m_process{};
    clap::helpers::EventList m_evIn;
    clap::helpers::EventList m_evOut;


  public slots:
    virtual void setProperty(const QString& /*name*/, const QString& /*value*/) {}


  signals:
    void nameChanged();
    void isByPassedChanged();


  protected:
    QString m_name;
};
