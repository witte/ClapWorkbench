#include "ChannelStrip.h"
#include "PluginManager.h"
#include "QJsonArray"


ChannelStrip::ChannelStrip(Node* parent) : Node(parent, Type::ChannelStrip)
{
    buffer[0] = static_cast<float*>(std::calloc(1, m_bufferSize * 4));
    buffer[1] = static_cast<float*>(std::calloc(1, m_bufferSize * 4));
}

ChannelStrip::~ChannelStrip()
{
    free(buffer[0]);
    free(buffer[1]);

    buffer[0] = nullptr;
    buffer[1] = nullptr;
}

void ChannelStrip::setPorts(const int numInputs, float**, const int numOutputs, float**)
{
    for (auto* node : m_nodes)
        node->setPorts(numInputs, buffer, numOutputs, buffer);

    for (auto* node : m_channels)
        node->setPorts(numInputs, buffer, numOutputs, buffer);
}

void ChannelStrip::activate(const std::int32_t sampleRate, const std::int32_t blockSize)
{
    m_bufferSize = blockSize;

    std::memset(buffer[0], 0, m_bufferSize * sizeof(float));
    std::memset(buffer[1], 0, m_bufferSize * sizeof(float));

    for (auto* node : m_nodes)
        node->activate(sampleRate, blockSize);

    for (auto* node : m_channels)
        node->activate(sampleRate, blockSize);

    auto curStatus = status.load();
    curStatus.status = S::Stopped;
    status.store(curStatus);
}

void ChannelStrip::deactivate()
{
    for (auto* node: m_nodes)
        node->deactivate();

    for (auto* node: m_channels)
        node->deactivate();
}

void ChannelStrip::startProcessing()
{
    for (auto* plugin : m_nodes)
    {
        if (plugin->status.load().status >= S::Stopped)
            plugin->startProcessing();
    }

    for (auto* plugin : m_channels)
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

    for (auto* plugin : m_channels)
        plugin->stopProcessing();

    auto curStatus = status.load();
    curStatus.status = S::Stopped;
    status.store(curStatus);
}

void ChannelStrip::processNoteRawMidi(const int sampleOffset, const std::vector<unsigned char>& data)
{
    for (auto* plugin : m_nodes)
        plugin->processNoteRawMidi(sampleOffset, data);

    for (auto* plugin : m_channels)
        plugin->processNoteRawMidi(sampleOffset, data);
}

void ChannelStrip::process()
{
    std::memset(buffer[0], 0, m_bufferSize * sizeof(float));
    std::memset(buffer[1], 0, m_bufferSize * sizeof(float));

    auto curStatus = status.load();
    if (pendingTopologyChanges > 0)
        return;

    if (curStatus.status == S::Starting)
    {
        startProcessing();
    }
    else if (curStatus.status != S::Running)
    {
        return;
    }

    for (auto* node : m_channels)
    {
        node->process();

        for (unsigned int i = 0; i < m_bufferSize; ++i)
        {
            buffer[0][i] += node->buffer[0][i];
            buffer[1][i] += node->buffer[1][i];
        }
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
        std::memset(buffer[0], 0, m_bufferSize * sizeof(float));
        std::memset(buffer[1], 0, m_bufferSize * sizeof(float));

        return;
    }

    const auto outputVolume = m_outputVolume.load();

    for (unsigned int i = 0; i < m_bufferSize; ++i)
    {
        buffer[0][i] *= outputVolume;
        buffer[1][i] *= outputVolume;
    }
}

QJsonObject ChannelStrip::getState() const
{
    QJsonObject state;
    state["type"] = "ChannelStrip";
    state["name"] = m_name;
    state["isBypassed"] = status.load().isBypassed;
    state["outputVolume"] = m_outputVolume.load();

    QJsonArray channelsArray;
    for (const auto* node : m_channels)
        channelsArray.append(node->getState());

    state["channels"] = channelsArray;

    QJsonArray jsonArray;
    for (const auto* node : m_nodes)
        jsonArray.append(node->getState());

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
    setName(stateToLoad["name"].toString());

    for (const auto plugins = stateToLoad["channels"].toArray(); const auto& jsPluginRef : plugins)
    {
        const auto jsPlugin = jsPluginRef.toObject();
        auto* plugin = Node::create(this, jsPlugin);
        m_channels.push_back(plugin);
    }

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
        pluginToReload->setPorts(2, buffer, 2, buffer);
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
