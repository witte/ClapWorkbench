#include "Node.h"
#include <QJsonObject>
#include "ChannelStrip.h"
#include "PluginHost.h"
#include "MidiFilePlayer.h"


Node* Node::create(Node* parent, const QJsonObject& stateToLoad)
{
    const auto& type = stateToLoad["type"].toString();
    if (type == "ChannelStrip")
    {
        auto* channelStrip = new ChannelStrip(parent);
        channelStrip->loadState(stateToLoad);
        return channelStrip;
    }

    if (type == "PluginHost")
    {
        auto* pluginHost = new PluginHost(parent);
        pluginHost->loadState(stateToLoad);
        return pluginHost;
    }

    if (type == "MidiFilePlayer")
    {
        auto* node = new MidiFilePlayer(parent);
        node->loadState(stateToLoad);
        return node;
    }

    qDebug() << "type of node not found:" << type;
    return nullptr;
}

Node::Node() : QObject{nullptr}, m_type{Type::Root}
{}

Node::Node(Node* parent, const Type type) : QObject(parent), m_type{type}
{
}

Node::~Node() = default;

Node::Type Node::type() const
{
    return m_type;
}

QString Node::name() const
{
    return m_name;
}

void Node::setName(const QString& name)
{
    if (name == m_name)
        return;

    m_name = name;
    emit nameChanged();
}

bool Node::isByPassed() const
{
    return status.load().isBypassed;
}

void Node::setIsByPassed(const bool newValue)
{
    auto status_ = status.load();
    if (newValue == status_.isBypassed)
        return;

    status_.isBypassed = newValue;
    status.store(status_);
    emit isByPassedChanged();
}

QList<Node*> Node::nodes() const
{
    return m_nodes;
}

void Node::clearNodes()
{
    for (const auto* node : m_nodes)
        delete node;

    m_nodes.clear();
}
