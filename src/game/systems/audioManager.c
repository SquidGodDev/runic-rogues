#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "game/systems/audioManager.h"

#define DEFAULT_VELOCITY 1
#define DEFAULT_PLAY_LEN 3.0f
#define DEFAULT_VOLUME 0.5f

typedef struct SFXData {
    int defaultNote;
    float volume;
    SoundWaveform waveform;
    float attack;
    float decay;
    float sustain;
    float release;
    float curvature;
    int lfo;
    LFOType lfoType;
    float depth;
    float center;
    float rate;
    float phase;
} SFXData;

SFXData sfxData[] = {
    [SFX_SHOOT] = {
        .defaultNote = 25,
        .volume = 0.21f,
        .waveform = kWaveformNoise,
        .attack = 0.09f,
        .decay = 0.39f,
        .sustain = 0,
        .release = 0.09f,
        .curvature = 0.52f,
        .lfo = 0
    },
    [SFX_WALL_HIT] = {
        .defaultNote = 31,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0,
        .decay = 0.12f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_ENEMY_HIT] = {
        .defaultNote = 29,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0,
        .decay = 0.22f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_ENEMY_DEATH] = {
        .defaultNote = 14,
        .volume = 0.26f,
        .waveform = kWaveformNoise,
        .attack = 0.18f,
        .decay = 0.43f,
        .sustain = 0,
        .release = 0.26f,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_PLAYER_HIT] = {
        .defaultNote = 36,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0,
        .decay = 0.5f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.63f,
        .center = 0.5f,
        .rate = 5.0f,
        .phase = 0
    },
    [SFX_LIGHTNING] = {
        .defaultNote = 29,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0.02f,
        .decay = 0.41f,
        .sustain = 0,
        .release = 0,
        .curvature = 0.62f,
        .lfo = 0
    }
};

PDSynth *synths[SFX_COUNT];

void createSynth(SFX sfx) {
    SFXData data = sfxData[sfx];
    PDSynth *synth = pd->sound->synth->newSynth();
    pd->sound->synth->setWaveform(synth, data.waveform);
    pd->sound->synth->setVolume(synth, data.volume, data.volume);

    if (data.lfo) {
        PDSynthLFO *lfo = pd->sound->lfo->newLFO(data.lfoType);
        pd->sound->lfo->setRate(lfo, data.rate);
        pd->sound->lfo->setPhase(lfo, data.phase);
        pd->sound->lfo->setDepth(lfo, data.depth);
        pd->sound->lfo->setCenter(lfo, data.center);
        pd->sound->synth->setFrequencyModulator(synth, (PDSynthSignalValue *)lfo);
    }

    PDSynthEnvelope *envelope = pd->sound->synth->getEnvelope(synth);
    pd->sound->envelope->setAttack(envelope, data.attack);
    pd->sound->envelope->setDecay(envelope, data.decay);
    pd->sound->envelope->setSustain(envelope, data.sustain);
    pd->sound->envelope->setRelease(envelope, data.release);
    pd->sound->envelope->setCurvature(envelope, data.curvature);

    synths[sfx] = synth;
}

// Scaffolding for future audio file loading. Populate these alongside SFX_FILE_COUNT
// and MUSIC_COUNT in audioManager.h, then dispatch in loadAudioAsset below.
// static const char *sfxFilePathStrings[SFX_FILE_COUNT] = { };
// static AudioSample *sfxFiles[SFX_FILE_COUNT];
// static const char *musicPathStrings[MUSIC_COUNT] = { };
// static FilePlayer *musicFiles[MUSIC_COUNT];

int getAudioAssetCount(void) {
    return SFX_COUNT + SFX_FILE_COUNT + MUSIC_COUNT;
}

void loadAudioAsset(int step) {
    if (step < SFX_COUNT) {
        createSynth(step);
        return;
    }
    step -= SFX_COUNT;

    if (step < SFX_FILE_COUNT) {
        // sfxFiles[step] = pd->sound->sample->load(sfxFilePathStrings[step]);
        return;
    }
    step -= SFX_FILE_COUNT;

    if (step < MUSIC_COUNT) {
        // musicFiles[step] = pd->sound->fileplayer->newPlayer();
        // pd->sound->fileplayer->loadIntoPlayer(musicFiles[step], musicPathStrings[step]);
        return;
    }
}

void playSound(SFX sfx) {
    if (sfx < 0 || sfx >= SFX_COUNT) return;

    SFXData data = sfxData[sfx];
    pd->sound->synth->playMIDINote(synths[sfx], data.defaultNote, DEFAULT_VELOCITY, DEFAULT_PLAY_LEN, 0);
}