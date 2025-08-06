#pragma once
#include "Node.h"
#include "PluginHost.h"


class ChannelStrip final : public Node
{
public:
    ~ChannelStrip() override;

    void setPorts(int numInputs, float** inputs, int numOutputs, float** outputs) override;
    void activate(std::int32_t sampleRate, std::int32_t blockSize) override;
    void deactivate() override;

    void startProcessing() override;
    void stopProcessing() override;

    void processNoteRawMidi(int sampleOffset, const std::vector<unsigned char>& data) override;
    void process() override;

    void clearNodes();

    QList<PluginHost*> nodes;
};
