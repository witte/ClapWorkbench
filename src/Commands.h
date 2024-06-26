#pragma once
#include <QUndoCommand>


class ParameterModel;


class ChangeParameterValueCommand final : public QUndoCommand
{
  public:
    enum { Id = 1234 };
    ChangeParameterValueCommand(ParameterModel& model, unsigned int parameterId, double oldValue, double newValue, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    [[nodiscard]] int id() const override { return Id; }


  private:
    ParameterModel& m_model;
    unsigned int m_parameterId;
    double oldValue_;
    double newValue_;
};
