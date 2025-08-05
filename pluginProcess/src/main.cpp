#include "PluginHost.h"
#include <__ostream/print.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <choc/gui/choc_MessageLoop.h>
#include "PluginHost.h"
#include "PluginManager.h"


constexpr auto* SHM_NAME = "/clap_shm_audio";
constexpr auto* SEM_HOST_TO_PLUGIN = "/sem_clap_host_to_plugin";
constexpr auto* SEM_PLUGIN_TO_HOST = "/sem_clap_plugin_to_host";
constexpr int AUDIO_BUFFER_SIZE = 512;

struct SharedAudioBuffer
{
    float input[2][AUDIO_BUFFER_SIZE];
    float output[2][AUDIO_BUFFER_SIZE];
};

int shm_fd = -1;
sem_t* sem_host_to_plugin = nullptr;
sem_t* sem_plugin_to_host = nullptr;
SharedAudioBuffer* buf = nullptr;

bool create(const bool create_new)
{
    const int flags = create_new ? O_CREAT | O_RDWR : O_RDWR;
    shm_fd = shm_open(SHM_NAME, flags, 0666);
    if (shm_fd == -1)
        return false;

    if (create_new)
    {
        if (ftruncate(shm_fd, sizeof(SharedAudioBuffer)) == -1)
            return false;
    }

    void* ptr = mmap(nullptr, sizeof(SharedAudioBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
        return false;

    buf = static_cast<SharedAudioBuffer*>(ptr);
    if (create_new)
    {
        memset(buf, 0, sizeof(SharedAudioBuffer));
    }

    return true;
}

void destroy()
{
    if (buf)
    {
        munmap(buf, sizeof(SharedAudioBuffer));
        buf = nullptr;
    }

    if (shm_fd != -1)
    {
        close(shm_fd);
        shm_fd = -1;
    }

    shm_unlink(SHM_NAME);
}


bool init_plugin_ipc()
{
    if (!create(false))
        return false;

    sem_host_to_plugin = sem_open(SEM_HOST_TO_PLUGIN, 0);
    sem_plugin_to_host = sem_open(SEM_PLUGIN_TO_HOST, 0);

    return sem_host_to_plugin && sem_plugin_to_host;
}

float* m_outputBuffer[2] = {nullptr, nullptr};
std::atomic_bool running{true};
PluginHost plugin{0};

void signalHandler(int sig)
{
    std::println("SIGINT!");
    running = false;
    choc::messageloop::stop();
}


int main()
{
    if (!init_plugin_ipc())
    {
        std::println("no :(");
        return 1;
    }

    signal(SIGINT, signalHandler);

    PluginManager pluginManager;

    m_outputBuffer[0] = static_cast<float*>(std::calloc(1, AUDIO_BUFFER_SIZE * 4));
    m_outputBuffer[1] = static_cast<float*>(std::calloc(1, AUDIO_BUFFER_SIZE * 4));

    pluginManager.load(plugin,
        // "/Users/witte/Work/clap/SamplePlayer/cmake-build-release/SamplePlayer.clap"
        // "/Users/witte/Work/clap/ClapSamplePlayer/cmake-build-release/ClapSamplePlayer.clap"
        // "/Users/witte/Work/clap/ClapSamplePlayer/cmake-build-debug/ClapSamplePlayer.clap"
        // "/Library/Audio/Plug-Ins/CLAP/LatinPercussion.clap"
        "/Library/Audio/Plug-Ins/CLAP/Springs.clap"
        , 0);
    plugin.setPorts(2, m_outputBuffer, 2, m_outputBuffer);
    plugin.activate(48000, 128);

    choc::messageloop::initialise();
    plugin.showNativeWindow();
    // plugin.addParameterChangeToFifo(2345, 2.0);

    auto th = std::thread([]()
    {
        std::println("audio thread started");
        plugin.startProcessing();

        while (running.load())
        {
            sem_wait(sem_host_to_plugin);

            for (int i = 0; i < 128; ++i)
            {
                const auto& l = buf->input[0][i];
                const auto& r = buf->input[1][i];
                m_outputBuffer[0][i] = buf->input[0][i];
                m_outputBuffer[1][i] = buf->input[1][i];
            }

            plugin.process();

            for (int i = 0; i < 128; ++i)
            {
                const auto& l = m_outputBuffer[0][i];
                const auto& r = m_outputBuffer[1][i];
                buf->output[0][i] = m_outputBuffer[0][i];
                buf->output[1][i] = m_outputBuffer[1][i];
            }

            sem_post(sem_plugin_to_host);
        }

        plugin.stopProcessing();
        std::println("audio thread stopped");
    });

    choc::messageloop::run();

    running = false;
    th.join();

    std::println("bye!");

    return 0;
}
