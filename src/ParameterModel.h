#pragma once
#include <clap/id.h>
#include <clap/helpers/plugin-proxy.hh>
#include <QAbstractItemModel>


class PluginHost;

constexpr auto PluginHost_MH = clap::helpers::MisbehaviourHandler::Terminate;
constexpr auto PluginHost_CL = clap::helpers::CheckingLevel::Maximal;

using PluginProxy = clap::helpers::PluginProxy<PluginHost_MH, PluginHost_CL>;
extern template class clap::helpers::PluginProxy<PluginHost_MH, PluginHost_CL>;


struct Parameter
{
    clap_id id;
    double value;
};

class ParameterModel final : public QAbstractListModel
{
    Q_OBJECT

  public:
    enum Roles
    {
        IdRole = Qt::UserRole + 1,
        NameRole,
        MinRole,
        MaxRole,
        ReadOnlyRole,
        SteppedRole,
        ValueRole,
        TextValueRole
    };

    explicit ParameterModel(PluginHost& plugin, PluginProxy& pluginProxy, QObject* parent = nullptr);

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] const std::vector<Parameter>& getValues() const { return m_values; };


  public slots:
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    [[nodiscard]] int rowCount(const QModelIndex&) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    [[nodiscard]] QString getTextValue(clap_id id, double value) const;


    void startGesture(clap_id id);
    void stopGesture(clap_id id);
    void setValue(clap_id id, double newValue);
    void setValue(int row, clap_id id, double newValue);
    void setValueFromEngine(clap_id id, double newValue);


  private:
    friend class ChangeParameterValueCommand;
    PluginHost& m_plugin;
    PluginProxy& m_pluginProxy;

    std::vector<Parameter> m_values;
    double m_gestureInitialValue = 0.0;

    Parameter& getFromId(clap_id id);
};
