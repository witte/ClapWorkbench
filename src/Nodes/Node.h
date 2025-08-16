#pragma once
#include <vector>
#include <clap/helpers/event-list.hh>
#include <clap/process.h>
#include <QObject>


enum class S : std::uint8_t
{
    Inactive,
    OnError,
    Stopped,
    Starting,
    Running,
    StopRequested,
    Stopping,
    ToBeDeleted,
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
    Q_PROPERTY(Type type READ type CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool isByPassed READ isByPassed WRITE setIsByPassed NOTIFY isByPassedChanged)
    Q_PROPERTY(QList<Node*> nodes READ nodes NOTIFY nodesChanged)


  public:
    enum class Type
    {
        Root,
        ChannelStrip,
        PluginHost,
        MidiFilePlayer,
    };
    Q_ENUM(Type)

    static Node* create(Node* parent, const QJsonObject& stateToLoad);
    ~Node() override;

    [[nodiscard]] Type type() const;

    [[nodiscard]] QString name() const;
    void setName(const QString& name);

    [[nodiscard]] bool isByPassed() const;
    void setIsByPassed(bool newValue);

    [[nodiscard]] QList<Node*> nodes() const;
    void clearNodes();

    virtual void setPorts(int numInputs, float** inputs, int numOutputs, float** outputs) = 0;
    virtual void activate(std::int32_t sampleRate, std::int32_t blockSize) = 0;
    virtual void deactivate() = 0;

    virtual void startProcessing() = 0;
    virtual void stopProcessing() = 0;

    virtual void processNoteRawMidi(int sampleOffset, const std::vector<unsigned char>& data) = 0;
    virtual void process() = 0;

    virtual QJsonObject getState() const = 0;
    virtual void loadState(const QJsonObject& stateToLoad) = 0;

    QList<Node*> m_nodes;
    std::atomic<int> pendingTopologyChanges = 0;
    std::atomic<Status> status;
    clap_process m_process{};
    clap::helpers::EventList m_evIn;
    clap::helpers::EventList m_evOut;
    float* buffer[2] = {nullptr, nullptr};


  public slots:
    virtual void addNode(const QJsonObject& /*state*/) {}
    virtual void removeNode(const QJsonObject& /*state*/) {}


  signals:
    void nameChanged();
    void isByPassedChanged();
    void nodesChanged();


  protected:
    explicit Node();
    explicit Node(Node* parent, Type type);

    const Type m_type;
    QString m_name;

};
