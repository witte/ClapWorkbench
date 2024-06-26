#include "PluginHost.h"
#include <exception>
#include <clap/helpers/host.hxx>
#include <clap/helpers/plugin-proxy.hxx>
#include <clap/helpers/reducing-param-queue.hxx>
#include <QFile>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include "App.h"
#include "Utils.h"
#include "Components/PluginQuickView.h"

#include <thread>


enum class ThreadType
{
    Unknown,
    MainThread,
    AudioThread,
};
thread_local auto threadType = ThreadType::Unknown;


inline clap_event_param_value buildParameterValueChangeEvent(const clap_id key, const double value)
{
    clap_event_param_value event{};

    event.header.time = 0;
    event.header.type = CLAP_EVENT_PARAM_VALUE;
    event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    event.header.flags = 0;
    event.header.size = sizeof(event);
    event.param_id = key;
    event.cookie = nullptr;
    event.port_index = 0;
    event.key = -1;
    event.channel = -1;
    event.note_id = -1;
    event.value = value;

    return event;
}

template class clap::helpers::Host<PluginHost_MH, PluginHost_CL>;
template class clap::helpers::PluginProxy<PluginHost_MH, PluginHost_CL>;


PluginHost::PluginHost(QObject* parent)
    : Node(parent), BaseHost("Clap Workbench", "witte", "io.github.witte", "0.0.1")
{
    threadType = ThreadType::MainThread;

    connect(this, &PluginHost::parameterGestureBegan, this, &PluginHost::beginParameterGesture);
    connect(this, &PluginHost::parameterGestureEnded, this, &PluginHost::endParameterGesture);
}

PluginHost::~PluginHost()
{
    setIsFloatingWindowOpen(false);
    PluginHost::deactivate();
    destroyGuiWindow();

    if (m_plugin)
        m_plugin->destroy();
}

uint32_t PluginHost::parameterCount() const
{
    if (!m_plugin)
        return 0;

    return m_plugin->paramsCount();
}

void PluginHost::setIsByPassed(const bool newValue)
{
    if (newValue == m_isByPassed)
        return;

    m_isByPassed = newValue;
    emit isByPassedChanged();
}

QSize PluginHost::guiSize() const
{
    if (!m_plugin || !m_plugin->canUseGui() || !m_isNativeGuiOpen)
        return {};

    uint32_t width = 0;
    uint32_t height = 0;

    if (!m_plugin->guiGetSize(&width, &height))
    {
        qWarning() << "could not get the size of the plugin gui";
        return {};
    }

    return {static_cast<int>(width), static_cast<int>(height)};
}

void PluginHost::setFloatingWindow(PluginQuickView* window)
{
    if (window == m_floatingWindow)
        return;

    m_floatingWindow = window;
    emit floatingWindowChanged();
    emit isFloatingWindowOpenChanged();
}

bool PluginHost::isFloatingWindowOpen() const
{
    return m_floatingWindow != nullptr;
}

void PluginHost::setIsFloatingWindowOpen(const bool value)
{
    if (value)
    {
        if (isFloatingWindowOpen())
        {
            m_floatingWindow->raise();
            m_floatingWindow->requestActivate();

            return;
        }

        auto& appQmlEngine = App::instance()->getQmlEngine();

        m_floatingWindow = new PluginQuickView{appQmlEngine, *this};

        connect(m_floatingWindow, &QQuickView::closing, this, [this](QQuickCloseEvent*)
        {
            m_floatingWindow->deleteLater();
            setFloatingWindow(nullptr);
        });
    }
    else
    {
        if (m_floatingWindow)
            m_floatingWindow->close();
    }

    emit isFloatingWindowOpenChanged();
}

void PluginHost::setIsFloatingWindowVisible(const bool value)
{
    if (!isFloatingWindowOpen())
        return;

    m_floatingWindow->setVisible(value);
}

bool PluginHost::hasNativeGUI() const
{
    return m_plugin && m_plugin->canUseGui();
}

void PluginHost::setPorts(const int numInputs, float** inputs, const int numOutputs, float** outputs)
{
    m_audioIn.channel_count = numInputs;
    m_audioIn.data32 = inputs;
    m_audioIn.constant_mask = 0;
    m_audioIn.latency = 0;

    m_audioOut.channel_count = numOutputs;
    m_audioOut.data32 = outputs;
    m_audioOut.constant_mask = 0;
    m_audioOut.latency = 0;
}

