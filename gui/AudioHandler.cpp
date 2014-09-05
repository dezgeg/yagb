#include <stdint.h>
#include <algorithm>
#include <QDebug>
#include <emu/Utils.hpp>
#include "AudioHandler.hpp"
#include "TimingUtils.hpp"

using namespace std;

void AudioHandler::feedSamples(int16_t left, int16_t right) {
    if ((head + 1) % SIZE == tail) {
        if (!wasFull) {
            TimingUtils::log() << "Audio buffer full!";
        }
        wasFull = true;
        return;
    }
    buf[head].left = left;
    buf[head].right = right;
    head = (head + 1) % SIZE;
}
void AudioHandler::outputStateChanged(QAudio::State state) {
    TimingUtils::log() << "State: " << state << ", error: " << audioOutput->error();
}
QAudioFormat AudioHandler::createFormat() {
    QAudioFormat f;

    f.setCodec("audio/pcm");
    f.setChannelCount(2);
    f.setSampleRate(48000);
    f.setSampleSize(16);
    f.setSampleType(QAudioFormat::SignedInt);
    f.setByteOrder(QAudioFormat::LittleEndian);

    return f;
}

qint64 AudioHandler::readData(char* data, qint64 maxLen) {
    qint64 nSamples = min(qint64(samplesAvailable()), qint64(maxLen / sizeof(Sample)));

    if (!nSamples) {
        // TimingUtils::log() << "Audio buffer underrun: " << maxLen;
        // memset(data, 0, sizeof(Sample));
        // return sizeof(Sample);
        return 0;
    }

    // TimingUtils::log() << "Filling samples: " << nSamples << ", available: " << samplesAvailable();
    for (qint64 i = 0; i < nSamples; ++i) {
        memcpy(data, &buf[tail], sizeof(Sample));
        data += sizeof(Sample);
        tail = (tail + 1) % SIZE;
    }
    wasFull = false;

    return nSamples * sizeof(Sample);
}

qint64 AudioHandler::writeData(const char* data, qint64 len) {
    unreachable();
    return -1;
}
AudioHandler::AudioHandler() : format(createFormat()),
                               audioOutput(new QAudioOutput(format)),
                               head(0),
                               tail(0),
                               wasFull(false) {
    connect(audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(outputStateChanged(QAudio::State)));
    this->open(OpenModeFlag::ReadOnly);
    audioOutput->start(this);
}
