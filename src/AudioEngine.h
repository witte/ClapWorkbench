#pragma once
#include "ChannelStrip.h"
#include "PluginHost.h"
#include "PluginManager.h"
#include "Utils/RecursiveFileSystemWatcher.h"
#include <QObject>

typedef unsigned int RtAudioStreamStatus;
class RtAudio;
class RtMidiIn;
class PluginInstanceWindow;
class QUndoStack;
class QAction;
class QQuickView;


class AudioEngine final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PluginManager* pluginManager READ pluginManager CONSTANT)
    Q_PROPERTY(bool isRunning READ isRunning WRITE setIsRunning NOTIFY isRunningChanged)
    Q_PROPERTY(QList<ChannelStrip*> channelStrips READ channelStrips NOTIFY channelStripsChanged)
    Q_PROPERTY(double bpm READ bpm WRITE setBpm NOTIFY bpmChanged)
    Q_PROPERTY(float outputVolume READ outputVolume WRITE setOutputVolume NOTIFY outputVolumeChanged)


  public:
    static AudioEngine* instance();

    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine() override;

    [[nodiscard]] PluginManager* pluginManager() const;

    void start();
    void pause();
    void stop();

    void loadSession(const QString& path);

    void clearPluginsList();

    [[nodiscard]] QList<ChannelStrip*> channelStrips() const;

    [[nodiscard]] bool isRunning() const;
    void setIsRunning(bool newIsRunning);

    [[nodiscard]] float outputVolume() const;
    void setOutputVolume(float newOutputVolume);

    [[nodiscard]] double bpm() const;
    void setBpm(double newBpm);

    [[nodiscard]] QUndoStack& undoStack() const { return *m_undoStack; }


  public slots:
    void addNewChannelStrip();
    void unload(PluginHost* pluginToUnload);
    void undo() const;

    void key(int key, bool on);


  signals:
    void channelStripsChanged();

    void pluginsChanged();
    void pluginHostReloaded(PluginHost* pluginHost);
    void pluginHostRemoved(PluginHost* pluginHost);

    void isRunningChanged();
    void outputVolumeChanged();
    void bpmChanged();
    void outputDelayChanged();

    void stopRequested();


  private:
    std::atomic<Status> m_status;

    static int audioCallback(void* outputBuffer,
                             void* inputBuffer,
                             unsigned int frameCount,
                             double /*streamTime*/,
                             RtAudioStreamStatus /*status*/,
                             void* data);

    std::atomic<double> m_bpm = 120.0;
    int m_sampleRate = 48000;
    unsigned int m_bufferSize = 4096;
    std::unique_ptr<RtAudio> m_audio;
    std::unique_ptr<RtMidiIn> m_midiIn;
    std::vector<unsigned char> m_midiInBuffer;

    int m_inputChannelCount = 0;
    int m_outputChannelCount = 0;

    float* m_outputBuffer[2] = {nullptr, nullptr};

    QList<ChannelStrip*> m_channelStrips;

    std::atomic<float> m_outputVolume = 0.3f;

    QUndoStack* m_undoStack = nullptr;
    double m_gestureInitialValue = 0.0;

    RecursiveFileSystemWatcher m_pluginWatcher;
};
