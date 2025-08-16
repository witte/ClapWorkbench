#include "MidiFilePlayer.h"


MidiFilePlayer::MidiFilePlayer(Node* parent) : Node(parent, Type::MidiFilePlayer) {}

MidiFilePlayer::~MidiFilePlayer() = default;

void MidiFilePlayer::setPorts(int, float**, int, float** outputs)
{
    buffer[0] = outputs[0];
    buffer[1] = outputs[1];
}

void MidiFilePlayer::activate(std::int32_t /*sampleRate*/, std::int32_t /*blockSize*/)
{
    qDebug() << "MidiFilePlayer::activate";
}

void MidiFilePlayer::deactivate() {}

void MidiFilePlayer::startProcessing()
{
    qDebug() << "MidiFilePlayer::startProcessing";
}

void MidiFilePlayer::stopProcessing() {}

void MidiFilePlayer::processNoteRawMidi(int /*sampleOffset*/, const std::vector<unsigned char>& /*data*/) {}

void MidiFilePlayer::process() {}

float MidiFilePlayer::outputVolume() const
{
    return m_outputVolume;
}

void MidiFilePlayer::setOutputVolume(float /*newOutputVolume*/) {}

const QString& MidiFilePlayer::filePath() const
{
    return m_filePath;
}

void MidiFilePlayer::setFilePath(const QString& /*newFilePath*/) {}

QJsonObject MidiFilePlayer::getState() const
{
    QJsonObject state;
    state["type"] = "MidiFilePlayer";
    state["name"] = m_name;
    state["nodes"] = {};

    return state;
}

void MidiFilePlayer::loadState(const QJsonObject& stateToLoad)
{
    m_name = stateToLoad["name"].toString();
}
