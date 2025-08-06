#include "ChannelStrip.h"

ChannelStrip::~ChannelStrip() = default;

void ChannelStrip::setPorts(const int numInputs, float** inputs, const int numOutputs, float** outputs)
{
    for (auto* node : nodes)
    {
        node->setPorts(numInputs, inputs, numOutputs, outputs);
    }
}

void ChannelStrip::activate(const std::int32_t sampleRate, const std::int32_t blockSize)
{
    for (auto* node : nodes)
    {
        node->activate(sampleRate, static_cast<int>(blockSize));
    }
}

void ChannelStrip::deactivate()
{
    for (auto* node: nodes)
        node->deactivate();
}

void ChannelStrip::startProcessing()
{
    for (auto* plugin : nodes)
    {
        if (plugin->status() >= ocp::Status::OnHold)
            plugin->startProcessing();
    }
}

void ChannelStrip::stopProcessing()
{
    for (auto* plugin : nodes)
        plugin->stopProcessing();
}
void ChannelStrip::processNoteRawMidi(const int sampleOffset, const std::vector<unsigned char>& data)
{
    for (auto* plugin : nodes)
        plugin->processNoteRawMidi(sampleOffset, data);
}

void ChannelStrip::process()
{
    for (auto* plugin : nodes)
        plugin->process();
}

void ChannelStrip::clearNodes()
{
    for (const auto* node : nodes)
        delete node;

    nodes.clear();
}