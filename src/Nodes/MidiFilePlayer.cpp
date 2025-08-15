#include "MidiFilePlayer.h"


MidiFilePlayer::MidiFilePlayer(Node* parent) : Node(parent) {}

MidiFilePlayer::~MidiFilePlayer() = default;

void MidiFilePlayer::setPorts(int, float**, int, float**) {}

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
