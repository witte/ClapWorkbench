#pragma once
#include <filesystem>
#include <clap/helpers/event-list.hh>
#include <clap/helpers/host.hh>
#include <clap/helpers/plugin-proxy.hh>
#include <clap/helpers/reducing-param-queue.hh>
#include <QObject>
#include <QQuickWindow>
#include <QJsonObject>
#include "Node.h"
#include "Utils.h"
#include "ParameterModel.h"


class PluginQuickView;

namespace Ocp
{
    Q_NAMESPACE
    QML_ELEMENT

    enum class GuiType
    {
        Generic,
        Native,
    };
    Q_ENUM_NS(GuiType)
}


using BaseHost = clap::helpers::Host<PluginHost_MH, PluginHost_CL>;
extern template class clap::helpers::Host<PluginHost_MH, PluginHost_CL>;

using PluginProxy = clap::helpers::PluginProxy<PluginHost_MH, PluginHost_CL>;
extern template class clap::helpers::PluginProxy<PluginHost_MH, PluginHost_CL>;

class PluginHost : public Node, public BaseHost
{
    Q_OBJECT
    Q_PROPERTY(ParameterModel* parameters READ parameters CONSTANT)
    Q_PROPERTY(bool isByPassed READ isByPassed WRITE setIsByPassed NOTIFY isByPassedChanged)
    Q_PROPERTY(bool hasNativeGUI READ hasNativeGUI NOTIFY hasNativeGUIChanged)
    Q_PROPERTY(QSize guiSize READ guiSize NOTIFY guiSizeChanged)
    Q_PROPERTY(bool isFloatingWindowOpen READ isFloatingWindowOpen WRITE setIsFloatingWindowOpen NOTIFY isFloatingWindowOpenChanged)
    Q_PROPERTY(PluginQuickView* floatingWindow READ floatingWindow NOTIFY floatingWindowChanged)
    QML_ELEMENT


  public:
    explicit PluginHost(QObject* parent = nullptr);
    ~PluginHost() override;
    PluginHost(PluginHost&) = delete;
    PluginHost(PluginHost&&) = delete;
    PluginHost(const PluginHost&) = delete;
    PluginHost(const PluginHost&&) = delete;


    [[nodiscard]] QString path() const noexcept { return m_pluginPath; }

    [[nodiscard]] uint32_t index() const noexcept { return m_index; }

    [[nodiscard]] uint32_t parameterCount() const;

    [[nodiscard]] ParameterModel* parameters() const
    {
        return m_parameterModel.get();
    }

    [[nodiscard]] bool isByPassed() const;
    void setIsByPassed(bool newValue);

    [[nodiscard]] QSize guiSize() const;
    [[nodiscard]] PluginQuickView* floatingWindow() const { return m_floatingWindow; }
    void setFloatingWindow(PluginQuickView* window);

    [[nodiscard]] bool isFloatingWindowOpen() const;
    void setIsFloatingWindowOpen(bool value);

    void setIsFloatingWindowVisible(bool value);

    [[nodiscard]] bool hasNativeGUI() const;

    void setPorts(int numInputs, float** inputs, int numOutputs, float** outputs) override;
    void activate(int32_t sample_rate, int32_t blockSize) override;
    void deactivate() override;

    void startProcessing() override;
    void stopProcessing() override;


    void setParamValue(clap_id id, double newValue);

    void processNoteOn(int sampleOffset, int channel, int key, int velocity);
    void processNoteOff(int sampleOffset, int channel, int key, int velocity);
    void processNoteRawMidi(int sampleOffset, const std::vector<unsigned char>& data) override;
    void outputRawMidi(int sampleOffset, const std::vector<unsigned char>& data);
    void process() override;

    [[nodiscard]] bool threadCheckIsMainThread() const noexcept override;
    [[nodiscard]] bool threadCheckIsAudioThread() const noexcept override;


  public slots:
    bool hasWindow(const QQuickWindow* window) const;
    void createGuiWindow(QQuickWindow* parentWindow);
    void setParentWindow(QQuickWindow* parentWindow);
    void destroyGuiWindow();

    void loadPluginState(const QString& stateAsBase64);
    [[nodiscard]] QJsonObject getState() const override;


  signals:
    void hostedPluginChanged();
    void parameterGestureBegan(clap_id id);
    void parameterGestureEnded(clap_id id);
    void parameterValueChanged(clap_id id, double value);
    void isByPassedChanged();

    void hasNativeGUIChanged();
    void guiSizeChanged();
    void isFloatingWindowOpenChanged();
    void floatingWindowChanged();


  private slots:
    void beginParameterGesture(clap_id id) const;
    void endParameterGesture(clap_id id) const;


  private:
    friend class PluginManager;

    int32_t m_sampleRate = 48000;
    double m_sampleStep = 0.0;
    uint32_t m_index = 0;
    QString m_pluginPath;
    std::filesystem::path m_pluginPathAsPath;

    bool m_isProcessing = false;
    bool m_isNativeGuiOpen = false;
    PluginQuickView* m_floatingWindow = nullptr;

    std::unique_ptr<PluginProxy> m_plugin;

    QQuickWindow* m_parentWindow = nullptr;
    std::unique_ptr<ParameterModel> m_parameterModel;

    clap_audio_buffer m_audioIn = {};
    clap_audio_buffer m_audioOut = {};
    int32_t m_blockSize = 0;

    // double m_bpm = 120.0;
    double samplesPerBeat = 60.0 / 120.0;

    uint64_t current_sample = 0;
    // uint64_t bpmAdjustmentSample = 0;
    // uint64_t bpmAdjustmentBeat = 0;
    double song_pos_beats = 0.0;



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
