#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "game/settings/settings.h"
#include "game/systems/audioManager.h"
#include "game/player/player.h"

#define DEFAULT_VELOCITY 1
#define DEFAULT_PLAY_LEN 3.0f
#define DEFAULT_VOLUME 0.5f

// Spatial playback (playSoundSpatial). Sources at or within FALLOFF_MIN units play at
// full per-SFX volume; volume rolls off to silence at FALLOFF_MAX, beyond which the
// sound is culled entirely. PAN_STRENGTH is how hard a fully off-centre source ducks
// the opposite channel (0 == mono, 1 == fully silence the far side). Note the Playdate
// speaker is mono, so panning is only audible on headphones / in the Simulator.
#define SFX_FALLOFF_MIN 1.0f * CELL_SIZE
#define SFX_FALLOFF_MAX 8.0f * CELL_SIZE
#define SFX_PAN_STRENGTH 0.8f

// Pooled voices per SFX. Voices give polyphony so overlapping plays of the same SFX
// (e.g. several enemies acting at once) don't steal each other. SFX leave `voices` unset
// to take DEFAULT_SFX_VOICES; set it explicitly (e.g. 1) to override.
#define MAX_SFX_VOICES 4
#define DEFAULT_SFX_VOICES 3

typedef struct SFXData {
    int defaultNote;
    float pitchRandomization;  // semitones of +/- jitter around defaultNote; 0 == off
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
    int voices;  // polyphony for overlapping plays; 0 == DEFAULT_SFX_VOICES
} SFXData;

