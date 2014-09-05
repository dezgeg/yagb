#include "Sound.hpp"

void Sound::registerAccess(Word address, Byte* pData, bool isWrite) {
    if (address == 0xff15 || address == 0xff1f ||
            (address >= 0xff27 && address <= 0xff2f)) {
        log->warn("Unimplemented sound reg 0x%04x", address);
        return;
    }

    // TODO: mask
    BusUtil::arrayMemAccess((Byte*)&regs, address - 0xff10, pData, isWrite);
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

    rightSample = ch1 ? 0xFFF : -0xFFF;
    leftSample = rightSample;
}
bool Sound::evalSquareChannel(SquareChannelRegs& ch) {
    static const unsigned pulseWidthLookup[] = { 4 * 1, 4 * 2, 4 * 4, 4 * 6 };

    if (!ch.freqCtrl.start) {
        return false;
    }
    unsigned pulseLen = 2048 - ch.freqCtrl.getFrequency();
    unsigned long stepInPulse = currentCycle % (pulseLen * 4 * 8);

    return stepInPulse / pulseLen < pulseWidthLookup[ch.waveDuty];
}
