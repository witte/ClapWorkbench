#include "AudioEngine.h"
#include <rtaudio/RtAudio.h>
#include <rtmidi/RtMidi.h>
#include <QUndoStack>
#include <QAction>
#include <QQuickView>
#include <QTimer>


AudioEngine* inst_ = nullptr;

AudioEngine* AudioEngine::instance()
{
    return inst_;
}

AudioEngine::AudioEngine(QObject* parent)
    : QObject{parent}
    , m_pluginManager{std::make_unique<PluginManager>()}
{
    inst_ = this;

    m_outputBuffer[0] = static_cast<float*>(std::calloc(1, m_bufferSize * 4));
    m_outputBuffer[1] = static_cast<float*>(std::calloc(1, m_bufferSize * 4));

    connect(this, &AudioEngine::stopRequested, this, [this]()
    {
        qDebug() << "AudioEngine::stopRequested:";
        m_status = ocp::Status::OnHold;

        if (m_audio->isStreamOpen())
        {
            m_audio->stopStream();
            m_audio->closeStream();
        }

        for (auto* plugin: m_plugins)
            plugin->deactivate();

        m_audio.reset();


        emit isRunningChanged();
    });

    connect(&m_pluginWatcher, &RecursiveFileSystemWatcher::directoryChanged, &m_pluginWatcher, [this](const QString& path)
    {
        qDebug() << "directoryChanged:" << path;
        for (auto* plugin : m_plugins)
        {
            if (!path.startsWith(plugin->path()))
                continue;

            qDebug() << "reloading:" << plugin->path();
            m_pluginManager->unload(*plugin, true);
            load(plugin, plugin->path(), static_cast<int>(plugin->index()));
        }
    });

    m_undoStack = new QUndoStack(this);
}

AudioEngine::~AudioEngine()
{
    if (m_audio && m_audio->isStreamOpen())
    {
        m_audio->stopStream();
        m_audio->closeStream();
    }

    free(m_outputBuffer[0]);
    free(m_outputBuffer[1]);

    m_outputBuffer[0] = nullptr;
    m_outputBuffer[1] = nullptr;

    clearPluginsList();
}

PluginManager* AudioEngine::pluginManager() const
{
    return m_pluginManager.get();
}

void AudioEngine::stop()
{
    if (!m_audio)
        return;

    m_status = ocp::Status::StopRequested;
}

QList<PluginHost*> AudioEngine::plugins()
{
    return m_plugins;
}

void AudioEngine::clearPluginsList()
{
    for (const auto* pluginHost : m_plugins)
        delete pluginHost;

    m_plugins.clear();
    emit pluginsChanged();
}

void AudioEngine::start()
{
    m_audio = std::make_unique<RtAudio>();
    if (!m_audio)
    {
        std::cout << "no audio :(" << std::endl;
        m_status = ocp::Status::Inactive;
        emit isRunningChanged();

        return;
    }

    m_midiIn = std::make_unique<RtMidiIn>();
    for (unsigned int i = 0; i < m_midiIn->getPortCount(); ++i)
    {
        std::string portName = m_midiIn->getPortName(i);
        std::cout << "  Input Port #" << i << ": " << portName << '\n';

        if (i > 0)
            m_midiIn->openPort(i);
    }

    RtAudio::StreamParameters inParams;
    inParams.deviceId = m_audio->getDefaultInputDevice();

    const auto inputDeviceInfo = m_audio->getDeviceInfo(inParams.deviceId);
    m_inputChannelCount = static_cast<int>(inputDeviceInfo.inputChannels);
    if (m_inputChannelCount > 2)
        m_inputChannelCount = 2;

    inParams.firstChannel = 0;
    inParams.nChannels = m_inputChannelCount;

    RtAudio::StreamParameters outParams;
    outParams.deviceId = m_audio->getDefaultOutputDevice();

    const auto outputDeviceInfo = m_audio->getDeviceInfo(outParams.deviceId);
    m_outputChannelCount = static_cast<int>(outputDeviceInfo.outputChannels);
    if (m_outputChannelCount > 2)
        m_outputChannelCount = 2;

    outParams.firstChannel = 0;
    outParams.nChannels = m_outputChannelCount;

    m_audio->openStream(&outParams,
                       &inParams,
                       RTAUDIO_FLOAT32,
                       m_sampleRate,
                       &m_bufferSize,
                       &AudioEngine::audioCallback,
                       this);

    for (unsigned int i = 0; i < m_bufferSize; ++i)
    {
        m_outputBuffer[0][i] = 0.0f;
        m_outputBuffer[1][i] = 0.0f;
    }

    for (auto* plugin : m_plugins)
    {
        plugin->setPorts(2, m_outputBuffer, 2, m_outputBuffer);
        plugin->activate(m_sampleRate, static_cast<int>(m_bufferSize));
    }

    m_audio->startStream();

    m_status = ocp::Status::Starting;
    emit isRunningChanged();
}