void PluginHost::activate(const int32_t sample_rate, const int32_t blockSize)
{
    if (!m_plugin || status >= ocp::Status::OnHold)
        return;

    if (!m_plugin->activate(sample_rate, blockSize, blockSize))
    {
        qWarning() << "Could not activate plugin:" << m_name;
        status = ocp::Status::OnError;

        return;
    }

    m_sampleRate = sample_rate;
    m_sampleStep = 1.0 / m_sampleRate;
    m_blockSize = blockSize;
    status = ocp::Status::Starting;
}

void PluginHost::deactivate()
{
    if (!m_plugin || status < ocp::Status::OnHold)
        return;

    status = ocp::Status::Inactive;

    m_blockSize = 0;
    m_plugin->deactivate();
}

void PluginHost::startProcessing()
{
    if (!m_plugin)
        return;

    m_evOut.clear();
    m_evIn.clear();

    m_plugin->startProcessing();

    status = ocp::Status::Running;
}

void PluginHost::stopProcessing()
{
    if (!m_plugin)
        return;

    status = ocp::Status::OnHold;

    m_plugin->stopProcessing();
    m_isProcessing = false;

    m_evOut.clear();
    m_evIn.clear();
}

void PluginHost::setParamValue(const clap_id id, const double newValue)
{
    clap_event_param_value event{};
    event.header.time = 0;
    event.header.type = CLAP_EVENT_PARAM_VALUE;
    event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    event.header.flags = 0;
    event.header.size = sizeof(event);
    event.param_id = id;
    event.cookie = nullptr;
    event.port_index = 0;
    event.key = -1;
    event.channel = -1;
    event.note_id = -1;
    event.value = newValue;

    m_evIn.push(&event.header);
}

void PluginHost::processNoteOn(const int sampleOffset, const int channel, const int key, const int velocity)
{
    clap_event_note ev{};
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_NOTE_ON;
    ev.header.time = sampleOffset;
    ev.header.flags = 0;
    ev.header.size = sizeof(ev);
    ev.port_index = 0;
    ev.key = static_cast<short>(key);
    ev.channel = static_cast<short>(channel);
    ev.note_id = -1;
    ev.velocity = velocity / 127.0;

    m_evIn.push(&ev.header);
}

void PluginHost::processNoteOff(const int sampleOffset, const int channel, const int key, const int velocity)
{
    clap_event_note ev{};
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_NOTE_OFF;
    ev.header.time = sampleOffset;
    ev.header.flags = 0;
    ev.header.size = sizeof(ev);
    ev.port_index = 0;
    ev.key = static_cast<short>(key);
    ev.channel = static_cast<short>(channel);
    ev.note_id = -1;
    ev.velocity = velocity / 127.0;

    m_evIn.push(&ev.header);
}

void PluginHost::processNoteRawMidi(const int sampleOffset, const std::vector<unsigned char>& data)
{
    clap_event_midi ev{};
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_MIDI;
    ev.header.time = sampleOffset;
    ev.header.flags = 0;
    ev.header.size = sizeof(ev);
    ev.port_index = 0;
    ev.data[0] = data[0];
    ev.data[1] = data[1];
    ev.data[2] = data[2];

    m_evOut.push(&ev.header);
}

void PluginHost::outputRawMidi(const int sampleOffset, const std::vector<unsigned char>& data)
{
    clap_event_midi ev{};
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_MIDI;
    ev.header.time = sampleOffset;
    ev.header.flags = 0;
    ev.header.size = sizeof(ev);
    ev.port_index = 0;
    ev.data[0] = data[0];
    ev.data[1] = data[1];
    ev.data[2] = data[2];

    m_evIn.push(&ev.header);
}

