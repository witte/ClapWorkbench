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
