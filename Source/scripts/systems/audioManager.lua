local pd = playdate
local sound = pd.sound

local DEFAULT_PLAY_DURATION <const> = 3.0

SFX = {
    CursorNavigate = 1,
    CursorConfirm = 2,
    CursorSlide = 3,
    RoomPopulate = 4,
    BossRoomPopulate = 5,
    TransitionStart = 6,
    TransitionEnd = 7
}

local synths = {}
local defaultNote = {}
local volume = {}

AudioManager = {}

local function createSynth(sfx, data)
    local synth = sound.synth.new(data.waveform)
    synth:setADSR(data.attack, data.decay, data.sustain, data.release, data.curvature)
    synths[sfx] = synth
    defaultNote[sfx] = data.defaultNote
    volume[sfx] = data.volume
    return synth
end

function AudioManager.init()
    createSynth(SFX.CursorNavigate, {
        defaultNote = 27,
        volume = 1.0,
        waveform = sound.kWaveSquare,
        attack = 0.0,
        decay = 0.3,
        sustain = 0.0,
        release = 0.0,
        curvature = 1.0
    })

    createSynth(SFX.CursorConfirm, {
        defaultNote = 56,
        volume = 1.0,
        waveform = sound.kWaveNoise,
        attack = 0.0,
        decay = 0.2,
        sustain = 0.0,
        release = 0.0,
        curvature = 1.0
    })

    createSynth(SFX.CursorSlide, {
        defaultNote = 52,
        volume = 1.0,
        waveform = sound.kWaveNoise,
        attack = 0.2,
        decay = 1.2,
        sustain = 0.0,
        release = 0.0,
        curvature = 1.0
    })

    createSynth(SFX.RoomPopulate, {
        defaultNote = 60,
        volume = 1.0,
        waveform = sound.kWaveSawtooth,
        attack = 0.0,
        decay = 0.1,
        sustain = 0.0,
        release = 0.4,
        curvature = 0.6
    })

    createSynth(SFX.BossRoomPopulate, {
        defaultNote = 30,
        volume = 1.0,
        waveform = sound.kWaveNoise,
        attack = 0.0,
        decay = 1.0,
        sustain = 0.0,
        release = 0.0,
        curvature = 1.0
    })

    createSynth(SFX.TransitionStart, {
        defaultNote = 67,
        volume = 1.0,
        waveform = sound.kWaveNoise,
        attack = 0.15,
        decay = 0.6,
        sustain = 0.0,
        release = 0.0,
        curvature = 1.0
    })

    createSynth(SFX.TransitionEnd, {
        defaultNote = 77,
        volume = 1.0,
        waveform = sound.kWaveNoise,
        attack = 0.2,
        decay = 0.8,
        sustain = 0.0,
        release = 0.0,
        curvature = 1.0
    })
end

function AudioManager.playSFX(sfx, midiNote)
    if not midiNote then
        midiNote = defaultNote[sfx]
    end
    synths[sfx]:playMIDINote(midiNote, volume[sfx], DEFAULT_PLAY_DURATION)
end