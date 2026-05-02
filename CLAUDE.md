# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A 3D raycasting dungeon crawler for the [Playdate](https://play.date/) handheld console. Written in C using the Playdate C SDK.

## Build Commands

Requires the Playdate SDK installed with `PLAYDATE_SDK_PATH` environment variable set.

```bash
# Build simulator (existing configured dir)
cmake --build build/build-sim

# Build device (existing configured dir)
cmake --build build/build-device

# Output: Game.pdx (runnable in Playdate Simulator)
```

To reconfigure from scratch (uses presets defined in `CMakePresets.json`):
```bash
# Simulator (Ninja + clang, Debug)
cmake --preset sim

# Device (Ninja + ARM toolchain, Release)
cmake --preset device
```

The simulator build compiles as a shared library with `-Wall -Werror`. The device build compiles as an ARM executable with `-O2 -ffast-math`.

## Architecture

### Scene System

`src/globals/sceneManager.h/c` — Central dispatcher. `updateSceneManager()` is the Playdate update callback registered in `main.c`. Manages 4 scenes (`TITLE`, `GAME`, `ROOM_MAP`, `GAME_OVER`) with animated circular wipe transitions.

### Game Loop (40 FPS)

`src/game/gameScene.c` orchestrates all subsystems each frame in this order:
1. Pickups → Spells → Player → Enemies → Projectiles → Particles → Interactables
2. Wave spawning, Timers
3. Raycasting render pass
4. UI overlay draw

### Rendering

`src/game/rendering/raycasting.h/c` — Custom raycasting engine. 66° FOV, 200×120 logical resolution (displayed at 2× = 400×240 on device). 16-level fog system from distance 60–150 units. Map is a 21×21 grid with 16-unit cells. Wall/floor/ceiling textures are area-based (Echoing Caverns, Lost Archives, Test).

### Spell System

`src/game/player/spells/` — Player has two equipped spell slots (basic/special). `spellData.c` defines all spell types. Runes can dynamically modify spell properties at runtime via the rune modifier framework in `runeManager.h/c`.

### Enemy System

`src/game/enemies/enemies.h/c` — 15+ enemy types, max 50 per room. Each enemy uses a state machine (`IDLE → PREP → ATTACK_1/2/3 → DEAD`) with cooldowns, knockback, slow effects, and hit-flash tracking.

### Progression & Rune System

- `progressionManager.h/c` — Tracks room progress, room types (`ENEMY`, `ELITE`, `CAMPFIRE`, `BOSS`), three areas.
- `runeManager.h/c` — Up to 14 runes per run. Rune types include `VAMPIRISM`, `WINGED_BOOTS`, `HEALTHY`, `SWIFT`. Each rune registers modifier callbacks that alter spell properties.
- `shopManager.h/c` — Animated popup shop triggered on room clear (all enemies defeated), supports English and Japanese localization.

### Key Utilities

`src/globals/helpers.h/c` — Fixed-size Queue (30), Stack (30, with shuffle), Set (53-slot hash). Math: `fastSqrt()`, `fastRsqrt()` (Quake-style), easing functions (`easeOutCubic`, `easeOutBack`), `lerp()`.

## Code Style

- Always use braces `{}` for if/else bodies, even single-line ones — no braceless one-liners
- Avoid verbose comments and comments on code that is self documenting

## Key Constants (globals.h)

| Constant | Value |
|---|---|
| Screen logical size | 200×120 |
| Display scale | 2× |
| Target FPS | 40 |
| FOV | 66° |
| Map size | 21×21 |
| Cell size | 16 units |
| Max enemies | 50 |
| Max runes | 14 |
