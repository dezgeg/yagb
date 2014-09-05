#pragma once

#include <QAudioFormat>
#include <QAudioOutput>

class AudioHandler : public QIODevice {
Q_OBJECT
    QAudioFormat format;
    QAudioOutput* audioOutput;

    enum {
        SIZE = 65536,
    };

    struct Sample {
        uint16_t left, right;
    } buf[SIZE];

    size_t head;
    size_t tail;

    bool wasFull;

    QAudioFormat createFormat();

private slots:
    void outputStateChanged(QAudio::State st);

protected:
    virtual qint64 readData(char* data, qint64 maxLen) override;
    virtual qint64 writeData(const char* data, qint64 len);

public:
    size_t samplesAvailable() { return (head - tail) % SIZE; }
    void feedSamples(uint16_t left, uint16_t right);

    AudioHandler();
};
