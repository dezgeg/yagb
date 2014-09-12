#pragma once

#include "BusUtil.hpp"
#include "Logger.hpp"
#include "Platform.hpp"

enum {
    Snd_Ch1_Sweep = 0x10,
    Snd_Ch1_Wave = 0x11,
    Snd_Ch1_Envelope = 0x12,
    Snd_Ch1_FreqLo = 0x13,
    Snd_Ch1_FreqHi = 0x14,

    Snd_Ch2_Wave = 0x16,
    Snd_Ch2_Envelope = 0x17,
    Snd_Ch2_FreqLo = 0x18,
    Snd_Ch2_FreqHi = 0x19,

    Snd_Ch3_Enable = 0x1a,
    Snd_Ch3_Length = 0x1b,
    Snd_Ch3_Volume = 0x1c,
    Snd_Ch3_FreqLo = 0x1d,
    Snd_Ch3_FreqHi = 0x1e,
    Snd_Ch3_WaveRam = 0x30,

    Snd_Ch4_Length = 0x20,
    Snd_Ch4_Envelope = 0x21,
    Snd_Ch4_Poly = 0x22,
    Snd_Ch4_Control = 0x23,

    Snd_Ctrl_Volume = 0x24,
    Snd_Ctrl_ChSel = 0x25,
    Snd_Ctrl_Stat = 0x26,
};

union EnvelopeRegs {
    Byte bits;
    struct {
        Byte sweep : 3;
        Byte increase : 1;
        Byte initialVolume : 4;
    };
};
static_assert(sizeof(EnvelopeRegs) == 1, "");

struct TimerState {
    unsigned long lengthCounterStartCycle;

    bool lengthTimerRunning() {
        return lengthCounterStartCycle != 0;
    }
};

struct FrequencyRegs {
    Byte low;
    union {
        Byte control;
        struct {
            Byte high : 3;
            Byte _unused : 3;
            Byte noRestart : 1;
            Byte start : 1;
        };
    };

    Word getFrequency() {
        return (high << 8) | low;
    }
};

static_assert(sizeof(FrequencyRegs) == 2, "");

struct SquareChannelRegs {
    union {
        Byte wave;
        struct {
            Byte soundLength : 6;
            Byte waveDuty : 2;
        };
    };
    EnvelopeRegs envelope;
    FrequencyRegs freqCtrl;
};

struct SoundRegs {
    struct {
        union {
            Byte sweep;
            struct {
                Byte sweepShift : 3;
                Byte sweepDecrease : 1;
                Byte sweepTime : 3;
            };
        };
        SquareChannelRegs square;
    } ch1;

    Byte _unused1; // 0xff15

    struct {
        SquareChannelRegs square;
    } ch2;

    struct {
        Byte enable;
        Byte length;
        struct {
            Byte _unused : 5;
            Byte volume : 2;
        };
        FrequencyRegs freqCtrl;
    } ch3;

    Byte _unused2; // 0xff1f

    struct {
        Byte length;
        EnvelopeRegs envelope;
        union {
            Byte polynomial;
            struct {
                Byte polyDivider : 3;
                Byte counterShort : 1;
                Byte frequency : 4;
            };
        };
        union {
            Byte control;
            struct {
                Byte _unused : 6;
                Byte noRestart : 1;
                Byte start : 1;
            };
        };
    } ch4;

    struct {
        union {
            Byte volume;
            struct {
                Byte leftVolume : 3;
                Byte leftVin : 1;
                Byte rightVolume : 3;
                Byte rightVin : 1;
            };
        };
        Byte chSel;
        Byte stat;
    } ctrl;

    Byte _unused3[9]; // 0xff27 -- 0xff2f
    Byte waveRam[16];
};
static_assert(sizeof(SoundRegs::ch1) == 5, "Ch1 incorrect");
static_assert(sizeof(SoundRegs::ch2) == 4, "Ch2 incorrect");
static_assert(sizeof(SoundRegs::ch3) == 5, "Ch3 incorrect");
static_assert(sizeof(SoundRegs::ch4) == 4, "Ch4 incorrect");
static_assert(sizeof(SoundRegs::ctrl) == 3, "Control regs incorrect");

static_assert(sizeof(SoundRegs) == (0xff3f - 0xff10 + 1), "Sound regs incorrect");

class Sound {
    Logger* log;
    SoundRegs regs;
    struct {
        TimerState ch1;
        TimerState ch2;
        TimerState ch3; // no real envelope but used as enable/disable counter
        TimerState ch4;
    } timers;

    unsigned long currentCycle;
    long cycleResidue;
    long currentSampleNumber;

    uint16_t leftSample;
    uint16_t rightSample;

public:
    Sound(Logger* log);

    void generateSamples();
    void registerAccess(Word address, Byte* pData, bool isWrite);
    void tick(int cycleDelta);

    int evalPulseChannel(SquareChannelRegs& regs, TimerState& envelState);
    int evalWaveChannel();

    void restartTimer(TimerState& state);
    unsigned int evalEnvelope(EnvelopeRegs& regs, TimerState& state);
    void tickTimer(TimerState& state, Byte curLength, int channelMaxLength, bool noRestart);

    int mixVolume(int sample, unsigned int volume);
    bool evalPulseWaveform(SquareChannelRegs& ch);

    long getCurrentSampleNumber() { return currentSampleNumber; }
    uint16_t getLeftSample() { return leftSample; }
    uint16_t getRightSample() { return rightSample; }
};
