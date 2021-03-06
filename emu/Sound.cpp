#include <string.h>
#include <QDebug>
#include "Sound.hpp"
#include "Utils.hpp"
#include "Serializer.hpp"

#include <bitset>

static constexpr int MaxChanLevel = 0x2000;
static std::bitset<(1 << 15) - 1> lfsr15Table;
static std::bitset<(1 << 7) - 1> lfsr7Table;

static bool stepLfsr(uint32_t& st, bool small) {
    bool out = !(st & 1);
    uint32_t xorOut = ((st & 1) ^ (st >> 1)) & 1;
    st >>= 1;
    st |= xorOut << 14;

    if (small) {
        st &= ~(1 << 6);
        st |= xorOut << 6;
    }

    return out;
}

static struct InitLfsrTables {
    InitLfsrTables() {
        uint32_t st15 = (1 << 15) - 1;
        uint32_t st7 = (1 << 15) - 1;
        for (unsigned i = 0; i < bit(15); ++i) {
            lfsr15Table[i] = stepLfsr(st15, false);
            if (i < bit(7)) {
                lfsr7Table[i] = stepLfsr(st7, true);
            }
        }
    }
} _initLfsrTables;

void Sound::registerAccess(Word address, Byte* pData, bool isWrite) {
    if (address == 0xff15 || address == 0xff1f ||
            (address >= 0xff27 && address <= 0xff2f)) {
        log->warn("Unimplemented sound reg 0x%04x", address);
        return;
    }

    // TODO: mask
    Byte reg = address - 0xff10;
    // qDebug() << "Register write: " << reg;
    BusUtil::arrayMemAccess((Byte*)&regs, reg, pData, isWrite);

    if (!isWrite || (*pData & 0x80) == 0) {
        return;
    }

    switch (address & 0xff) {
        case Snd_Ch1_FreqHi:
            // case Snd_Ch1_Envelope:
            restartTimer(timers.ch1);
            break;

        case Snd_Ch2_FreqHi:
            // case Snd_Ch2_Envelope:
            restartTimer(timers.ch2);
            break;

        case Snd_Ch3_FreqHi:
            restartTimer(timers.ch3);
            break;

        case Snd_Ch4_Control:
            restartTimer(timers.ch4);
            break;
    }
}

void Sound::tick(int cycleDelta) {
    currentCycle += cycleDelta;
    cycleResidue += 375 * cycleDelta;

    tickTimer(timers.ch1, regs.ch1.square.soundLength, 64, regs.ch1.square.freqCtrl.noRestart);
    tickTimer(timers.ch2, regs.ch2.square.soundLength, 64, regs.ch2.square.freqCtrl.noRestart);
    tickTimer(timers.ch3, regs.ch3.length, 256, regs.ch3.freqCtrl.noRestart);
    tickTimer(timers.ch4, regs.ch4.length, 64, regs.ch4.noRestart);

    if (cycleResidue >= 32768) {
        cycleResidue -= 32768;
        currentSampleNumber++;

        generateSamples();
    }
}

void Sound::generateSamples() {
    int sounds[] = {
            evalPulseChannel(regs.ch1.square, timers.ch1),
            evalPulseChannel(regs.ch2.square, timers.ch2),
            evalWaveChannel(),
            evalNoiseChannel(),
    };

    int leftDac = 0, rightDac = 0;
    for (int i = 0; i < 4; ++i) {
        if (regs.ctrl.chSel & bit(i)) {
            leftDac += sounds[i];
        }
        if (regs.ctrl.chSel & bit(i + 4)) {
            rightDac += sounds[i];
        }
    }

    leftSample = leftDac * (regs.ctrl.leftVolume + 1) / 16;
    rightSample = rightDac * (regs.ctrl.rightVolume + 1) / 16;
}

int Sound::evalWaveChannel() {
    static const unsigned shiftLookup[] = { 4, 0, 1, 2 };
    static const int sampleLookup[] = {
            -8192, -7100, -6007, -4915,
            -3823, -2731, -1638, -546,
            546, 1638, 2731, 3823,
            4915, 6007, 7100, 8192,
    };

    if (!timers.ch3.lengthTimerRunning() || !regs.ch3.enable) {
        return 0;
    }

    // TODO: make some sense of this code
    unsigned period = (2048 - regs.ch3.freqCtrl.getFrequency());
    unsigned long cycleDelta = currentCycle - timers.ch3.frequencyCounterStartCycle;
    unsigned cycleInWave = cycleDelta % (period * 32 * 2);
    unsigned pointer = ((cycleInWave / period) / 2 + 1) % 32;

#if 0
    char buf[100] = "";
    char buf2[8];
    for (unsigned i = 0; i < 16; ++i) {
        sprintf(buf2, "%x %x ", regs.waveRam[i] >> 4, regs.waveRam[i] & 0xf);
        strcat(buf, buf2);
    }
    log->warn("Wave pointer %02d: %s", pointer, buf);
#endif

    unsigned sample = (regs.waveRam[pointer >> 1] >> (pointer & 1 ? 0 : 4)) & 0xf;
    sample >>= shiftLookup[regs.ch3.volume];
    // qDebug() << "Wave sample: " << sample << " at pointer: " << ((cycleInWave / period) / 2 + 1);

    return sampleLookup[sample];
}