SFXData sfxData[] = {
    [SFX_FIREBALL] = {
        .defaultNote = 25,
        .pitchRandomization = 4.3f,
        .volume = 0.21f,
        .waveform = kWaveformNoise,
        .attack = 0.09f,
        .decay = 0.39f,
        .sustain = 0,
        .release = 0.09f,
        .curvature = 0.52f,
        .lfo = 0
    },
    [SFX_LIGHTNING] = {
        .defaultNote = 29,
        .pitchRandomization = 1.4f,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0.02f,
        .decay = 0.41f,
        .sustain = 0,
        .release = 0,
        .curvature = 0.62f,
        .lfo = 0
    },
    [SFX_SWORD] = {
        .defaultNote = 40,
        .pitchRandomization = 0.2f,
        .volume = 0.1f,
        .waveform = kWaveformNoise,
        .attack = 0.07f,
        .decay = 0.31f,
        .sustain = 0,
        .release = 0,
        .curvature = 0,
        .lfo = 0
    },
    [SFX_SHOUT] = {
        .defaultNote = 33,
        .pitchRandomization = 1.0f,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0.23f,
        .decay = 0.52f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_KNIFE] = {
        .defaultNote = 40,
        .pitchRandomization = 2.1f,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0,
        .decay = 0.2f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeTriangle,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 8.1f,
        .phase = 0
    },
    [SFX_FAN_OF_KNIVES] = {
        .defaultNote = 29,
        .pitchRandomization = 1.9f,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0.07f,
        .decay = 0.34f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothDown,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 5.8f,
        .phase = 0
    },
    [SFX_BOW_DRAW] = {
        .defaultNote = 52,
        .pitchRandomization = 1.0f,
        .volume = 0.05f,
        .waveform = kWaveformNoise,
        .attack = 0.5f,
        .decay = 0.48f,
        .sustain = 0,
        .release = 0,
        .curvature = 0.56f,
        .lfo = 0
    },
    [SFX_BOW_DRAW_BG] = {
        .defaultNote = 40,
        .pitchRandomization = 0.3f,
        .volume = 0.09f,
        .waveform = kWaveformSawtooth,
        .attack = 0.5f,
        .decay = 0.48f,
        .sustain = 0,
        .release = 0,
        .curvature = 0.56f,
        .lfo = 0
    },
    [SFX_BOW_SHOOT] = {
        .defaultNote = 31,
        .pitchRandomization = 1.0f,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0.05f,
        .decay = 0.57f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_NET] = {
        .defaultNote = 55,
        .pitchRandomization = 1.0f,
        .volume = 0.65f,
        .waveform = kWaveformTriangle,
        .attack = 0,
        .decay = 0.54f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeTriangle,
        .lfo = 1,
        .depth = 0.61f,
        .center = 0.26f,
        .rate = 4.0f,
        .phase = 0
    },
    [SFX_NET_BG] = {
        .defaultNote = 45,
        .pitchRandomization = 1.0f,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0,
        .decay = 0.54f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeTriangle,
        .lfo = 1,
        .depth = 0.61f,
        .center = 0.26f,
        .rate = 4.0f,
        .phase = 0
    },
    [SFX_DASH] = {
        .defaultNote = 40,
        .pitchRandomization = 1.9f,
        .volume = 0.12f,
        .waveform = kWaveformNoise,
        .attack = 0.16f,
        .decay = 0.35f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_DASH_BG] = {
        .defaultNote = 41,
        .volume = 0.33f,
        .waveform = kWaveformSquare,
        .attack = 0.04f,
        .decay = 0.25f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothDown,
        .lfo = 1,
        .depth = 1.01f,
        .center = 0.42f,
        .rate = 15.5f,
        .phase = 0
    },
    [SFX_BOMB] = {
        .defaultNote = 17,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0,
        .decay = 0.73f,
        .sustain = 0,
        .release = 0,
        .curvature = 0.51f,
        .lfoType = kLFOTypeTriangle,
        .lfo = 1,
        .depth = 0.45f,
        .center = 0.5f,
        .rate = 5.9f,
        .phase = 0
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
    [SFX_UI_NAVIGATE_TAB] = {
        .defaultNote = 58,
        .volume = 1.0f,
        .waveform = kWaveformTriangle,
        .attack = 0,
        .decay = 0.07f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_UI_NAVIGATE] = {
        .defaultNote = 64,
        .volume = 1.0f,
        .waveform = kWaveformTriangle,
        .attack = 0,
        .decay = 0.07f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_UI_ADJUST_UP] = {
        .defaultNote = 80,
        .volume = 0.6f,
        .waveform = kWaveformSine,
        .attack = 0,
        .decay = 0.05f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_UI_ADJUST_DOWN] = {
        .defaultNote = 76,
        .volume = 0.6f,
        .waveform = kWaveformSine,
        .attack = 0,
        .decay = 0.05f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_UI_ADJUST_NEUTRAL] = {
        .defaultNote = 78,
        .volume = 0.6f,
        .waveform = kWaveformSine,
        .attack = 0,
        .decay = 0.05f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_UI_RUNE_SLIDE] = {
        .defaultNote = 24,
        .volume = 0.08f,
        .waveform = kWaveformNoise,
        .attack = 0.1f,
        .decay = 0.41f,
        .sustain = 0,
        .release = 0.22f,
        .curvature = 0.9f,
        .lfo = 0
    },
    [SFX_UI_ERROR] = {
        .defaultNote = 59,
        .volume = 1.0f,
        .waveform = kWaveformTriangle,
        .attack = 0,
        .decay = 0.08f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_UI_CHECKBOX_CHECKED] = {
        .defaultNote = 60,
        .volume = 0.27f,
        .waveform = kWaveformNoise,
        .attack = 0,
        .decay = 0.13f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_UI_CHECKBOX_UNCHECKED] = {
        .defaultNote = 43,
        .volume = 0.27f,
        .waveform = kWaveformNoise,
        .attack = 0,
        .decay = 0.13f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_UI_CLICK] = {
        .defaultNote = 65,
        .volume = 1.0f,
        .waveform = kWaveformTriangle,
        .attack = 0,
        .decay = 0.12f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_UI_CLICK_NO] = {
        .defaultNote = 62,
        .volume = 1.0f,
        .waveform = kWaveformTriangle,
        .attack = 0,
        .decay = 0.12f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_SHOP_SELECT] = {
        .defaultNote = 28,
        .volume = 0.12f,
        .waveform = kWaveformNoise,
        .attack = 0.11f,
        .decay = 0.59f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 8.5f,
        .phase = 0
    },
    [SFX_SHOP_SLIDE_IN] = {
        .defaultNote = 29,
        .volume = 0.11f,
        .waveform = kWaveformNoise,
        .attack = 0.29f,
        .decay = 0.39f,
        .sustain = 0,
        .release = 0.08f,
        .curvature = 0.83f,
        .lfoType = kLFOTypeSawtoothUp,
        .lfo = 1,
        .depth = 0.44f,
        .center = 0.5f,
        .rate = 0.6f,
        .phase = 0
    },
    [SFX_SHOP_SLIDE_OUT] = {
        .defaultNote = 29,
        .volume = 0.12f,
        .waveform = kWaveformNoise,
        .attack = 0.29f,
        .decay = 0.39f,
        .sustain = 0,
        .release = 0.08f,
        .curvature = 0.83f,
        .lfoType = kLFOTypeSawtoothDown,
        .lfo = 1,
        .depth = 0.44f,
        .center = 0.5f,
        .rate = 0.6f,
        .phase = 0
    },
    [SFX_AREA_MAP_PLAYER_SLIDE] = {
        .defaultNote = 30,
        .volume = 0.08f,
        .waveform = kWaveformNoise,
        .attack = 0.31f,
        .decay = 0.73f,
        .sustain = 0,
        .release = 0.22f,
        .curvature = 0.9f,
        .lfo = 0
    },
    [SFX_AREA_MAP_PLAYER_SLIDE_BETWEEN] = {
        .defaultNote = 30,
        .volume = 0.08f,
        .waveform = kWaveformNoise,
        .attack = 0.42f,
        .decay = 1.06f,
        .sustain = 0,
        .release = 0.45f,
        .curvature = 0.9f,
        .lfo = 0
    },
    [SFX_AREA_MAP_AREA_SLIDE] = {
        .defaultNote = 29,
        .volume = 0.08f,
        .waveform = kWaveformSawtooth,
        .attack = 0.24f,
        .decay = 0.71f,
        .sustain = 0,
        .release = 0.22f,
        .curvature = 0.9f,
        .lfo = 0
    },
    [SFX_AREA_MAP_SELECT] = {
        .defaultNote = 62,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformTriangle,
        .attack = 0,
        .decay = 0.33f,
        .sustain = 0,
        .release = 0.02f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothUp,
        .lfo = 1,
        .depth = 0.52f,
        .center = 0,
        .rate = 8.1f,
        .phase = 0
    },
    [SFX_CLASS_SELECT] = {
        .defaultNote = 28,
        .volume = 0.12f,
        .waveform = kWaveformNoise,
        .attack = 0.11f,
        .decay = 0.59f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 8.5f,
        .phase = 0
    },
    [SFX_CLASS_BACK] = {
        .defaultNote = 19,
        .volume = 0.12f,
        .waveform = kWaveformNoise,
        .attack = 0.1f,
        .decay = 0.37f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_CLASS_ERROR] = {
        .defaultNote = 14,
        .volume = 0.12f,
        .waveform = kWaveformNoise,
        .attack = 0.04f,
        .decay = 0.3f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_SCENE_TRANSITION_OUT] = {
        .defaultNote = 11,
        .volume = 0.06f,
        .waveform = kWaveformSquare,
        .attack = 0.25f,
        .decay = 1.05f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothUp,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 0.1f,
        .phase = 0
    },
    [SFX_SCENE_TRANSITION_IN] = {
        .defaultNote = 12,
        .volume = 0.05f,
        .waveform = kWaveformSquare,
        .attack = 0.47f,
        .decay = 1.3f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothDown,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 0.1f,
        .phase = 0
    },
    [SFX_TUTORIAL_TYPEWRITER] = {
        .defaultNote = 57,
        .pitchRandomization = 1.2f,
        .volume = 0.75f,
        .waveform = kWaveformTriangle,
        .attack = 0,
        .decay = 0.07f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_TUTORIAL_ARROW_PICKUP] = {
        .defaultNote = 68,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformTriangle,
        .attack = 0,
        .decay = 0.3f,
        .sustain = 0,
        .release = 0.06f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothUp,
        .lfo = 1,
        .depth = 0.52f,
        .center = 0,
        .rate = 14.6f,
        .phase = 0
    },
    [SFX_TUTORIAL_PASS] = {
        .defaultNote = 66,
        .volume = 0.71f,
        .waveform = kWaveformSine,
        .attack = 0.08f,
        .decay = 0.67f,
        .sustain = 0,
        .release = 0.1f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.77f,
        .center = 0,
        .rate = 8.2f,
        .phase = 0
    },
    [SFX_TUTORIAL_CONFIRM] = {
        .defaultNote = 80,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformSine,
        .attack = 0,
        .decay = 0.11f,
        .sustain = 0,
        .release = 0.09f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothUp,
        .lfo = 1,
        .depth = 0.55f,
        .center = 0,
        .rate = 10.9f,
        .phase = 0
    },
    [SFX_BOSS_SHOUT_ELDER] = {
        .defaultNote = 21,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0.71f,
        .decay = 1.26f,
        .sustain = 0,
        .release = 0,
        .curvature = 0.85f,
        .lfo = 0
    },
    [SFX_BOSS_SHOUT_WRAITH] = {
        .defaultNote = 36,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0.73f,
        .decay = 1.38f,
        .sustain = 0,
        .release = 0,
        .curvature = 0.85f,
        .lfo = 0
    },
    [SFX_BOSS_SHOUT_DRAGON] = {
        .defaultNote = 16,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0.68f,
        .decay = 1.32f,
        .sustain = 0,
        .release = 0,
        .curvature = 0.85f,
        .lfo = 0
    },
    [SFX_CINEMATIC_SWEEP_IN] = {
        .defaultNote = 21,
        .volume = 0.13f,
        .waveform = kWaveformSquare,
        .attack = 0.85f,
        .decay = 0.98f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.02f,
        .lfo = 0
    },
    [SFX_CINEMATIC_SWEEP_OUT] = {
        .defaultNote = 45,
        .volume = 0.08f,
        .waveform = kWaveformNoise,
        .attack = 0.52f,
        .decay = 0.69f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothDown,
        .lfo = 1,
        .depth = 0.26f,
        .center = 0.44f,
        .rate = 0.2f,
        .phase = 0
    },
    [SFX_CINEMATIC_INTRO] = {
        .defaultNote = 8,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0.22f,
        .decay = 0.46f,
        .sustain = 0,
        .release = 0.03f,
        .curvature = 0.54f,
        .lfo = 0
    },
    [SFX_CINEMATIC_DEFEATED] = {
        .defaultNote = 21,
        .volume = 0.13f,
        .waveform = kWaveformSquare,
        .attack = 1.18f,
        .decay = 1.93f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.02f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.42f,
        .center = 0.44f,
        .rate = 6.7f,
        .phase = 0
    },
    [SFX_CINEMATIC_HEAL] = {
        .defaultNote = 42,
        .volume = 0.31f,
        .waveform = kWaveformSquare,
        .attack = 0.25f,
        .decay = 1.61f,
        .sustain = 0.01f,
        .release = 0,
        .curvature = 0.64f,
        .lfoType = kLFOTypeSawtoothUp,
        .lfo = 1,
        .depth = 0.54f,
        .center = 0,
        .rate = 6.3f,
        .phase = 0
    },
    [SFX_CINEMATIC_VICTORY] = {
        .defaultNote = 68,
        .volume = 0.27f,
        .waveform = kWaveformNoise,
        .attack = 0.2f,
        .decay = 1.08f,
        .sustain = 0,
        .release = 0.07f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.59f,
        .center = 0,
        .rate = 10.5f,
        .phase = 0
    },
    [SFX_BABY_SLIME_WALK] = {
        .defaultNote = 60,
        .pitchRandomization = 1.3f,
        .volume = 0.11f,
        .waveform = kWaveformSquare,
        .attack = 0,
        .decay = 0.24f,
        .sustain = 0,
        .release = 0.08f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothUp,
        .lfo = 1,
        .depth = 0.44f,
        .center = 0,
        .rate = 6.4f,
        .phase = 0
    },
    [SFX_BABY_SLIME_PREP] = {
        .defaultNote = 38,
        .volume = 0.19f,
        .waveform = kWaveformSquare,
        .attack = 0.07f,
        .decay = 0.22f,
        .sustain = 0,
        .release = 0.1f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.64f,
        .center = 0,
        .rate = 11.3f,
        .phase = 0
    },
    [SFX_BABY_SLIME_ATTACK] = {
        .defaultNote = 38,
        .volume = 0.25f,
        .waveform = kWaveformNoise,
        .attack = 0.05f,
        .decay = 0.22f,
        .sustain = 0,
        .release = 0.1f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.64f,
        .center = 0,
        .rate = 11.3f,
        .phase = 0
    },
    [SFX_SLIME_WALK] = {
        .defaultNote = 44,
        .pitchRandomization = 1.3f,
        .volume = 0.11f,
        .waveform = kWaveformSquare,
        .attack = 0,
        .decay = 0.24f,
        .sustain = 0,
        .release = 0.08f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothUp,
        .lfo = 1,
        .depth = 0.44f,
        .center = 0,
        .rate = 6.4f,
        .phase = 0
    },
    [SFX_SLIME_PREP] = {
        .defaultNote = 33,
        .volume = 0.19f,
        .waveform = kWaveformSquare,
        .attack = 0.07f,
        .decay = 0.22f,
        .sustain = 0,
        .release = 0.1f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.64f,
        .center = 0,
        .rate = 11.3f,
        .phase = 0
    },
    [SFX_SLIME_ATTACK] = {
        .defaultNote = 31,
        .volume = 0.25f,
        .waveform = kWaveformNoise,
        .attack = 0.05f,
        .decay = 0.22f,
        .sustain = 0,
        .release = 0.1f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.64f,
        .center = 0,
        .rate = 11.3f,
        .phase = 0
    },
    [SFX_BAT_FLAP] = {
        .defaultNote = 45,
        .pitchRandomization = 1.2f,
        .volume = 0.15f,
        .waveform = kWaveformNoise,
        .attack = 0.1f,
        .decay = 0.32f,
        .sustain = 0,
        .release = 0.09f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.34f,
        .center = 0.5f,
        .rate = 2.8f,
        .phase = 0
    },
    [SFX_BAT_PREP] = {
        .defaultNote = 32,
        .volume = 0.29f,
        .waveform = kWaveformNoise,
        .attack = 0.07f,
        .decay = 0.35f,
        .sustain = 0,
        .release = 0.13f,
        .curvature = 0.64f,
        .lfoType = kLFOTypeTriangle,
        .lfo = 1,
        .depth = 0.85f,
        .center = 0.5f,
        .rate = 9.4f,
        .phase = 0
    },
    [SFX_BAT_ATTACK] = {
        .defaultNote = 32,
        .volume = 0.33f,
        .waveform = kWaveformNoise,
        .attack = 0.26f,
        .decay = 0.35f,
        .sustain = 0,
        .release = 0.13f,
        .curvature = 0.64f,
        .lfoType = kLFOTypeTriangle,
        .lfo = 1,
        .depth = 0.85f,
        .center = 0.5f,
        .rate = 9.4f,
        .phase = 0
    },
    [SFX_SKELETON_WALK] = {
        .defaultNote = 35,
        .pitchRandomization = 3.4f,
        .volume = 0.44f,
        .waveform = kWaveformNoise,
        .attack = 0.03f,
        .decay = 0.15f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 16.2f,
        .phase = 0
    },
    [SFX_SKELETON_PREP] = {
        .defaultNote = 38,
        .pitchRandomization = 0.7f,
        .volume = 0.33f,
        .waveform = kWaveformNoise,
        .attack = 0.11f,
        .decay = 0.23f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 16.2f,
        .phase = 0
    },
    [SFX_SKELETON_ATTACK] = {
        .defaultNote = 40,
        .pitchRandomization = 0.2f,
        .volume = 0.52f,
        .waveform = kWaveformNoise,
        .attack = 0.02f,
        .decay = 0.31f,
        .sustain = 0,
        .release = 0,
        .curvature = 0,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 5.0f,
        .phase = 0
    },
    [SFX_SERPENT_SLITHER] = {
        .defaultNote = 54,
        .pitchRandomization = 1.5f,
        .volume = 0.11f,
        .waveform = kWaveformNoise,
        .attack = 0.1f,
        .decay = 0.3f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_SERPENT_PREP] = {
        .defaultNote = 41,
        .volume = 0.18f,
        .waveform = kWaveformSquare,
        .attack = 0.13f,
        .decay = 0.3f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 5.0f,
        .phase = 0
    },
    [SFX_SERPENT_ATTACK] = {
        .defaultNote = 22,
        .volume = 0.29f,
        .waveform = kWaveformNoise,
        .attack = 0.04f,
        .decay = 0.23f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_SERPENT_TAIL] = {
        .defaultNote = 40,
        .pitchRandomization = 1.2f,
        .volume = 0.3f,
        .waveform = kWaveformNoise,
        .attack = 0.09f,
        .decay = 0.3f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_RAT_JUMP] = {
        .defaultNote = 60,
        .pitchRandomization = 1.3f,
        .volume = 0.11f,
        .waveform = kWaveformSquare,
        .attack = 0.04f,
        .decay = 0.22f,
        .sustain = 0,
        .release = 0.08f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothUp,
        .lfo = 1,
        .depth = 0.44f,
        .center = 0,
        .rate = 6.4f,
        .phase = 0
    },
    [SFX_RAT_PREP] = {
        .defaultNote = 55,
        .volume = 0.39f,
        .waveform = kWaveformSawtooth,
        .attack = 0,
        .decay = 0.23f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 7.2f,
        .phase = 0
    },
    [SFX_RAT_ATTACK] = {
        .defaultNote = 51,
        .pitchRandomization = 1.0f,
        .volume = 0.39f,
        .waveform = kWaveformNoise,
        .attack = 0,
        .decay = 0.23f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 7.2f,
        .phase = 0
    },
    [SFX_IMP_WALK] = {
        .defaultNote = 42,
        .pitchRandomization = 1.0f,
        .volume = 0.39f,
        .waveform = kWaveformNoise,
        .attack = 0,
        .decay = 0.15f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 7.2f,
        .phase = 0
    },
    [SFX_IMP_PREP] = {
        .defaultNote = 42,
        .pitchRandomization = 1.0f,
        .volume = 0.39f,
        .waveform = kWaveformPOPhase,
        .attack = 0.07f,
        .decay = 0.24f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 7.2f,
        .phase = 0
    },
    [SFX_IMP_ATTACK] = {
        .defaultNote = 52,
        .pitchRandomization = 0.8f,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0.07f,
        .decay = 0.21f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfo = 0
    },
    [SFX_FLYING_TOME_FLAP] = {
        .defaultNote = 38,
        .pitchRandomization = 1.2f,
        .volume = 0.15f,
        .waveform = kWaveformNoise,
        .attack = 0.1f,
        .decay = 0.32f,
        .sustain = 0,
        .release = 0.09f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.34f,
        .center = 0.5f,
        .rate = 2.8f,
        .phase = 0
    },
    [SFX_FLYING_TOME_PREP] = {
        .defaultNote = 41,
        .pitchRandomization = 0.8f,
        .volume = 0.14f,
        .waveform = kWaveformSquare,
        .attack = 0.1f,
        .decay = 0.32f,
        .sustain = 0,
        .release = 0.09f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.34f,
        .center = 0.5f,
        .rate = 8.3f,
        .phase = 0
    },
    [SFX_FLYING_TOME_ATTACK] = {
        .defaultNote = 45,
        .pitchRandomization = 0.8f,
        .volume = 0.3f,
        .waveform = kWaveformSquare,
        .attack = 0,
        .decay = 0.29f,
        .sustain = 0,
        .release = 0.09f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.34f,
        .center = 0.5f,
        .rate = 11.6f,
        .phase = 0
    },
    [SFX_LIVING_ARMOR_WALK] = {
        .defaultNote = 28,
        .pitchRandomization = 0.9f,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0,
        .decay = 0.37f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 5.0f,
        .phase = 0
    },
    [SFX_LIVING_ARMOR_PREP] = {
        .defaultNote = 35,
        .pitchRandomization = 0.8f,
        .volume = 0.75f,
        .waveform = kWaveformSawtooth,
        .attack = 0.13f,
        .decay = 0.7f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothUp,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 7.8f,
        .phase = 0
    },
    [SFX_LIVING_ARMOR_ATTACK] = {
        .defaultNote = 30,
        .pitchRandomization = 0.2f,
        .volume = 0.54f,
        .waveform = kWaveformNoise,
        .attack = 0,
        .decay = 0.31f,
        .sustain = 0,
        .release = 0,
        .curvature = 0,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 4.4f,
        .phase = 0
    },
    [SFX_BOMBER_WALK] = {
        .defaultNote = 40,
        .pitchRandomization = 0.8f,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformSquare,
        .attack = 0,
        .decay = 0.3f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSawtoothUp,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 5.0f,
        .phase = 0
    },
    [SFX_BOMBER_ATTACK] = {
        .defaultNote = 58,
        .pitchRandomization = 1.4f,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformTriangle,
        .attack = 0,
        .decay = 0.3f,
        .sustain = 0,
        .release = 0,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.89f,
        .center = 0.45f,
        .rate = 9.7f,
        .phase = 0
    },
    [SFX_MUMMY_WALK] = {
        .defaultNote = 44,
        .pitchRandomization = 1.3f,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformNoise,
        .attack = 0.07f,
        .decay = 0.15f,
        .sustain = 0,
        .release = 0,
        .curvature = 0.9f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.94f,
        .center = 0.04f,
        .rate = 5.3f,
        .phase = 0.38f
    },
    [SFX_MUMMY_PREP] = {
        .defaultNote = 53,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformPOVosim,
        .attack = 0.27f,
        .decay = 0.56f,
        .sustain = 0,
        .release = 0.07f,
        .curvature = 0.72f,
        .lfo = 0
    },
    [SFX_MUMMY_ATTACK] = {
        .defaultNote = 47,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformSine,
        .attack = 0.16f,
        .decay = 0.46f,
        .sustain = 0,
        .release = 0,
        .curvature = 0.94f,
        .lfoType = kLFOTypeSquare,
        .lfo = 1,
        .depth = 0.49f,
        .center = 0.29f,
        .rate = 9.1f,
        .phase = 0
    },
    [SFX_WRAITH_IDLE] = {
        .defaultNote = 44,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformTriangle,
        .attack = 0.08f,
        .decay = 0.38f,
        .sustain = 0,
        .release = 0.32f,
        .curvature = 0.86f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.97f,
        .center = 0.04f,
        .rate = 15.3f,
        .phase = 0.55f
    },
    [SFX_BABY_DRAGON_FLAP] = {
        .defaultNote = 35,
        .pitchRandomization = 1.2f,
        .volume = 0.15f,
        .waveform = kWaveformNoise,
        .attack = 0.1f,
        .decay = 0.32f,
        .sustain = 0,
        .release = 0.09f,
        .curvature = 1.0f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.34f,
        .center = 0.5f,
        .rate = 2.8f,
        .phase = 0
    },
    [SFX_BABY_DRAGON_PREP] = {
        .defaultNote = 64,
        .volume = 0.56f,
        .waveform = kWaveformPOVosim,
        .attack = 0.09f,
        .decay = 0.44f,
        .sustain = 0,
        .release = 0,
        .curvature = 0,
        .lfoType = kLFOTypeSawtoothUp,
        .lfo = 1,
        .depth = 0.76f,
        .center = -0.17f,
        .rate = 10.4f,
        .phase = 0.59f
    },
    [SFX_BABY_DRAGON_ATTACK] = {
        .defaultNote = 37,
        .pitchRandomization = 4.3f,
        .volume = 0.33f,
        .waveform = kWaveformNoise,
        .attack = 0.06f,
        .decay = 0.39f,
        .sustain = 0,
        .release = 0.09f,
        .curvature = 0.52f,
        .lfo = 0
    },
    [SFX_AIR_ELEMENTAL_IDLE] = {
        .defaultNote = 40,
        .pitchRandomization = 1.4f,
        .volume = 0.1f,
        .waveform = kWaveformNoise,
        .attack = 0.24f,
        .decay = 0.31f,
        .sustain = 0,
        .release = 0,
        .curvature = 0,
        .lfo = 0
    },
    [SFX_AIR_ELEMENTAL_PREP] = {
        .defaultNote = 40,
        .pitchRandomization = 0.7f,
        .volume = 0.1f,
        .waveform = kWaveformNoise,
        .attack = 0.24f,
        .decay = 0.31f,
        .sustain = 0,
        .release = 0,
        .curvature = 0,
        .lfoType = kLFOTypeTriangle,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 1.0f,
        .phase = 0
    },
    [SFX_AIR_ELEMENTAL_ATTACK] = {
        .defaultNote = 40,
        .pitchRandomization = 0.7f,
        .volume = 0.1f,
        .waveform = kWaveformNoise,
        .attack = 0.24f,
        .decay = 0.31f,
        .sustain = 0,
        .release = 0,
        .curvature = 0,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.5f,
        .center = 0.5f,
        .rate = 7.6f,
        .phase = 0
    },
    [SFX_WRAITH_BOSS_TELEPORT] = {
        .defaultNote = 59,
        .volume = DEFAULT_VOLUME,
        .waveform = kWaveformPOPhase,
        .attack = 0.28f,
        .decay = 0.56f,
        .sustain = 0,
        .release = 0,
        .curvature = 0.82f,
        .lfoType = kLFOTypeSine,
        .lfo = 1,
        .depth = 0.81f,
        .center = -0.29f,
        .rate = 11.2f,
        .phase = 0.87f
    }
};

static PDSynth *synthVoices[SFX_COUNT][MAX_SFX_VOICES];
static int synthVoiceCount[SFX_COUNT];
static int nextVoice[SFX_COUNT];

static PDSynth *buildSynth(SFXData data) {
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

    return synth;
}

void createSynth(SFX sfx) {
    SFXData data = sfxData[sfx];
    int count = data.voices > 0 ? data.voices : DEFAULT_SFX_VOICES;
    if (count > MAX_SFX_VOICES) {
        count = MAX_SFX_VOICES;
    }
    synthVoiceCount[sfx] = count;
    for (int i = 0; i < count; i++) {
        synthVoices[sfx][i] = buildSynth(data);
    }
}

// Round-robins across an SFX's voice pool so overlapping plays land on different
// synths instead of stealing each other. Single-voice SFX always return voice 0.
static PDSynth *nextSynthVoice(SFX sfx) {
    int count = synthVoiceCount[sfx];
    if (count <= 1) {
        return synthVoices[sfx][0];
    }
    int index = nextVoice[sfx];
    nextVoice[sfx] = (index + 1) % count;
    return synthVoices[sfx][index];
}

static float computeNote(SFXData data) {
    float note = (float)data.defaultNote;
    if (data.pitchRandomization > 0) {
        note += randFloat(-data.pitchRandomization, data.pitchRandomization);
    }
    return note;
}

// Scaffolding for future audio file loading. Populate these alongside SFX_FILE_COUNT
// and MUSIC_COUNT in audioManager.h, then dispatch in loadAudioAsset below.
// static const char *sfxFilePathStrings[SFX_FILE_COUNT] = { };
// static AudioSample *sfxFiles[SFX_FILE_COUNT];

static const char *musicPathStrings[MUSIC_COUNT] = {
    [MUSIC_TITLE] = "audio/music/Title",
    [MUSIC_TUTORIAL] = "audio/music/Tutorial",
    [MUSIC_RUNIC_ROGUES_1] = "audio/music/TwilightCourt",
    [MUSIC_RUNIC_ROGUES_2] = "audio/music/LostArchives",
    [MUSIC_RUNIC_ROGUES_3] = "audio/music/EchoingCaverns",
};
static FilePlayer *musicFiles[MUSIC_COUNT];

#define MUSIC_LOWPASS_FREQUENCY 500.0f
#define MUSIC_LOWPASS_RESONANCE 0.2f
static SoundChannel *musicChannel = NULL;
static TwoPoleFilter *musicLowpass = NULL;

static void initMusicChannel(void) {
    musicChannel = pd->sound->channel->newChannel();
    pd->sound->addChannel(musicChannel);

    musicLowpass = pd->sound->effect->twopolefilter->newFilter();
    pd->sound->effect->twopolefilter->setType(musicLowpass, kFilterTypeLowPass);
    pd->sound->effect->twopolefilter->setFrequency(musicLowpass, MUSIC_LOWPASS_FREQUENCY);
    pd->sound->effect->twopolefilter->setResonance(musicLowpass, MUSIC_LOWPASS_RESONANCE);
    pd->sound->effect->setMix((SoundEffect *)musicLowpass, 0.0f);
    pd->sound->channel->addEffect(musicChannel, (SoundEffect *)musicLowpass);
}

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
        if (musicChannel == NULL) {
            initMusicChannel();
        }
        musicFiles[step] = pd->sound->fileplayer->newPlayer();
        pd->sound->fileplayer->loadIntoPlayer(musicFiles[step], musicPathStrings[step]);
        pd->sound->channel->addSource(musicChannel, (SoundSource *)musicFiles[step]);
        return;
    }
}

