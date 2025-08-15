#include "ChannelStrip.h"
#include "PluginManager.h"
#include "QJsonArray"


ChannelStrip::ChannelStrip(Node* parent) : Node(parent)
{
    m_outputBuffer[0] = static_cast<float*>(std::calloc(1, m_bufferSize * 4));
    m_outputBuffer[1] = static_cast<float*>(std::calloc(1, m_bufferSize * 4));
}

ChannelStrip::~ChannelStrip()
{
    free(m_outputBuffer[0]);
    free(m_outputBuffer[1]);

    m_outputBuffer[0] = nullptr;
    m_outputBuffer[1] = nullptr;
}

void ChannelStrip::setPorts(const int numInputs, float**, const int numOutputs, float**)
{
    for (auto* node : m_nodes)
    {
        node->setPorts(numInputs, m_outputBuffer, numOutputs, m_outputBuffer);
    }
}

void ChannelStrip::activate(const std::int32_t sampleRate, const std::int32_t blockSize)
{
    m_bufferSize = blockSize;

    for (unsigned int i = 0; i < m_bufferSize; ++i)
    {
        m_outputBuffer[0][i] = 0.0f;
        m_outputBuffer[1][i] = 0.0f;
    }

    for (auto* node : m_nodes)
    {
        node->activate(sampleRate, blockSize);
    }
}

void ChannelStrip::deactivate()
{
    for (auto* node: m_nodes)
        node->deactivate();
}

void ChannelStrip::startProcessing()
{
    for (auto* plugin : m_nodes)
    {
        if (plugin->status.load().status >= S::Stopped)
            plugin->startProcessing();
    }

    auto curStatus = status.load();
    curStatus.status = S::Running;
    status.store(curStatus);
}

void ChannelStrip::stopProcessing()
{
    for (auto* plugin : m_nodes)
        plugin->stopProcessing();

    auto curStatus = status.load();
    curStatus.status = S::Stopped;
    status.store(curStatus);
}

void ChannelStrip::processNoteRawMidi(const int sampleOffset, const std::vector<unsigned char>& data)
{
    for (auto* plugin : m_nodes)
        plugin->processNoteRawMidi(sampleOffset, data);
}

void ChannelStrip::process()
{
    std::memset(m_outputBuffer[0], 0, m_bufferSize * sizeof(float));
    std::memset(m_outputBuffer[1], 0, m_bufferSize * sizeof(float));

    auto curStatus = status.load();
    if (pendingTopologyChanges > 0)
    {
        return;
    }

    if (curStatus.status == S::Starting)
    {
        startProcessing();
    }
    else if (curStatus.status != S::Running)
    {
        return;
    }

    const clap::helpers::EventList* lastPluginEventsOut = nullptr;
    for (auto* plugin : m_nodes)
    {
        if (lastPluginEventsOut)
        {
            for (uint32_t i = 0; i < lastPluginEventsOut->size(); ++i)
            {
                const clap_event_header_t* ev = lastPluginEventsOut->get(i);
                if (!ev)
                    continue;

                plugin->m_evIn.tryPush(ev);
            }
        }

        plugin->process();

        if (plugin->m_process.out_events)
            lastPluginEventsOut = static_cast<clap::helpers::EventList*>(plugin->m_process.out_events->ctx);
        else
            lastPluginEventsOut = nullptr;
    }

    if (curStatus.isBypassed)
    {
        std::memset(m_outputBuffer[0], 0, m_bufferSize * sizeof(float));
        std::memset(m_outputBuffer[1], 0, m_bufferSize * sizeof(float));

        return;
    }

    const auto outputVolume = m_outputVolume.load();

    for (unsigned int i = 0; i < m_bufferSize; ++i)
    {
        m_outputBuffer[0][i] *= outputVolume;
        m_outputBuffer[1][i] *= outputVolume;
    }

    static int warnVolumeError = 1;

    for (unsigned int i = 0; i < m_bufferSize; ++i)
    {
        --warnVolumeError;

        if (m_outputBuffer[0][i] > 1.0f || m_outputBuffer[1][i] > 1.0f)
        {
            if (warnVolumeError <= 0)
            {
                qWarning() << "trash at " << i << ": " << m_outputBuffer[0][i] << ", " << m_outputBuffer[1][i];
                warnVolumeError = 24000;
            }

            std::memset(m_outputBuffer[0], 0, m_bufferSize * sizeof(float));
            std::memset(m_outputBuffer[1], 0, m_bufferSize * sizeof(float));

            curStatus.isBypassed = true;
            status.store(curStatus);
            emit isByPassedChanged();

            break;
        }
    }
}

QJsonObject ChannelStrip::getState() const
{
    QJsonObject state;
    state["type"] = "ChannelStrip";
    state["name"] = m_name;
    state["isBypassed"] = status.load().isBypassed;
    state["outputVolume"] = m_outputVolume.load();

    QJsonArray jsonArray;
    for (const auto* node : m_nodes)
    {
        jsonArray.append(node->getState());
    }

    state["nodes"] = jsonArray;
    return state;
}

void ChannelStrip::loadState(const QJsonObject& stateToLoad)
{
    if (stateToLoad.isEmpty())
        return;

    Status status_;
    status_.isBypassed = stateToLoad["isBypassed"].toBool();
    status.store(status_);
    setOutputVolume(stateToLoad["outputVolume"].toDouble());

    for (const auto plugins = stateToLoad["nodes"].toArray(); const auto& jsPluginRef : plugins)
    {
        const auto jsPlugin = jsPluginRef.toObject();
        auto* plugin = Node::create(this, jsPlugin);
        m_nodes.push_back(plugin);
    }

    emit nodesChanged();
}

double ChannelStrip::outputVolume() const
{
    return m_outputVolume.load();
}

void ChannelStrip::setOutputVolume(const double newOutputVolume)
{
    if (qFuzzyCompare(newOutputVolume, m_outputVolume))
        return;

    m_outputVolume = newOutputVolume;

    emit outputVolumeChanged();
}

void ChannelStrip::addNode(const QJsonObject& state)
{

}

void ChannelStrip::removeNode(const QJsonObject& state)
{

}

void ChannelStrip::load(PluginHost* plugin, const QString& path, const int pluginIndex)
{
    if (plugin)
        plugin->setIsByPassed(true);

    PluginHost* pluginToReload = plugin;

    if (pluginToReload == nullptr)
    {
        pluginToReload = new PluginHost{this};
        m_nodes.push_back(pluginToReload);
    }

    auto* m_pluginManager = PluginManager::instance();

    m_pluginManager->load(*pluginToReload, path, pluginIndex);

    if (status.load().status > S::Stopped)
    {
        pluginToReload->setPorts(2, m_outputBuffer, 2, m_outputBuffer);
        pluginToReload->activate(48000, static_cast<int>(m_bufferSize));

        auto tmpStatus = status.load();
        tmpStatus.status = S::Starting;
        pluginToReload->status.store(tmpStatus);
    }

    pluginToReload->setIsByPassed(false);

    emit nodesChanged();
    emit pluginToReload->nameChanged();
    emit pluginToReload->hasNativeGUIChanged();
}

void ChannelStrip::reorder(const int from, const int to)
{
    const auto oldStatus = status.load();
    auto tmpStatus = oldStatus;
    tmpStatus.status = S::Inactive;

    status.store(tmpStatus);

    const auto element = m_nodes.takeAt(from);
    m_nodes.insert(to, element);

    status.store(oldStatus);

    emit nodesChanged();
}
