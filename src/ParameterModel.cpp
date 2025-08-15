#include "ParameterModel.h"
#include "Nodes/PluginHost.h"
#include "AudioEngine.h"
#include "Commands.h"


ParameterModel::ParameterModel(PluginHost& plugin, PluginProxy& pluginProxy, QObject* parent)
    : QAbstractListModel{parent}
    , m_plugin{plugin}
    , m_pluginProxy{pluginProxy}
{
    if (!m_pluginProxy.canUseParams())
        return;

    for (auto i = 0u; i < m_pluginProxy.paramsCount(); ++i)
    {
        clap_param_info info{};
        m_pluginProxy.paramsGetInfo(i, &info);

        m_values.push_back({info.id, info.default_value});
        m_plugin.setParamValue(info.id, info.default_value);
    }
}

QHash<int, QByteArray> ParameterModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[MinRole] = "min";
    roles[MaxRole] = "max";
    roles[ReadOnlyRole] = "readOnly";
    roles[SteppedRole] = "stepped";
    roles[ValueRole] = "value";
    return roles;
}

Qt::ItemFlags ParameterModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractListModel::flags(index);
    if (index.isValid())
        f |= Qt::ItemNeverHasChildren | Qt::ItemIsEditable;

    return f;
}

int ParameterModel::rowCount(const QModelIndex&) const
{
    if (m_plugin.status.load().status == S::OnError)
        return 0;

    if (!m_pluginProxy.canUseParams())
        return 0;

    return static_cast<int>(m_pluginProxy.paramsCount());
}

int ParameterModel::columnCount(const QModelIndex&) const
{
    return 1;
}

QVariant ParameterModel::data(const QModelIndex& index, int role) const
{
    switch (role)
    {
        case IdRole:    return m_values[index.row()].id;
        case ValueRole: return m_values[index.row()].value;
        default: break;
    }

    clap_param_info info{};
    m_pluginProxy.paramsGetInfo(index.row(), &info);

    switch (role)
    {
        case NameRole:      return QString{info.name};
        case MinRole:       return info.min_value;
        case MaxRole:       return info.max_value;
        case ReadOnlyRole:  return (info.flags & CLAP_PARAM_IS_READONLY) != 0;
        case SteppedRole:   return (info.flags & CLAP_PARAM_IS_STEPPED) != 0;
        default: return {};
    }
}

bool ParameterModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != ValueRole)
        return false;

    auto& parameter = m_values[index.row()];

    const auto newValue = value.toDouble();

    if (qFuzzyCompare(parameter.value, newValue))
        return false;

    m_plugin.setParamValue(parameter.id, newValue);
    parameter.value = newValue;

    emit dataChanged(index, index, {ValueRole});

    return true;
}

void ParameterModel::startGesture(const clap_id id)
{
    m_gestureInitialValue = getFromId(id).value;
}

void ParameterModel::stopGesture(const clap_id id)
{
    const auto&[_, value] = getFromId(id);
    if (qFuzzyCompare(m_gestureInitialValue, value))
        return;

    AudioEngine::instance()->undoStack().push(
        new ChangeParameterValueCommand(*this, id, m_gestureInitialValue, value));
}

void ParameterModel::setValue(const clap_id id, const double newValue)
{
    for (size_t i = 0; i < m_values.size(); ++i)
    {
        auto& parameter = m_values[i];
        if (parameter.id != id)
            continue;

        if (qFuzzyCompare(parameter.value, newValue))
            return;

        m_plugin.setParamValue(parameter.id, newValue);
        parameter.value = newValue;

        auto index = createIndex(static_cast<int>(i), 0);
        emit dataChanged(index, index, {ValueRole});
    }
}

void ParameterModel::setValue(const int row, clap_id, const double newValue)
{
    auto&[id, value] = m_values[row];

    if (qFuzzyCompare(value, newValue))
        return;

    m_plugin.setParamValue(id, newValue);
    value = newValue;

    const auto index = createIndex(row, 0);
    emit dataChanged(index, index, {ValueRole});
}

void ParameterModel::setValueFromEngine(const clap_id id, const double newValue)
{
    for (size_t i = 0; i < m_values.size(); ++i)
    {
        auto&[parameterId, parameterValue] = m_values[i];
        if (parameterId != id)
            continue;

        if (qFuzzyCompare(parameterValue, newValue))
            return;

        parameterValue = newValue;

        auto index = createIndex(static_cast<int>(i), 0);
        emit dataChanged(index, index, {ValueRole});
    }
}

// REMOVE ME???
Parameter& ParameterModel::getFromId(const clap_id id)
{
    for (auto& parameter : m_values)
    {
        if (parameter.id == id)
            return parameter;
    }

    return m_values.front();
}

QString ParameterModel::getTextValue(const clap_id id, const double value) const
{
    std::string text;
    text.resize(256);

    if (m_pluginProxy.paramsValueToText(id, value, text.data(), text.size()))
    {
        // Clion says the `.c_str()` is redundant but we get trash in the TextValue without it
        const auto textAsCharP = text.c_str();
        return QString::fromStdString(textAsCharP);
    }

    return QString::number(value);
}