void PluginHost::process()
{
    threadType = ThreadType::AudioThread;
    if (!m_plugin || m_blockSize == 0)
        return;

    const auto status_ = status.load();
    if (status_ == ocp::Status::Starting)
        startProcessing();

    else if (status_ != ocp::Status::Running || m_isByPassed)
    {
        m_evIn.clear();
        return;
    }

    clap_event_transport transport_event = {};
    transport_event.header.size = sizeof(clap_event_transport);
    transport_event.header.time = 0;
    transport_event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    transport_event.header.type = CLAP_EVENT_TRANSPORT;
    transport_event.header.flags = 0;

    transport_event.flags = CLAP_TRANSPORT_HAS_TEMPO |
                            CLAP_TRANSPORT_HAS_BEATS_TIMELINE |
                            CLAP_TRANSPORT_IS_PLAYING;

    samplesPerBeat = m_sampleRate * (60.0 / AudioEngine::instance()->bpm());

    const double somethingElse = m_blockSize / samplesPerBeat;
    song_pos_beats += somethingElse;
    m_process.frames_count = m_blockSize;
    current_sample += m_blockSize;

    transport_event.song_pos_beats = static_cast<clap_beattime>(std::round(CLAP_BEATTIME_FACTOR * song_pos_beats));
    transport_event.song_pos_seconds = current_sample / m_sampleRate;

    transport_event.tempo = AudioEngine::instance()->bpm();
    transport_event.tempo_inc = 0.0;

    m_process.transport = &transport_event;

    m_process.in_events = m_evIn.clapInputEvents();
    m_process.out_events = m_evOut.clapOutputEvents();

    m_process.audio_inputs = &m_audioIn;
    m_process.audio_inputs_count = 1;
    m_process.audio_outputs = &m_audioOut;
    m_process.audio_outputs_count = 1;

    m_evOut.clear();

    const auto clapStatus = m_plugin->process(&m_process);
    (void)clapStatus;

    for (size_t i = 0; i < m_evOut.size(); ++i)
    {
        switch (const auto event = m_evOut.get(i); event->type)
        {
            case CLAP_EVENT_PARAM_GESTURE_BEGIN:
            {
                const auto ev = reinterpret_cast<const clap_event_param_gesture*>(event);
                emit parameterGestureBegan(ev->param_id);
                break;
            }
            case CLAP_EVENT_PARAM_GESTURE_END:
            {
                const auto ev = reinterpret_cast<const clap_event_param_gesture*>(event);
                emit parameterGestureEnded(ev->param_id);
                break;
            }
            case CLAP_EVENT_PARAM_VALUE:
            {
                const auto ev = reinterpret_cast<const clap_event_param_value*>(event);
                m_parameterModel->setValueFromEngine(ev->param_id, ev->value);
                break;
            }
            case CLAP_EVENT_NOTE_ON:
            {
                const auto ev = reinterpret_cast<const clap_event_note_t*>(event);
                qDebug() << "NoteOn" << ev->key << " " << ev->velocity << " " << ev->channel << " " << ev->port_index << " ";

                std::vector<unsigned char> midiMessage;

                unsigned char _status = 0x90;    // Note On, channel 1
                unsigned char note = 60;         // Middle C (MIDI note number)
                unsigned char velocity = 100;    // Velocity (0-127)

                midiMessage.push_back(_status);
                midiMessage.push_back(note);
                midiMessage.push_back(velocity);

                outputRawMidi(0, midiMessage);
            }
            default:
                break;
        }
    }

    // m_evOut.clear();
    m_evIn.clear();
}

bool PluginHost::threadCheckIsMainThread() const noexcept
{
    return threadType == ThreadType::MainThread;
}

bool PluginHost::threadCheckIsAudioThread() const noexcept
{
    return threadType != ThreadType::MainThread;
}

bool PluginHost::hasWindow(const QQuickWindow* window) const
{
    return m_parentWindow == window;
}

void PluginHost::createGuiWindow(QQuickWindow* parentWindow)
{
    m_parentWindow = parentWindow;

    if (!m_plugin || !m_plugin->canUseGui())
        return;

    const auto w = ocp::makeClapWindow(m_parentWindow->winId());
    if (!m_isNativeGuiOpen)
    {
        if (!m_plugin->guiCreate(w.api, false))
        {
            qWarning() << "could not create the plugin gui";
            return;
        }
    }

    m_isNativeGuiOpen = true;

    uint32_t width = 0;
    uint32_t height = 0;

    if (!m_plugin->guiGetSize(&width, &height))
    {
        qWarning() << "could not get the size of the plugin gui";
        m_isNativeGuiOpen = false;
        m_plugin->guiDestroy();

        return;
    }

    m_parentWindow->resize(static_cast<int>(width), static_cast<int>(height));

    // Without this the gui flickers before being rendered, I _guess_
    // because of the resizing of the window.
    QTimer::singleShot(100, this, [this]()
    {
        const auto _w = ocp::makeClapWindow(m_parentWindow->winId());
        if (!m_plugin->guiSetParent(&_w))
        {
            qWarning() << "could not embbed the plugin gui";
            m_isNativeGuiOpen = false;
            m_plugin->guiDestroy();

            return;
        }

        emit guiSizeChanged();
    });
}

void PluginHost::setParentWindow(QQuickWindow* parentWindow)
{
    destroyGuiWindow();
    createGuiWindow(parentWindow);
}

void PluginHost::destroyGuiWindow()
{
    if (!m_plugin || !m_plugin->canUseGui() || !m_isNativeGuiOpen)
        return;

    m_parentWindow = nullptr;
    m_plugin->guiDestroy();
    m_isNativeGuiOpen = false;
}