int Sound::evalPulseChannel(SquareChannelRegs& regs, TimerState& timerState) {
    if (!timerState.lengthTimerRunning()) {
        return 0;
    }

    bool pulseOn = evalPulseWaveform(regs);
    unsigned pulseVolume = evalEnvelope(regs.envelope, timerState);
    // qDebug() << "Envel result: " << pulseVolume;
    return mixVolume(pulseOn ? MaxChanLevel : -MaxChanLevel, pulseVolume);
}

// Returns true/false whether the square channel output is high or low
bool Sound::evalPulseWaveform(SquareChannelRegs& ch) {
    static const unsigned pulseWidthLookup[] = { 4 * 1, 4 * 2, 4 * 4, 4 * 6 };

    unsigned pulseLen = 2048 - ch.freqCtrl.getFrequency();
    unsigned long stepInPulse = currentCycle % (pulseLen * 4 * 8);

    return stepInPulse / pulseLen < pulseWidthLookup[ch.waveDuty];
}

int Sound::evalNoiseChannel() {
    if (!timers.ch4.lengthTimerRunning() || !regs.ch4.start) {
        return 0;
    }
    unsigned pulseLength = regs.ch4.polyDivider ? 16 * regs.ch4.polyDivider : 8;
    pulseLength *= bit(regs.ch4.frequency + 1) / 2; // TODO: why /2?
    unsigned lfsrWidth = regs.ch4.counterShort ? bit(7) - 1 : bit(15) - 1;

    unsigned long cycleDelta = currentCycle - timers.ch4.frequencyCounterStartCycle;
    unsigned long stepInPulse = (cycleDelta / pulseLength) % lfsrWidth;
    bool pulse = regs.ch4.counterShort ? lfsr7Table[stepInPulse] : lfsr15Table[stepInPulse];

    unsigned volume = evalEnvelope(regs.ch4.envelope, timers.ch4);
    return mixVolume(pulse ? MaxChanLevel : -MaxChanLevel, volume);
}

void Sound::tickTimer(TimerState& state, Byte curLength, int channelMaxLength, bool noRestart) {
    long cycleDelta = currentCycle - state.lengthCounterStartCycle;

    if (noRestart && cycleDelta >> 16 > (channelMaxLength - curLength)) {
        state.lengthCounterStartCycle = 0;
    }
}

void Sound::restartTimer(TimerState& state) {
    // qDebug() << "Restart envelope at cycle " << currentCycle;
    state.lengthCounterStartCycle = currentCycle;
    state.frequencyCounterStartCycle = currentCycle;
}

unsigned int Sound::evalEnvelope(EnvelopeRegs& regs, TimerState& state) {
    if (regs.sweep == 0) {
        return regs.initialVolume;
    }

    long steps = (currentCycle - state.lengthCounterStartCycle) / ((1 << 16) * regs.sweep);
    // qDebug() << "Envelope steps: " << steps << " at cycle " << currentCycle;
    return clamp(regs.initialVolume + (regs.increase ? steps : -steps), 0, 0xf);
}

int Sound::mixVolume(int sample, unsigned int volume) {
    static const int lookup[] = {
            0, 546, 1092, 1638,
            2185, 2731, 3277, 3823,
            4369, 4915, 5461, 6007,
            6554, 7100, 7646, 8192,
    };
    return (long(lookup[volume]) * sample) / 8192;
}

Sound::Sound(Logger* log) : log(log),
                            currentCycle(),
                            cycleResidue(),
                            currentSampleNumber(),
                            leftSample(),
                            rightSample() {
    memset(&regs, 0, sizeof(regs));
    memset(&timers, 0, sizeof(timers));
}

void Sound::serialize(Serializer& ser) {
    ser.handleObject("Sound.currentCycle", currentCycle);
    ser.handleObject("Sound.cycleResidue", cycleResidue);
    ser.handleObject("Sound.currentSampleNumber", currentSampleNumber);
    ser.handleObject("Sound.leftSample", leftSample);
    ser.handleObject("Sound.rightSample", rightSample);
    ser.handleObject("Sound.regs", regs);
    ser.handleObject("Sound.timers", timers);
}