void AudioEngine::pause()
{
    if (m_status > ocp::Status::OnHold)
        m_status = ocp::Status::OnHold;
}

void AudioEngine::unload(PluginHost* pluginToUnload)
{
    const auto indexOfPluginToRemove = m_plugins.indexOf(pluginToUnload);
    if (indexOfPluginToRemove < 0)
        return;

    const auto oldStatus = m_status.load();
    m_status = ocp::Status::OnHold;

    QTimer::singleShot(200, this, [this, indexOfPluginToRemove, oldStatus, pluginToUnload]()
    {
        m_plugins.remove(indexOfPluginToRemove);

        emit pluginHostRemoved(pluginToUnload);
        emit pluginsChanged();

        QTimer::singleShot(900, this, [this, oldStatus, pluginToUnload]()
        {
            m_status = oldStatus;

            m_pluginManager->unload(*pluginToUnload);
            delete pluginToUnload;
        });
    });
}

void AudioEngine::reorder(int from, int to)
{
    const auto oldStatus = m_status.load();

    auto reorderPlugin = [this, oldStatus, from, to]() -> void
    {
        const auto element = m_plugins.takeAt(from);
        m_plugins.insert(to, element);

        emit pluginsChanged();

        if (oldStatus > ocp::Status::OnHold)
            start();
    };

    if (m_status > ocp::Status::OnHold)
    {
        connect(this, &AudioEngine::isRunningChanged, this, reorderPlugin, Qt::SingleShotConnection);
        stop();

        return;
    }

    reorderPlugin();
}

void AudioEngine::load(PluginHost* plugin, const QString& path, const int pluginIndex)
{
    if (plugin)
        plugin->setIsByPassed(true);

    PluginHost* pluginToReload = plugin;

    if (pluginToReload == nullptr)
    {
        m_plugins.push_back(new PluginHost);
        pluginToReload = m_plugins.back();
    }

    m_pluginManager->load(*pluginToReload, path, pluginIndex);
    m_pluginWatcher.addPath(path);

    if (m_status > ocp::Status::OnHold)
    {
        pluginToReload->setPorts(2, m_outputBuffer, 2, m_outputBuffer);
        pluginToReload->activate(m_sampleRate, static_cast<int>(m_bufferSize));
    }

    pluginToReload->setIsByPassed(false);

    emit pluginsChanged();
    emit pluginToReload->nameChanged();
    emit pluginToReload->hasNativeGUIChanged();
    emit pluginHostReloaded(pluginToReload);
}

void AudioEngine::load(QList<std::tuple<QString, int, QString>>&& plugins)
{
    clearPluginsList();

    for (const auto&[path, index, stateData] : plugins)
    {
        auto* plugin = new PluginHost;
        m_pluginManager->load(*plugin, path, index);

        plugin->loadPluginState(stateData);

        m_plugins.push_back(plugin);
        m_pluginWatcher.addPath(path);

        emit plugin->nameChanged();
        emit plugin->hasNativeGUIChanged();
    }

    emit pluginsChanged();
}