void playSound(SFX sfx) {
    if (sfx < 0 || sfx >= SFX_COUNT) return;

    float velocity = getSfxVolumeSetting();
    if (velocity <= 0.0f) {
        return;
    }

    SFXData data = sfxData[sfx];
    PDSynth *voice = nextSynthVoice(sfx);
    // Reset to centred per-SFX volume in case this voice was last used spatially.
    pd->sound->synth->setVolume(voice, data.volume, data.volume);
    pd->sound->synth->playMIDINote(voice, computeNote(data), velocity, DEFAULT_PLAY_LEN, 0);
}

// Plays an SFX positioned in the world: volume falls off with distance to the player and
// the note is panned left/right based on which side of the camera the source sits. Sounds
// beyond SFX_FALLOFF_MAX are culled (not played at all).
void playSoundSpatial(SFX sfx, float sourceX, float sourceY) {
    if (sfx < 0 || sfx >= SFX_COUNT) return;

    float velocity = getSfxVolumeSetting();
    if (velocity <= 0.0f) {
        return;
    }

    float dx = sourceX - playerX;
    float dy = sourceY - playerY;
    float distSq = dx * dx + dy * dy;
    if (distSq > SFX_FALLOFF_MAX * SFX_FALLOFF_MAX) {
        return;
    }
    float dist = fastSqrt(distSq);

    float attenuation = 1.0f;
    if (dist > SFX_FALLOFF_MIN) {
        attenuation = 1.0f - (dist - SFX_FALLOFF_MIN) / (SFX_FALLOFF_MAX - SFX_FALLOFF_MIN);
        attenuation *= attenuation;  // steeper, more natural rolloff
    }

    // The camera plane points to screen-left, so projecting the source direction onto it
    // and negating gives a pan in [-1, 1] (negative == left, positive == right).
    float pan = 0.0f;
    if (dist > 0.01f) {
        float planeLen = fastSqrt(planeX * planeX + planeY * planeY);
        if (planeLen > 0.0f) {
            pan = -(dx * planeX + dy * planeY) / (planeLen * dist);
            pan = clampFloat(pan, -1.0f, 1.0f);
        }
    }
    float rightPan = pan > 0.0f ? pan : 0.0f;
    float leftPan = pan < 0.0f ? -pan : 0.0f;
    float leftGain = 1.0f - rightPan * SFX_PAN_STRENGTH;
    float rightGain = 1.0f - leftPan * SFX_PAN_STRENGTH;

    SFXData data = sfxData[sfx];
    float channelVolume = data.volume * attenuation;
    PDSynth *voice = nextSynthVoice(sfx);
    pd->sound->synth->setVolume(voice, channelVolume * leftGain, channelVolume * rightGain);
    pd->sound->synth->playMIDINote(voice, computeNote(data), velocity, DEFAULT_PLAY_LEN, 0);
}

