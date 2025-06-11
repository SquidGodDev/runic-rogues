local pd = playdate
local sound = pd.sound

local DEFAULT_PLAY_DURATION <const> = 3.0

SFX = {
    CursorNavigate = 1,
    CursorConfirm = 2,
    RoomPopulate = 3,
    BossRoomPopulate = 4,
}

local synths = {}
local defaultNotes = {
    [SFX.CursorNavigate] = 51,
    [SFX.CursorConfirm] = 56,
    [SFX.RoomPopulate] = 60,
    [SFX.BossRoomPopulate] = 30,
}

AudioManager = {}

local function createSynth(sfx, waveform, attack, delay, sustain, release, curvature)
    local synth = sound.synth.new(waveform)
    synth:setADSR(attack, delay, sustain, release, curvature)
    synths[sfx] = synth
    return synth
end

function AudioManager.init()
    -- Attack, Decay, Sustain, Release, Curvature
    createSynth(SFX.CursorNavigate, sound.kWaveTriangle, 0.0, 0.1, 0.0, 0.0, 1.0)

    createSynth(SFX.CursorConfirm, sound.kWaveNoise, 0.0, 1.0, 0.0, 0.0, 1.0)

    createSynth(SFX.RoomPopulate, sound.kWaveSawtooth, 0, 0.1, 0, 0.4, 0.6)

    createSynth(SFX.BossRoomPopulate, sound.kWaveNoise, 0.0, 1.0, 0.0, 0.0, 1.0)
end

function AudioManager.playSFX(sfx, midiNote)
    if not midiNote then
        midiNote = defaultNotes[sfx]
    end
    synths[sfx]:playMIDINote(midiNote, 1.0, DEFAULT_PLAY_DURATION)
end