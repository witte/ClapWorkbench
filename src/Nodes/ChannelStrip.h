#pragma once
#include "Node.h"
#include "PluginHost.h"
#include <QJsonObject>


class ChannelStrip final : public Node
{
    Q_OBJECT
    Q_PROPERTY(double outputVolume READ outputVolume WRITE setOutputVolume NOTIFY outputVolumeChanged)

  public:
    explicit ChannelStrip(Node* parent);
    ~ChannelStrip() override;

    void setPorts(int numInputs, float** inputs, int numOutputs, float** outputs) override;
    void activate(std::int32_t sampleRate, std::int32_t blockSize) override;
    void deactivate() override;

    void startProcessing() override;
    void stopProcessing() override;

    void processNoteRawMidi(int sampleOffset, const std::vector<unsigned char>& data) override;
    void process() override;

    [[nodiscard]] QJsonObject getState() const override;
    void loadState(const QJsonObject& stateToLoad) override;

    [[nodiscard]] double outputVolume() const;
    void setOutputVolume(double newOutputVolume);


  signals:
    void outputVolumeChanged();


  public slots:
    void load(PluginHost* plugin, const QString& path, int pluginIndex);
    void reorder(int from, int to);


  public:
    std::atomic<double> m_outputVolume = 0.7f;

    unsigned int m_bufferSize = 4096;
    float* m_outputBuffer[2] = {nullptr, nullptr};
};