// === Music manager ===
#define MUSIC_FADE_TIME 1.5f
#define MUSIC_LOWPASS_FADE_TIME 0.35f
#define MUSIC_PAUSE_FADE_TIME 0.75f

static float musicFade[MUSIC_COUNT];
static float musicTargetFade[MUSIC_COUNT];
static int currentSong = -1;

static float filterMix;
static float filterMixTarget;

static bool musicPaused = false;
static bool musicPauseFade = false;   // whether the active pause ramps or snaps
static bool musicRateFrozen = false;  // playback rate held at 0 to freeze position
static float musicChannelGain = 1.0f;
static float musicChannelGainTarget = 1.0f;

void setMusicLowpassEnabled(bool enabled) {
    filterMixTarget = enabled ? 1.0f : 0.0f;
}

void resetMusicManager(void) {
    musicPaused = false;
    musicPauseFade = false;
    musicRateFrozen = false;
    musicChannelGain = 1.0f;
    musicChannelGainTarget = 1.0f;
    filterMix = 0.0f;
    filterMixTarget = 0.0f;
    if (musicChannel != NULL) {
        pd->sound->channel->setVolume(musicChannel, 1.0f);
    }
    for (int i = 0; i < MUSIC_COUNT; i++) {
        if (musicFiles[i] != NULL) {
            pd->sound->fileplayer->setRate(musicFiles[i], 1.0f);
        }
    }
    if (musicLowpass != NULL) {
        pd->sound->effect->setMix((SoundEffect *)musicLowpass, 0.0f);
    }
}

