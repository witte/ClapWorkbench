#pragma once
#include "Node.h"
#include "PluginHost.h"


class MidiFilePlayer : public Node
{
    Q_OBJECT
    Q_PROPERTY(float outputVolume READ outputVolume WRITE setOutputVolume NOTIFY outputVolumeChanged)
    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)

  public:
    explicit MidiFilePlayer(Node* parent);
    ~MidiFilePlayer() override;

    void setPorts(int numInputs, float** inputs, int numOutputs, float** outputs) override;
    void activate(std::int32_t sampleRate, std::int32_t blockSize) override;
    void deactivate() override;

    void startProcessing() override;
    void stopProcessing() override;

    void processNoteRawMidi(int sampleOffset, const std::vector<unsigned char>& data) override;
    void process() override;

    [[nodiscard]] float outputVolume() const;
    void setOutputVolume(float newOutputVolume);

    [[nodiscard]] const QString& filePath() const;
    void setFilePath(const QString& newFilePath);


  signals:
    void outputVolumeChanged();
    void filePathChanged();


  public:
    std::atomic<Status> m_status;
    std::atomic<float> m_outputVolume = 0.7f;

    QString m_filePath;

    unsigned int m_bufferSize = 4096;
};
