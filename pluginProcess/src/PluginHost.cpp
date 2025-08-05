#include "PluginHost.h"
#include <thread>
#include <clap/helpers/host.hxx>
#include <clap/helpers/plugin-proxy.hxx>
#include <clap/helpers/reducing-param-queue.hxx>
#include <choc/gui/choc_MessageLoop.h>
#include <choc/memory/choc_Base64.h>
#include "Utils.h"
// #include "State.h"
#include "native/NativePluginGUI.h"


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


PluginHost::PluginHost(const int id) :
    BaseHost("Clap Workbench", "witte", "io.github.witte", "0.0.1"),
    m_id{id}
{
    threadType = ThreadType::MainThread;
    m_notesInt.reset(32);
    m_parameterChanges.reset(256);
}

PluginHost::~PluginHost()
{
    destroyNativeGuiWindow();
    deactivate();

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
}

bool PluginHost::hasNativeGUI() const
{
    return m_plugin && m_plugin->canUseGui();
}

void PluginHost::setPorts(const int numInputs, float** inputs, const int numOutputs, float** outputs)
{
    m_audioIn.channel_count = numInputs;
    m_audioIn.data32 = inputs;
    m_audioIn.data64 = nullptr;
    m_audioIn.constant_mask = 0;
    m_audioIn.latency = 0;

    m_audioOut.channel_count = numOutputs;
    m_audioOut.data32 = outputs;
    m_audioOut.data64 = nullptr;
    m_audioOut.constant_mask = 0;
    m_audioOut.latency = 0;
}

void PluginHost::activate(const int32_t sample_rate, const int32_t blockSize)
{
    std::println("Activating '{}'", m_name);
    if (!m_plugin || m_status >= ocp::Status::Activating)
        return;

    if (!m_plugin->activate(sample_rate, blockSize, blockSize))
    {
        std::println("Could not activate plugin: {}", m_name);
        m_status = ocp::Status::OnError;

        return;
    }

    m_blockSize = blockSize;
    m_status = ocp::Status::Activating;
}

void PluginHost::deactivate()
{
    std::println("Deactivating '{}'", m_name);
    if (!m_plugin || m_status < ocp::Status::Activating)
        return;

    m_blockSize = 0;
    m_plugin->deactivate();

    m_status = ocp::Status::Inactive;
}

void PluginHost::startProcessing()
{
    std::println("Start processing '{}'", m_name);
    if (!m_plugin || (m_status != ocp::Status::Activating && m_status != ocp::Status::Stopped))
        return;

    m_evOut.clear();
    m_evIn.clear();

    m_plugin->startProcessing();

    m_status = ocp::Status::Running;
}

void PluginHost::stopProcessing()
{
    std::println("Stop processing '{}'", m_name);
    if (!m_plugin)
        return;

    if (m_status >= ocp::Status::Stopped)
    {
        m_plugin->stopProcessing();

        m_evOut.clear();
        m_evIn.clear();
    }

    m_status = ocp::Status::Stopped;
}