// Pausing mutes the music channel and freezes playback rate rather than pausing the
// file players: fileplayer->play() re-adds the source to the default channel, which
// would bypass the lowpass effect on musicChannel. Muting keeps the routing intact.
// The channel gain ramps toward its target in updateMusicChannelGain; snapping the
// gain here makes the pause instant, otherwise it fades. Resume reuses the fade mode
// the pause was requested with.
static void requestMusicPause(bool paused, bool fade) {
    if (paused == musicPaused) {
        return;
    }
    musicPaused = paused;
    if (paused) {
        musicPauseFade = fade;
    }
    musicChannelGainTarget = paused ? 0.0f : 1.0f;
    if (!musicPauseFade) {
        musicChannelGain = musicChannelGainTarget;
    }
}

void setMusicPaused(bool paused) {
    requestMusicPause(paused, false);
}

void setMusicPausedWithFade(bool paused) {
    requestMusicPause(paused, true);
}

void playSong(Music song) {
    if (song < 0 || song >= MUSIC_COUNT) return;
    if (currentSong == (int)song) return;  // already the active track

    currentSong = (int)song;
    for (int i = 0; i < MUSIC_COUNT; i++) {
        musicTargetFade[i] = 0.0f;
    }
    musicTargetFade[song] = 1.0f;

    // A track fading back in keeps its current fade level; only start fresh
    // (from silence) if it isn't already playing.
    if (musicFiles[song] != NULL && !pd->sound->fileplayer->isPlaying(musicFiles[song])) {
        musicFade[song] = 0.0f;
        pd->sound->fileplayer->setVolume(musicFiles[song], 0.0f, 0.0f);
        pd->sound->fileplayer->play(musicFiles[song], 0);  // 0 == loop endlessly

        // play() re-adds the source to the default channel, which would bypass the
        // pause fade and lowpass living on musicChannel. Detach and re-route it back.
        pd->sound->removeSource((SoundSource *)musicFiles[song]);
        pd->sound->channel->addSource(musicChannel, (SoundSource *)musicFiles[song]);
    }
}

