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
    size_t samplesAvailable() { return (head - tail) % SIZE; }

private slots:
    void outputStateChanged(QAudio::State st);

protected:
    virtual qint64 readData(char* data, qint64 maxLen) override;
    virtual qint64 writeData(const char* data, qint64 len);

public:
    void feedSamples(int16_t left, int16_t right);

    AudioHandler();
};
