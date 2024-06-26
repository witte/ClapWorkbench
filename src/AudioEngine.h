#pragma once
#include <QObject>
#include "PluginManager.h"
#include "PluginHost.h"
#include "Utils.h"
#include "Utils/RecursiveFileSystemWatcher.h"


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
    Q_PROPERTY(QList<PluginHost*> plugins READ plugins NOTIFY pluginsChanged)
    Q_PROPERTY(float outputVolume READ outputVolume WRITE setOutputVolume NOTIFY outputVolumeChanged)


  public:
    static AudioEngine* instance();

    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine() override;


    [[nodiscard]] PluginManager* pluginManager() const;

    void start();
    void pause();
    void stop();

    void load(QList<std::tuple<QString, int, QString>>&& plugins);

    [[nodiscard]] QList<PluginHost*> plugins();
    void clearPluginsList();

    [[nodiscard]] bool isRunning() const;
    void setIsRunning(bool newIsRunning);

    [[nodiscard]] float outputVolume() const;
    void setOutputVolume(float newOutputVolume);

    [[nodiscard]] QUndoStack& undoStack() const { return *m_undoStack; }


  public slots:
    void load(PluginHost* plugin, const QString& path, int pluginIndex);
    void unload(PluginHost* pluginToUnload);
    void reorder(int from, int to);
    void undo() const;


  signals:
    void pluginsChanged();
    void pluginHostReloaded(PluginHost* pluginHost);
    void pluginHostRemoved(PluginHost* pluginHost);

    void isRunningChanged();
    void outputVolumeChanged();
    void outputDelayChanged();

    void stopRequested();


  private:
    std::unique_ptr<PluginManager> m_pluginManager;
    std::atomic<ocp::Status> m_status = ocp::Status::Inactive;

    static int audioCallback(void* outputBuffer,
                             void* inputBuffer,
                             unsigned int frameCount,
                             double /*streamTime*/,
                             RtAudioStreamStatus /*status*/,
                             void* data);

    int m_sampleRate = 48000;
    unsigned int m_bufferSize = 128;
    std::unique_ptr<RtAudio> m_audio;
    std::unique_ptr<RtMidiIn> m_midiIn;
    std::vector<unsigned char> m_midiInBuffer;

    int m_inputChannelCount = 0;
    int m_outputChannelCount = 0;

    float* m_outputBuffer[2] = {nullptr, nullptr};

    QList<PluginHost*> m_plugins;

    std::atomic<float> m_outputVolume = 0.0f;

    QUndoStack* m_undoStack = nullptr;
    double m_gestureInitialValue = 0.0;

    RecursiveFileSystemWatcher m_pluginWatcher;
};
