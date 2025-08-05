#pragma once
#include <semaphore.h>
#include "PluginHost.h"


constexpr auto* SHM_NAME = "/clap_shm_audio";
constexpr auto* SEM_HOST_TO_PLUGIN = "/sem_clap_host_to_plugin";
constexpr auto* SEM_PLUGIN_TO_HOST = "/sem_clap_plugin_to_host";
constexpr int AUDIO_BUFFER_SIZE = 512;

struct SharedAudioBuffer
{
    float input[2][AUDIO_BUFFER_SIZE];
    float output[2][AUDIO_BUFFER_SIZE];
};


class PluginProcessHost final : public PluginHost
{
public:
    SharedAudioBuffer* buf = nullptr;
    int shm_fd = -1;

    sem_t* sem_host_to_plugin = nullptr;
    sem_t* sem_plugin_to_host = nullptr;

    bool init_host_ipc();

    void destroy_host_ipc();

    bool create(bool create_new = true);

    void destroy();

    void activate(int32_t sample_rate, int32_t blockSize) override;
    void deactivate() override;
    void process() override;
};

