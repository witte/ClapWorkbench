#include "PluginProcessHost.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

bool PluginProcessHost::init_host_ipc()
{
    if (!create(true))
        return false;

    // Create or open named semaphores
    sem_unlink(SEM_HOST_TO_PLUGIN);
    sem_unlink(SEM_PLUGIN_TO_HOST);
    sem_host_to_plugin = sem_open(SEM_HOST_TO_PLUGIN, O_CREAT, 0666, 0);
    sem_plugin_to_host = sem_open(SEM_PLUGIN_TO_HOST, O_CREAT, 0666, 0);

    return sem_host_to_plugin && sem_plugin_to_host;
}

void PluginProcessHost::destroy_host_ipc()
{
    destroy();
    if (sem_host_to_plugin)
        sem_close(sem_host_to_plugin);
    if (sem_plugin_to_host)
        sem_close(sem_plugin_to_host);

    sem_unlink(SEM_HOST_TO_PLUGIN);
    sem_unlink(SEM_PLUGIN_TO_HOST);
}

bool PluginProcessHost::create(const bool create_new)
{
    shm_unlink(SHM_NAME);

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

void PluginProcessHost::destroy()
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

void PluginProcessHost::activate(const int32_t sample_rate, const int32_t blockSize)
{
    init_host_ipc();
    PluginHost::activate(sample_rate, blockSize);
    qDebug() << "blockSize --> " << blockSize;
}

void PluginProcessHost::deactivate()
{
    PluginHost::deactivate();
    destroy_host_ipc();
}

// bool wait_plugin_done(sem_t* sem_plugin_to_host, int timeout_ms)
// {
//     struct timespec ts;
//     clock_gettime(CLOCK_REALTIME, &ts);
//
//     // Add timeout_ms to current time
//     ts.tv_nsec += (timeout_ms % 1000) * 1000000;
//     ts.tv_sec  += (timeout_ms / 1000) + (ts.tv_nsec / 1000000000);
//     ts.tv_nsec %= 1000000000;
//
//     if (sem_timedwait(sem_plugin_to_host, &ts) == -1) {
//         if (errno == ETIMEDOUT) {
//             return false; // timeout occurred
//         } else {
//             // handle other errors if needed
//             return false;
//         }
//     }
//     return true; // plugin signaled done
// }


void PluginProcessHost::process()
{
    if (!buf)
        return;

    const auto status = m_status.load();
    if (status == ocp::Status::Starting)
        startProcessing();

    else if (status != ocp::Status::Running || m_isByPassed)
        return;

    // // Fill dummy input buffer
    for (int i = 0; i < m_blockSize; ++i)
    {
        buf->input[0][i] = m_audioIn.data32[0][i];
        buf->input[1][i] = m_audioIn.data32[1][i];
    }

    sem_post(sem_host_to_plugin);

    // if (sem_trywait(sem_plugin_to_host))
    // {
    //     for (int i = 0; i < m_blockSize; ++i)
    //     {
    //         m_audioOut.data32[0][i] = buf->output[0][i];
    //         m_audioOut.data32[1][i] = buf->output[1][i];
    //     }
    // }

    sem_wait(sem_plugin_to_host);
    for (int i = 0; i < m_blockSize; ++i)
    {
        m_audioOut.data32[0][i] = buf->output[0][i];
        m_audioOut.data32[1][i] = buf->output[1][i];
    }

    // float* audio_out = buf->output;
}