void PluginHost::loadPluginState(const QString& stateAsBase64)
{
    const QByteArray stateData = QByteArray::fromBase64(stateAsBase64.toUtf8());

    if (!m_plugin->canUseState())
    {
        if (!m_plugin->canUseParams())
            return;

        const QJsonDocument pluginStateJson = QJsonDocument::fromJson(stateData);
        QJsonObject jsonObject = pluginStateJson.object();

        clap::helpers::EventList in;
        const clap::helpers::EventList out;

        for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it)
        {
            const auto key = it.key().toUInt();
            const auto value = it.value().toDouble();

            auto event = buildParameterValueChangeEvent(key, value);
            in.push(&event.header);
        }

        m_plugin->paramsFlush(in.clapInputEvents(), out.clapOutputEvents());

        for (size_t i = 0; i < out.size(); ++i)
        {
            const auto event = out.get(i);
            if (event->type != CLAP_EVENT_PARAM_VALUE)
                continue;

            const auto ev = reinterpret_cast<const clap_event_param_value*>(event);
            m_parameterModel->setValueFromEngine(ev->param_id, ev->value);
        }

        return;
    }

    QFile tempFile;
    QBuffer stateBuffer(&tempFile);
    stateBuffer.setData(stateData);
    stateBuffer.open(QIODevice::ReadOnly);

    const clap_istream pluginStateStream{ &stateBuffer, [](const clap_istream* stream, void* buffer, const uint64_t size) -> int64_t
    {
        return static_cast<QBuffer*>(stream->ctx)->read(static_cast<char*>(buffer), static_cast<long long>(size));
    }};

    if (!m_plugin->stateLoad(&pluginStateStream))
        qWarning() << "Failed to load plugin state for:" << m_name;
}

QJsonObject PluginHost::getState() const
{
    QJsonObject pluginStateJson;
    pluginStateJson["path"] = path();
    pluginStateJson["index"] = static_cast<int>(index());
    pluginStateJson["name"] = name();

    if (!m_plugin->canUseState())
    {
        for (const auto&[id, value] : m_parameterModel->getValues())
            pluginStateJson[QString::number(id)] = value;

        const QJsonDocument jsonDoc(pluginStateJson);

        const auto ret = jsonDoc.toJson(QJsonDocument::Compact);
        pluginStateJson["stateData"]= QString(ret.toBase64());

        return pluginStateJson;
    }

    QFile file;
    QBuffer fileBuffer(&file);
    fileBuffer.open(QIODevice::WriteOnly);

    const clap_ostream pluginStateStream{ &fileBuffer, [](const clap_ostream* stream, const void* buffer, const uint64_t size) -> int64_t
    {
        auto& stateBuffer = *static_cast<QBuffer*>(stream->ctx);
        auto* stateData = static_cast<const char*>(buffer);

        return stateBuffer.write(stateData, static_cast<long long>(size));
    }};

    if (!m_plugin->stateSave(&pluginStateStream))
        qWarning() << "Failed to copy plugin state to buffer!";

    pluginStateJson["stateData"] = QString(fileBuffer.data().toBase64());

    return pluginStateJson;
}

void PluginHost::beginParameterGesture(const clap_id id) const
{
    m_parameterModel->startGesture(id);
}

void PluginHost::endParameterGesture(const clap_id id) const
{
    m_parameterModel->stopGesture(id);
}

bool PluginHost::guiRequestResize(uint32_t, uint32_t) noexcept
{
    emit guiSizeChanged();

    return true;
}

// void PluginHost::guiResizeHintsChanged() noexcept {}
// bool PluginHost::guiRequestResize(uint32_t width, uint32_t height) noexcept {}
// bool PluginHost::guiRequestShow() noexcept {}
// bool PluginHost::guiRequestHide() noexcept {}
// void PluginHost::guiClosed(bool wasDestroyed) noexcept {}
// void PluginHost::logLog(clap_log_severity severity, const char* message) const noexcept {}
// bool PluginHost::posixFdSupportRegisterFd(int fd, clap_posix_fd_flags_t flags) noexcept {}
// bool PluginHost::posixFdSupportModifyFd(int fd, clap_posix_fd_flags_t flags) noexcept {}
// bool PluginHost::posixFdSupportUnregisterFd(int fd) noexcept {}
// void PluginHost::remoteControlsChanged() noexcept {}
// void PluginHost::remoteControlsSuggestPage(clap_id pageId) noexcept {}
// void PluginHost::stateMarkDirty() noexcept {}
// bool PluginHost::timerSupportRegisterTimer(uint32_t periodMs, clap_id* timerId) noexcept {}
// bool PluginHost::timerSupportUnregisterTimer(clap_id timerId) noexcept {}
// bool PluginHost::threadPoolRequestExec(uint32_t numTasks) noexcept {}
