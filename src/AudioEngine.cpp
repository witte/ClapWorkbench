#include "AudioEngine.h"
#include <QAction>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickView>
#include <QTimer>
#include <QUndoStack>
#include <rtaudio/RtAudio.h>
#include <rtmidi/RtMidi.h>

AudioEngine* inst_ = nullptr;

AudioEngine* AudioEngine::instance()
{
    return inst_;
}

AudioEngine::AudioEngine(QObject* parent) : QObject{parent}
{
    inst_ = this;

    m_outputBuffer[0] = static_cast<float*>(std::calloc(1, m_bufferSize * 4));
    m_outputBuffer[1] = static_cast<float*>(std::calloc(1, m_bufferSize * 4));

    connect(this, &AudioEngine::stopRequested, this, [this]()
    {
        qDebug() << "AudioEngine::stopRequested:";
        auto curStatus = m_status.load();
        curStatus.status = S::Stopped;
        m_status.store(curStatus);

        if (m_audio->isStreamOpen())
        {
            m_audio->stopStream();
            m_audio->closeStream();
        }

        for (auto* channelStrip : m_channelStrips)
            channelStrip->deactivate();

        m_audio.reset();

        emit isRunningChanged();
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
    return PluginManager::instance();
}

void AudioEngine::stop()
{
    if (!m_audio)
        return;

    auto curStatus = m_status.load();
    curStatus.status = S::StopRequested;
    m_status.store(curStatus);
}

void AudioEngine::loadSession(const QString& path)
{
    clearPluginsList();

    if (path.isEmpty())
        return;

    QFile file(path.startsWith("file://")? path.mid(7) : path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open file for reading:" << path;
        return;
    }

    const auto jsonData = file.readAll();
    const auto jsonDoc = QJsonDocument::fromJson(jsonData);

    for (const auto channels = jsonDoc.array(); const auto& jsChannelRef : channels)
    {
        auto jsChannel = jsChannelRef.toObject();

        addNewChannelStrip();
        const auto& channelStrip = m_channelStrips.back();

        Status status;
        status.isBypassed = jsChannel["isBypassed"].toBool();
        channelStrip->status.store(status);
        channelStrip->setOutputVolume(jsChannel["outputVolume"].toDouble());

        for (const auto plugins = jsChannel["nodes"].toArray(); const auto& jsPluginRef : plugins)
        {
            const auto jsPlugin = jsPluginRef.toObject();

            const auto pluginPath = jsPlugin["path"].toString();
            const auto pluginIndex = jsPlugin["index"].toInt();
            const auto pluginStateData = jsPlugin["stateData"].toString();

            auto* plugin = new PluginHost;
            PluginManager::instance()->load(*plugin, pluginPath, pluginIndex);
            plugin->loadPluginState(pluginStateData);

            Status pluginStatus;
            pluginStatus.isBypassed = jsPlugin["isBypassed"].toBool();
            plugin->status.store(pluginStatus);

            channelStrip->nodes.push_back(plugin);
        }

        emit channelStrip->pluginsChanged();
    }
}

QList<ChannelStrip*> AudioEngine::channelStrips() const
{
    return m_channelStrips;
}

void AudioEngine::clearPluginsList()
{
    for (auto* channelStrip : m_channelStrips)
    {
        channelStrip->clearNodes();
        emit channelStrip->pluginsChanged();
    }
}

void AudioEngine::start()
{
    m_audio = std::make_unique<RtAudio>();
    if (!m_audio)
    {
        std::cout << "no audio :(" << std::endl;
        auto curStatus = m_status.load();
        curStatus.status = S::Inactive;
        m_status.store(curStatus);
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

    for (auto* channelStrip : m_channelStrips)
    {
        channelStrip->setPorts(2, m_outputBuffer, 2, m_outputBuffer);
        channelStrip->activate(m_sampleRate, static_cast<int>(m_bufferSize));
    }

    m_audio->startStream();

    auto curStatus = m_status.load();
    curStatus.status = S::Starting;
    m_status.store(curStatus);
    emit isRunningChanged();
}

void AudioEngine::pause()
{
    if (auto curStatus = m_status.load(); curStatus.status > S::Stopped)
    {
        curStatus.status = S::Stopped;
        m_status.store(curStatus);
    }
}

void AudioEngine::addNewChannelStrip()
{
    auto oldStatus = m_status.load();
    const auto curStatusStatus = oldStatus.status;
    oldStatus.status = S::Stopped;
    m_status.store(oldStatus);

    m_channelStrips.append(new ChannelStrip);

    oldStatus.status = curStatusStatus;
    m_status.store(oldStatus);
    emit channelStripsChanged();
}

void AudioEngine::unload(PluginHost* pluginToUnload)
{
    ChannelStrip* channelStripToChange = nullptr;
    auto indexOfPluginToRemove = -1LL;

    for (auto* channelStrip : m_channelStrips)
    {
        channelStripToChange = channelStrip;
        indexOfPluginToRemove = channelStrip->nodes.indexOf(pluginToUnload);

        if (indexOfPluginToRemove >= 0)
            break;
    }

    if (!channelStripToChange || indexOfPluginToRemove < 0)
        return;


    const auto oldStatus = m_status.load();
    auto curStatus = m_status.load();
    curStatus.status = S::Stopped;
    m_status.store(curStatus);

    channelStripToChange->nodes.remove(indexOfPluginToRemove);

    emit pluginHostRemoved(pluginToUnload);
    emit channelStripToChange->pluginsChanged();

    m_status = oldStatus;

    QTimer::singleShot(900, this, [pluginToUnload]()
    {
        PluginManager::instance()->unload(*pluginToUnload);
        delete pluginToUnload;
    });
}

bool AudioEngine::isRunning() const
{
    return m_status.load().status > S::Stopped;
}

void AudioEngine::setIsRunning(const bool newIsRunning)
{
    const auto curStatus = m_status.load();

    if (newIsRunning)
    {
        if (curStatus.status == S::Starting || curStatus.status == S::Running)
            return;

        start();
        return;
    }

    if (curStatus.status != S::Starting && curStatus.status != S::Running)
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

double AudioEngine::bpm() const { return m_bpm; }

void AudioEngine::setBpm(const double newBpm)
{
    if (qFuzzyCompare(newBpm, m_bpm))
        return;

    m_bpm = newBpm;

    emit bpmChanged();
}

void AudioEngine::undo() const { m_undoStack->undo(); }

void AudioEngine::key(const int key, const bool on)
{
    if (on)
        qDebug() << "AudioEngine::key(" << key << ")";

    // switch (key)
    // {
    // case 90:
    // case 83:
    // case 88:
    // case 68:
    // case 67:
    // case 86:
    // case 71:
    // case 66:
    // case 72:
    // case 78:
    // case 74:
    // case 77:
    // case 44:
    // }
    //
    // for (auto* channelStrip : engine->m_channelStrips)
    //     channelStrip->processNoteRawMidi(sampleOffset, midiBuffer);
}

int AudioEngine::audioCallback(void* outputBuffer, void*, const unsigned int frameCount,
                               const double currentTime, RtAudioStreamStatus,
                               void* data)
{
    auto* engine = static_cast<AudioEngine*>(data);
    auto status = engine->m_status.load();

    auto* out = static_cast<float*>(outputBuffer);

    if (status.status == S::Stopped)
    {
        for (unsigned int i = 0; i < frameCount; ++i)
        {
            out[2 * i]     = 0.0f;
            out[2 * i + 1] = 0.0f;
        }

        return 0;
    }

    if (status.status == S::StopRequested)
    {
        for (auto* channelStrip : engine->m_channelStrips)
            channelStrip->stopProcessing();

        status.status = S::Stopping;
        engine->m_status.store(status);
        emit engine->stopRequested();

        return 0;
    }

    if (status.status == S::Starting)
    {
        for (auto* channelStrip : engine->m_channelStrips)
            channelStrip->startProcessing();

        status.status = S::Running;
        engine->m_status.store(status);
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

            for (auto* channelStrip : engine->m_channelStrips)
                channelStrip->processNoteRawMidi(sampleOffset, midiBuffer);
        }
    }

    for (unsigned int i = 0; i < frameCount; ++i)
    {
        engine->m_outputBuffer[0][i] = 0.0f;
        engine->m_outputBuffer[1][i] = 0.0f;
    }

    for (auto* channelStrip : engine->m_channelStrips)
    {
        channelStrip->process();

        for (unsigned int i = 0; i < frameCount; ++i)
        {
            engine->m_outputBuffer[0][i] += channelStrip->m_outputBuffer[0][i];
            engine->m_outputBuffer[1][i] += channelStrip->m_outputBuffer[1][i];
        }
    }

    static int warnVolumeError = 1;
    auto outputVolume = engine->m_outputVolume.load();

    for (unsigned int i = 0; i < frameCount; ++i)
    {
        --warnVolumeError;

        if (engine->m_outputBuffer[0][i] > 1.0f || engine->m_outputBuffer[1][i] > 1.0f)
            //  || engine->m_outputBufferD[0][i] > 1.0f || engine->m_outputBufferD[1][i] > 1.0f)
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

        // TODO
        // out[2 * i]     = engine->m_outputBuffer[0][i] * outputVolume;
        // out[2 * i + 1] = engine->m_outputBuffer[1][i] * outputVolume;
    }

    return 0;
}
