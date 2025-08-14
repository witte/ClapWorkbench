#include "Node.h"


Node::Node(QObject* parent) : QObject(parent)
{
}

Node::~Node() = default;

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
