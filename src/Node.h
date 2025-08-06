#pragma once
#include <cstdint>
#include <vector>


class Node
{
  public:
    virtual ~Node() noexcept;

    virtual void setPorts(int numInputs, float** inputs, int numOutputs, float** outputs) = 0;
    virtual void activate(std::int32_t sampleRate, std::int32_t blockSize) = 0;
    virtual void deactivate() = 0;

    virtual void startProcessing() = 0;
    virtual void stopProcessing() = 0;

    virtual void processNoteRawMidi(int sampleOffset, const std::vector<unsigned char>& data) = 0;
    virtual void process() = 0;
};