void PluginHost::addParameterChangeToFifo(const clap_id id, const double value)
{
    m_parameterChanges.push({ id, value });
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

void PluginHost::addNoteToFifo(const int key, const int velocity, const bool isOn)
{
    const unsigned char _status = isOn? 0x90 : 0x80;
    const auto _note = static_cast<unsigned char>(key);
    const auto _velocity = static_cast<unsigned char>(velocity);

    const int midiMessage = (_status << 16) | (_note << 8) | _velocity;
    m_notesInt.push(midiMessage);
}

void PluginHost::processNoteOn(const int sampleOffset, const int channel, const int key, const int velocity)
{
    // TODO: this should be called from the audio thread!
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
    // TODO: this should be called from the audio thread!
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

void PluginHost::processNoteRawMidi(const int sampleOffset, const int midiMessage)
{
    // TODO: this should be called from the audio thread!
    clap_event_midi ev{};
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_MIDI;
    ev.header.time = sampleOffset;
    ev.header.flags = 0;
    ev.header.size = sizeof(ev);
    ev.port_index = 0;
    ev.data[0] = (midiMessage >> 16) & 0xFF;
    ev.data[1] = (midiMessage >> 8) & 0xFF;
    ev.data[2] = midiMessage & 0xFF;

    m_evIn.push(&ev.header);
}

void PluginHost::processNoteRawMidi(const int sampleOffset, const std::vector<unsigned char>& data)
{
    // TODO: this should be called from the audio thread!
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
    if (!m_plugin || m_blockSize == 0 || m_isByPassed)
        return;

    switch (m_status.load())
    {
        case ocp::Status::Inactive:
        case ocp::Status::OnError:
        case ocp::Status::Stopped:
        case ocp::Status::DeactivateRequested:
        case ocp::Status::Deactivating:
            return;
        case ocp::Status::Activating:
            startProcessing();
        case ocp::Status::Running:
        default:
            break;
    }

    m_process.transport = nullptr;

    int midiMessage = 0;
    while (m_notesInt.pop(midiMessage))
    {
        processNoteRawMidi(0, midiMessage);
    }

    std::pair<clap_id, double> parameterChange;
    while (m_parameterChanges.pop(parameterChange))
    {
        setParamValue(parameterChange.first, parameterChange.second);
    }

    m_process.in_events = m_evIn.clapInputEvents();
    m_process.out_events = m_evOut.clapOutputEvents();

    m_process.audio_inputs = &m_audioIn;
    m_process.audio_inputs_count = 1;
    m_process.audio_outputs = &m_audioOut;
    m_process.audio_outputs_count = 1;

    m_evOut.clear();

    m_process.frames_count = m_blockSize;

    const auto status = m_plugin->process(&m_process);
    (void)status;

    for (size_t i = 0; i < m_evOut.size(); ++i)
    {
        switch (const auto event = m_evOut.get(i); event->type)
        {
            case CLAP_EVENT_PARAM_GESTURE_BEGIN:
            {
                const auto ev = reinterpret_cast<const clap_event_param_gesture*>(event);
                break;
            }
            case CLAP_EVENT_PARAM_GESTURE_END:
            {
                const auto ev = reinterpret_cast<const clap_event_param_gesture*>(event);
                break;
            }
            case CLAP_EVENT_PARAM_VALUE:
            {
                // const auto ev = reinterpret_cast<const clap_event_param_value*>(event);
                // auto id = ev->param_id;
                // auto value = ev->value;
                //
                // choc::messageloop::postMessage([this, id, value]
                // {
                //     // m_state.plugins.parametersSetValue(m_id, id, value);
                // });

                break;
            }
            default:
                break;
        }
    }

    m_evOut.clear();
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

void PluginHost::showNativeWindow()
{
    if (!m_plugin->canUseGui())
        return;

    m_nativePluginGUI = std::make_unique<NativePluginGUI>(m_id);
    const auto w = m_nativePluginGUI->getClapWindow();

    const auto r1 = m_plugin->guiCreate(w.api, false);

    uint32_t width = 0;
    uint32_t height = 0;
    m_plugin->guiGetSize(&width, &height);

    const auto c = m_plugin->guiSetParent(&w);

    m_nativePluginGUI->setSize(static_cast<int>(width), static_cast<int>(height));
}

void PluginHost::destroyNativeGuiWindow()
{
    if (!m_plugin || !m_plugin->canUseGui() || !m_nativePluginGUI)
        return;

    m_nativePluginGUI.reset();
    m_plugin->guiDestroy();
}

void PluginHost::loadPluginState(const std::string_view stateAsBase64)
{
    // const QByteArray stateData = QByteArray::fromBase64(stateAsBase64.toUtf8());
    //
    // if (!m_plugin->canUseState())
    // {
    //     const QJsonDocument pluginStateJson = QJsonDocument::fromJson(stateData);
    //     QJsonObject jsonObject = pluginStateJson.object();
    //
    //     clap::helpers::EventList in;
    //     const clap::helpers::EventList out;
    //
    //     for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it)
    //     {
    //         const auto key = it.key().toUInt();
    //         const auto value = it.value().toDouble();
    //
    //         auto event = buildParameterValueChangeEvent(key, value);
    //         in.push(&event.header);
    //     }
    //
    //     m_plugin->paramsFlush(in.clapInputEvents(), out.clapOutputEvents());
    //
    //     for (size_t i = 0; i < out.size(); ++i)
    //     {
    //         const auto event = out.get(i);
    //         if (event->type != CLAP_EVENT_PARAM_VALUE)
    //             continue;
    //
    //         const auto ev = reinterpret_cast<const clap_event_param_value*>(event);
    //         m_parameterModel->setValueFromEngine(ev->param_id, ev->value);
    //     }
    //
    //     return;
    // }

    struct BufferReader
    {
        std::vector<uint8_t> data;
        size_t position = 0;
    };

    BufferReader stateBufferReader;
    choc::base64::decodeToContainer(stateBufferReader.data, stateAsBase64);

    const clap_istream pluginStateStream{ &stateBufferReader, [](const clap_istream* stream, void* buffer, const uint64_t size) -> int64_t
    {
        auto& reader = *static_cast<BufferReader*>(stream->ctx);

        if (reader.position >= reader.data.size())
            return 0;

        const uint64_t remaining = reader.data.size() - reader.position;
        const uint64_t toRead = std::min(size, remaining);

        std::memcpy(buffer, reader.data.data() + reader.position, toRead);
        reader.position += toRead;

        return static_cast<int64_t>(toRead);
    }};

    if (!m_plugin->stateLoad(&pluginStateStream))
        std::cerr << "Failed to load plugin state for:" << m_name << std::endl;
}

std::string_view PluginHost::getState() const
{
    if (!m_plugin->canUseState())
    {
        // QJsonObject pluginStateJson;
        //
        // for (const auto&[id, value] : m_parameterModel->getValues())
        //     pluginStateJson[QString::number(id)] = value;
        //
        // const QJsonDocument jsonDoc(pluginStateJson);
        //
        // const auto ret = jsonDoc.toJson(QJsonDocument::Compact);
        // return ret.toBase64();
    }

    // QFile file;
    // QBuffer buffer(&file);
    // buffer.open(QIODevice::WriteOnly);
    //
    // const clap_ostream pluginStateStream{ &buffer, [](const clap_ostream* stream, const void* buffer, const uint64_t size) -> int64_t
    // {
    //     auto& stateBuffer = *static_cast<QBuffer*>(stream->ctx);
    //     auto* stateData = static_cast<const char*>(buffer);
    //
    //     return stateBuffer.write(stateData, static_cast<long long>(size));
    // }};
    //
    // if (!m_plugin->stateSave(&pluginStateStream))
    //     qWarning() << "Failed to copy plugin state to buffer!";
    //
    // return buffer.data().toBase64();
    return "";
}

void PluginHost::beginParameterGesture(const clap_id id) const
{
    // m_parameterModel->startGesture(id);
}

void PluginHost::endParameterGesture(const clap_id id) const
{
    // m_parameterModel->stopGesture(id);
}

bool PluginHost::guiRequestResize(const uint32_t width, const uint32_t height) noexcept
{
    m_nativePluginGUI->setSize(static_cast<int>(width), static_cast<int>(height));
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
