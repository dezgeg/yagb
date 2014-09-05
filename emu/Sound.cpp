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
    if (cycleResidue >= 32768) {
        cycleResidue -= 32768;
        currentSampleNumber++;

        generateSamples();
    }
}

void Sound::generateSamples() {
    bool ch1 = evalSquareChannel(regs.ch1.square);
    unsigned ch1EnvelopeVolume = regs.ch1.square.freqCtrl.start ? evalEnvelope(regs.ch1.square.envelope, envelopes.ch1) : 0;
    // qDebug() << "Envel result: " << ch1EnvelopeVolume;
    int ch1Volume = mixVolume(ch1 ? MaxChanLevel : -MaxChanLevel, ch1EnvelopeVolume);

    rightSample = ch1Volume;
    leftSample = rightSample;
}

// Returns true/false whether the square channel output is high or low
bool Sound::evalSquareChannel(SquareChannelRegs& ch) {
    static const unsigned pulseWidthLookup[] = { 4 * 1, 4 * 2, 4 * 4, 4 * 6 };

    if (!ch.freqCtrl.start) {
        return false;
    }
    unsigned pulseLen = 2048 - ch.freqCtrl.getFrequency();
    unsigned long stepInPulse = currentCycle % (pulseLen * 4 * 8);

    return stepInPulse / pulseLen < pulseWidthLookup[ch.waveDuty];
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
