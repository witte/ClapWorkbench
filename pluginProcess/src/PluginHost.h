#pragma once
#include <filesystem>
#include <clap/helpers/event-list.hh>
#include <clap/helpers/host.hh>
#include <clap/helpers/plugin-proxy.hh>
#include <clap/helpers/reducing-param-queue.hh>
#include <choc/containers/choc_SingleReaderSingleWriterFIFO.h>
#include "Utils.h"

class State;
class NativePluginGUI;

constexpr auto PluginHost_MH = clap::helpers::MisbehaviourHandler::Terminate;
constexpr auto PluginHost_CL = clap::helpers::CheckingLevel::Maximal;

using BaseHost = clap::helpers::Host<PluginHost_MH, PluginHost_CL>;
extern template class clap::helpers::Host<PluginHost_MH, PluginHost_CL>;

using PluginProxy = clap::helpers::PluginProxy<PluginHost_MH, PluginHost_CL>;
extern template class clap::helpers::PluginProxy<PluginHost_MH, PluginHost_CL>;

class PluginHost : public BaseHost
{
  public:
    explicit PluginHost(int id);
    ~PluginHost() override;
    PluginHost(PluginHost&) = delete;
    PluginHost(PluginHost&&) = delete;
    PluginHost(const PluginHost&) = delete;
    PluginHost(const PluginHost&&) = delete;


    [[nodiscard]] ocp::Status status() const noexcept { return m_status; }

    [[nodiscard]] std::string_view path() const noexcept { return m_pluginPath; }

    [[nodiscard]] uint32_t index() const noexcept { return m_index; }

    [[nodiscard]] std::string_view name() const noexcept { return m_name; }

    [[nodiscard]] uint32_t parameterCount() const;

    [[nodiscard]] bool isByPassed() const { return m_isByPassed; }
    void setIsByPassed(bool newValue);

    [[nodiscard]] bool hasNativeGUI() const;

    void setPorts(int numInputs, float** inputs, int numOutputs, float** outputs);
    void activate(int32_t sample_rate, int32_t blockSize);
    void deactivate();

    void startProcessing();
    void stopProcessing();


    void addParameterChangeToFifo(clap_id id, double value);
    void setParamValue(clap_id id, double newValue);

    void addNoteToFifo(int key, int velocity, bool isOn);

    void processNoteOn(int sampleOffset, int channel, int key, int velocity);
    void processNoteOff(int sampleOffset, int channel, int key, int velocity);
    void processNoteRawMidi(int sampleOffset, int midiMessage);
    void processNoteRawMidi(int sampleOffset, const std::vector<unsigned char>& data);
    void process();

    [[nodiscard]] bool threadCheckIsMainThread() const noexcept override;
    [[nodiscard]] bool threadCheckIsAudioThread() const noexcept override;

    void showNativeWindow();
    void destroyNativeGuiWindow();

    void loadPluginState(std::string_view stateAsBase64);
    [[nodiscard]] std::string_view getState() const;

    void beginParameterGesture(clap_id id) const;
    void endParameterGesture(clap_id id) const;

    std::unique_ptr<PluginProxy> m_plugin;

  private:
    friend class PluginManager;

    int m_id;

    uint32_t m_index = 0;
    std::string m_name;
    std::string m_pluginPath;
    std::filesystem::path m_pluginPathAsPath;
    std::atomic<ocp::Status> m_status = ocp::Status::Inactive;

    std::unique_ptr<NativePluginGUI> m_nativePluginGUI;

    std::atomic_bool m_isByPassed = false;

    choc::fifo::SingleReaderSingleWriterFIFO<std::pair<clap_id, double>> m_parameterChanges;
    choc::fifo::SingleReaderSingleWriterFIFO<int> m_notesInt;
    clap_audio_buffer m_audioIn = {};
    clap_audio_buffer m_audioOut = {};
    clap::helpers::EventList m_evIn;
    clap::helpers::EventList m_evOut;
    int32_t m_blockSize = 0;
    clap_process m_process{};


    void requestRestart() noexcept override {};
    void requestProcess() noexcept override {};
    void requestCallback() noexcept override {};

    bool implementsAudioPorts() const noexcept override { return true; }
    bool audioPortsIsRescanFlagSupported(uint32_t /*flag*/) noexcept override { return false; }
    void audioPortsRescan(uint32_t /*flags*/) noexcept override {}


    bool implementsParams() const noexcept override { return true; }
    void paramsRescan(clap_param_rescan_flags /*flags*/) noexcept override {};
    void paramsClear(clap_id /*paramId*/, clap_param_clear_flags /*flags*/) noexcept override {};
    void paramsRequestFlush() noexcept override {};

    // clap_host
    // void requestRestart() noexcept override;
    // void requestProcess() noexcept override;
    // void requestCallback() noexcept override;

    // // clap_host_gui
    bool implementsGui() const noexcept override { return true; }
    // void guiResizeHintsChanged() noexcept override;
    bool guiRequestResize(uint32_t width, uint32_t height) noexcept override;
    // bool guiRequestShow() noexcept override;
    // bool guiRequestHide() noexcept override;
    // void guiClosed(bool wasDestroyed) noexcept override;

    // // clap_host_log
    // bool implementsLog() const noexcept override { return true; }
    // void logLog(clap_log_severity severity, const char *message) const noexcept override;

    // // clap_host_params
    // // bool implementsParams() const noexcept override { return true; }
    // // void paramsRescan(clap_param_rescan_flags flags) noexcept override;
    // // void paramsClear(clap_id paramId, clap_param_clear_flags flags) noexcept override;
    // // void paramsRequestFlush() noexcept override;

    // // clap_host_posix_fd_support
    // bool implementsPosixFdSupport() const noexcept override { return true; }
    // bool posixFdSupportRegisterFd(int fd, clap_posix_fd_flags_t flags) noexcept override;
    // bool posixFdSupportModifyFd(int fd, clap_posix_fd_flags_t flags) noexcept override;
    // bool posixFdSupportUnregisterFd(int fd) noexcept override;

    // // clap_host_remote_controls
    // bool implementsRemoteControls() const noexcept override { return true; }
    // void remoteControlsChanged() noexcept override;
    // void remoteControlsSuggestPage(clap_id pageId) noexcept override;

    // // clap_host_state
    bool implementsState() const noexcept override { return true; }
    // void stateMarkDirty() noexcept override;

    // // clap_host_timer_support
    // bool implementsTimerSupport() const noexcept override { return true; }
    // bool timerSupportRegisterTimer(uint32_t periodMs, clap_id *timerId) noexcept override;
    // bool timerSupportUnregisterTimer(clap_id timerId) noexcept override;

    // // clap_host_thread_check
    // // bool threadCheckIsMainThread() const noexcept override;
    // // bool threadCheckIsAudioThread() const noexcept override;

    // // clap_host_thread_pool
    // bool implementsThreadPool() const noexcept override { return true; }
    // bool threadPoolRequestExec(uint32_t numTasks) noexcept override;
};
