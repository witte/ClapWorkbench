#include "Commands.h"
#include "ParameterModel.h"


ChangeParameterValueCommand::ChangeParameterValueCommand(ParameterModel& model, const unsigned int parameterId,
    const double oldValue, const double newValue, QUndoCommand* parent)
    : QUndoCommand{parent}, m_model{model}, m_parameterId{parameterId}, oldValue_{oldValue}, newValue_{newValue}
{}

void ChangeParameterValueCommand::undo()
{
    m_model.setValue(m_parameterId, oldValue_);
}

void ChangeParameterValueCommand::redo()
{
    m_model.setValue(m_parameterId, newValue_);
}
