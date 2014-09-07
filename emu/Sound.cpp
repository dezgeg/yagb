#include <string.h>
#include <QDebug>
#include "Sound.hpp"
#include "Utils.hpp"

static constexpr int MaxChanLevel = 0xFFF;

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

    switch (address & 0xff) {
        case Snd_Ch1_FreqHi:
        case Snd_Ch1_Envelope:
            restartEnvelope(envelopes.ch1);
            break;

        case Snd_Ch2_FreqHi:
        case Snd_Ch2_Envelope:
            restartEnvelope(envelopes.ch2);
            break;

        case Snd_Ch4_Envelope:
            // TODO: need control register here
            restartEnvelope(envelopes.ch4);
            break;
    }
}

void Sound::tick(int cycleDelta) {
    currentCycle += cycleDelta;
    cycleResidue += 375 * cycleDelta;

    if (regs.ch1.square.freqCtrl.noRestart) {
        tickEnvelope(envelopes.ch1, regs.ch1.square.soundLength, 64);
    }
    if (regs.ch2.square.freqCtrl.noRestart) {
        tickEnvelope(envelopes.ch2, regs.ch2.square.soundLength, 64);
    }

    if (cycleResidue >= 32768) {
        cycleResidue -= 32768;
        currentSampleNumber++;

        generateSamples();
    }
}

void Sound::generateSamples() {
    int ch1Volume = evalPulseChannel(regs.ch1.square, envelopes.ch1);
    int ch2Volume = evalPulseChannel(regs.ch2.square, envelopes.ch2);

    rightSample = ch1Volume + ch2Volume;
    leftSample = rightSample;
}

int Sound::evalPulseChannel(SquareChannelRegs& regs, EnvelopeState& envelState) {
    if (!envelState.startCycle)
        return 0;

    bool ch1 = evalPulseWaveform(regs);
    unsigned ch1EnvelopeVolume = regs.freqCtrl.start ? evalEnvelope(regs.envelope, envelState) : 0;
    // qDebug() << "Envel result: " << ch1EnvelopeVolume;
    return mixVolume(ch1 ? MaxChanLevel : -MaxChanLevel, ch1EnvelopeVolume);
}

// Returns true/false whether the square channel output is high or low
bool Sound::evalPulseWaveform(SquareChannelRegs& ch) {
    static const unsigned pulseWidthLookup[] = { 4 * 1, 4 * 2, 4 * 4, 4 * 6 };

    if (!ch.freqCtrl.start) {
        return false;
    }
    unsigned pulseLen = 2048 - ch.freqCtrl.getFrequency();
    unsigned long stepInPulse = currentCycle % (pulseLen * 4 * 8);

    return stepInPulse / pulseLen < pulseWidthLookup[ch.waveDuty];
}

void Sound::tickEnvelope(EnvelopeState& state, unsigned curLength, unsigned channelMaxLength) {
    if ((currentCycle - state.startCycle) >> 16 > (channelMaxLength - curLength)) {
        state.startCycle = 0;
    }
}

void Sound::restartEnvelope(EnvelopeState& state) {
    // qDebug() << "Restart envelope at cycle " << currentCycle;
    state.startCycle = currentCycle;
}

unsigned int Sound::evalEnvelope(EnvelopeRegs& regs, EnvelopeState& state) {
    if (regs.sweep == 0) {
        return regs.initialVolume;
    }

    long steps = (currentCycle - state.startCycle) / ((1 << 16) * regs.sweep);
    // qDebug() << "Envelope steps: " << steps << " at cycle " << currentCycle;
    return clamp(regs.initialVolume + (regs.increase ? steps : -steps), 0, 0xf);
}

int Sound::mixVolume(int sample, unsigned int volume) {
    return sample * (int)volume / 0xf;
}

Sound::Sound(Logger* log) : log(log),
                            regs(),
                            currentCycle(),
                            cycleResidue(),
                            currentSampleNumber(),
                            leftSample(),
                            rightSample() {
    memset(&envelopes, 0, sizeof(envelopes));
}