bool AudioEngine::isRunning() const
{
    return m_status > ocp::Status::OnHold;
}

void AudioEngine::setIsRunning(const bool newIsRunning)
{
    if (newIsRunning)
    {
        if (m_status == ocp::Status::Starting || m_status == ocp::Status::Running)
            return;

        start();
        return;
    }

    if (m_status != ocp::Status::Starting && m_status != ocp::Status::Running)
        return;

    stop();
}

float AudioEngine::outputVolume() const
{
    return m_outputVolume;
}

void AudioEngine::setOutputVolume(const float newOutputVolume)
{
    if (qFuzzyCompare(newOutputVolume, m_outputVolume))
        return;

    m_outputVolume = newOutputVolume;

    emit outputVolumeChanged();
}

void AudioEngine::undo() const
{
    m_undoStack->undo();
}

int AudioEngine::audioCallback(void* outputBuffer, void*, const unsigned int frameCount,
                               const double currentTime, RtAudioStreamStatus,
                               void* data)
{
    auto* engine = static_cast<AudioEngine*>(data);
    const auto status = engine->m_status.load();

    auto* out = static_cast<float*>(outputBuffer);

    if (status == ocp::Status::OnHold)
    {
        for (unsigned int i = 0; i < frameCount; ++i)
        {
            out[2 * i]     = 0.0f;
            out[2 * i + 1] = 0.0f;
        }

        return 0;
    }

    if (status == ocp::Status::StopRequested)
    {
        for (auto* plugin : engine->m_plugins)
            plugin->stopProcessing();

        engine->m_status = ocp::Status::Stopping;
        emit engine->stopRequested();

        return 0;
    }

    if (status == ocp::Status::Starting)
    {
        for (auto* plugin : engine->m_plugins)
        {
            if (plugin->status() >= ocp::Status::OnHold)
                plugin->startProcessing();
        }

        engine->m_status = ocp::Status::Running;
    }

    for (unsigned int i = 0; i < frameCount; ++i)
    {
        engine->m_outputBuffer[0][i] = 0.0f;
        engine->m_outputBuffer[1][i] = 0.0f;
    }

    // Midi
    {
        auto& midiBuffer = engine->m_midiInBuffer;
        while (engine->m_midiIn && engine->m_midiIn->isPortOpen())
        {
            const auto msgTime = engine->m_midiIn->getMessage(&midiBuffer);
            if (midiBuffer.empty())
                break;

            const double deltaMs = currentTime - msgTime;
            double deltaSample = (deltaMs * engine->m_sampleRate) / 1000;

            if (deltaSample >= frameCount)
                deltaSample = frameCount - 1;

            const int32_t sampleOffset = static_cast<int>(frameCount) - static_cast<int>(deltaSample);

            for (auto* plugin : engine->m_plugins)
                plugin->processNoteRawMidi(sampleOffset, midiBuffer);
        }
    }

    for (auto* plugin : engine->m_plugins)
        plugin->process();


    static int warnVolumeError = 1;
    auto outputVolume = engine->m_outputVolume.load();

    for (unsigned int i = 0; i < frameCount; ++i)
    {
        --warnVolumeError;

        if (engine->m_outputBuffer[0][i] > 1.0f || engine->m_outputBuffer[1][i] > 1.0f)
        {
            if (warnVolumeError <= 0)
            {
                qWarning() << "trash at " << i << ": " << engine->m_outputBuffer[0][i] << ", " << engine->m_outputBuffer[1][i];
                warnVolumeError = 24000;
            }

            engine->setOutputVolume(0.0f);
            outputVolume = 0.0f;
        }

        out[2 * i]     = engine->m_outputBuffer[0][i] * outputVolume;
        out[2 * i + 1] = engine->m_outputBuffer[1][i] * outputVolume;
    }

    return 0;
}