Music getCurrentSong(void) {
    return (Music)currentSong;
}

void stopSong(void) {
    currentSong = -1;
    for (int i = 0; i < MUSIC_COUNT; i++) {
        musicTargetFade[i] = 0.0f;
    }
}

static void updateMusicLowpass(float deltaTime) {
    if (musicLowpass == NULL) {
        return;
    }
    float step = (MUSIC_LOWPASS_FADE_TIME > 0.0f) ? (deltaTime / MUSIC_LOWPASS_FADE_TIME) : 1.0f;
    if (filterMix < filterMixTarget) {
        filterMix += step;
        if (filterMix > filterMixTarget) {
            filterMix = filterMixTarget;
        }
    } else if (filterMix > filterMixTarget) {
        filterMix -= step;
        if (filterMix < filterMixTarget) {
            filterMix = filterMixTarget;
        }
    }
    pd->sound->effect->setMix((SoundEffect *)musicLowpass, filterMix);
}

static void updateMusicChannelGain(float deltaTime) {
    if (musicChannel == NULL) {
        return;
    }
    float step = (MUSIC_PAUSE_FADE_TIME > 0.0f) ? (deltaTime / MUSIC_PAUSE_FADE_TIME) : 1.0f;
    if (musicChannelGain < musicChannelGainTarget) {
        musicChannelGain += step;
        if (musicChannelGain > musicChannelGainTarget) {
            musicChannelGain = musicChannelGainTarget;
        }
    } else if (musicChannelGain > musicChannelGainTarget) {
        musicChannelGain -= step;
        if (musicChannelGain < musicChannelGainTarget) {
            musicChannelGain = musicChannelGainTarget;
        }
    }
    pd->sound->channel->setVolume(musicChannel, musicChannelGain);

    // Freeze the playback position only once the fade-out has fully silenced it, and
    // unfreeze the instant a resume begins so it advances as it fades back in.
    bool shouldFreeze = musicPaused && musicChannelGain <= 0.0f;
    if (shouldFreeze != musicRateFrozen) {
        musicRateFrozen = shouldFreeze;
        for (int i = 0; i < MUSIC_COUNT; i++) {
            if (musicFiles[i] != NULL) {
                pd->sound->fileplayer->setRate(musicFiles[i], shouldFreeze ? 0.0f : 1.0f);
            }
        }
    }
}

void updateMusicManager(float deltaTime) {
    updateMusicLowpass(deltaTime);
    updateMusicChannelGain(deltaTime);

    if (musicPaused && musicChannelGain <= 0.0f) {
        return;
    }

    float step = (MUSIC_FADE_TIME > 0.0f) ? (deltaTime / MUSIC_FADE_TIME) : 1.0f;

    for (int i = 0; i < MUSIC_COUNT; i++) {
        if (musicFiles[i] == NULL) continue;

        float target = musicTargetFade[i];
        if (musicFade[i] < target) {
            musicFade[i] += step;
            if (musicFade[i] > target) musicFade[i] = target;
        } else if (musicFade[i] > target) {
            musicFade[i] -= step;
            if (musicFade[i] < target) musicFade[i] = target;
        }

        if (pd->sound->fileplayer->isPlaying(musicFiles[i])) {
            float volume = musicFade[i] * getMusicVolumeSetting();
            pd->sound->fileplayer->setVolume(musicFiles[i], volume, volume);
            if (musicFade[i] <= 0.0f && target <= 0.0f) {
                pd->sound->fileplayer->stop(musicFiles[i]);
            }
        }
    }
}