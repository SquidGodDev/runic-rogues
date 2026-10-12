# Runic Rogues
Source code for my Playdate game "Runic Rogues", a 3D fantasy spellcasting roguelike built on a custom raycasting engine written in C. Pick one of four classes (Mage, Rogue, Knight, or Ranger) and fight your way through room after room of enemies across three areas: the Twilight Court, the Lost Archives, and the Echoing Caverns. Select runes that give you upgrades, create synergistic builds, and fight against 3 different bullet hell boss battles. Fully localized in English and Japanese. You can see the game in action in the [trailer](https://youtu.be/Iw3_OlA3yOk) and get the game on [Catalog](https://play.date/games/runic-rogues/).

![1](https://github.com/user-attachments/assets/2dc46ef1-3286-4dc7-bf92-5f997858a520")
<img width="400" height="240" alt="DragonBossWarrior_hnyJyB4-ezgif com-optimize-gray" src="https://github.com/user-attachments/assets/8f99de48-f347-4dda-a62d-27b348c341af" />
<img width="400" height="240" alt="UpgradeSelect_AfYBBPf-ezgif com-optimize-gray" src="https://github.com/user-attachments/assets/d68a4276-f592-46ea-a614-ccc45f1afd31" />
<img width="400" height="240" alt="ClassSelect1-ezgif com-optimize-gray" src="https://github.com/user-attachments/assets/9ac3bf2e-b67e-4b9f-8069-95f11da7b2f4" />

## Building
Requires the [Playdate SDK](https://play.date/dev/) with the `PLAYDATE_SDK_PATH` environment variable set, plus CMake and Ninja.

```bash
# Simulator (Debug)
cmake --preset sim
cmake --build build/build-sim

# Device (Release)
cmake --preset device
cmake --build build/build-device
```

VSCode tasks can be used to run the build process as well (ctrl+shift+B). Both builds output `RunicRogues.pdx`, which can be run in the Playdate Simulator or sideloaded onto a device.

## Project Structure
- `assets/`: source art and audio files (.aseprite, music projects, mockups, Catalog art, etc.)
- `Source/`: Playdate game assets bundled into the .pdx
  - `audio/music/`: music for each area, the title, and the tutorial
  - `data/localization.csv`: all English and Japanese strings
  - `fonts/`: contains the fonts used: [m5x7](https://managore.itch.io/m5x7) and [m6x11](https://managore.itch.io/m6x11) by managore, and KH-Dot fonts for Japanese text
  - `images/`: all game images (enemies, environment textures, spells, runes, UI, etc.)
  - `launcherImages/`: contains all images used by Playdate launcher
  - `pdxinfo`: game metadata
- `src/`: contains all source code
  - `game/`
    - `enemies/`
      - `enemies.c` - Enemy data, spawning, and state machine
      - `enemyAI.c` - Per-enemy-type attack behaviors
    - `entities/`
      - `entities.c` - Rendering of the billboard sprites (enemies, projectiles, exit portal, etc.)
      - `interactables.c` - Interactable level objects (exit portal and unused campfire)
      - `pickups.c` - Tutorial arrow item pickups (unused health and xp)
      - `projectiles.c` - Player and enemy projectiles
      - `turret.c` - Stationary turret entities (unused)
    - `map/`
      - `map.c` - 21x21 grid map data and room layouts
    - `player/`
      - `classes/classData.c` - Stats, descriptions, starting spells, and unlock order for each class
      - `spells/`
        - `spellData.c` - Base stats for every spell
        - `spells.c` - Casting and updating the basic/special spell slots
      - `player.c` - Player controller (movement, collision, damage, etc.)
      - `playerStats.c` - Player stats modified by runes
    - `rendering/`
      - `particles.c` - Particle effects (same as other entities - just billboard sprites drawn from spritesheet)
      - `raycasting.c` - The raycasting engine: walls, floors, ceilings, and distance fog (sprites handled by `entities.c`)
      - `textures.c` - Loading and caching wall/floor/ceiling textures per area
    - `settings/`
      - `settings.c` - Settings menu and saved preferences
    - `systems/`
      - `audioManager.c` - Handles all SFX and music
      - `bossCinematic.c` - Boss intro cinematic
      - `chainLightningManager.c` - Chain lightning targeting and drawing
      - `difficultyManager.c` - Per-class difficulty unlocks and the run modifiers for each difficulty level
      - `progressionManager.c` - Room progress, room types (enemy, elite, campfire, boss), and areas
      - `runeManager.c` - Rune data and the modifier callbacks runes use to change spells
      - `runStats.c` - Tracks stats over a run for the game over screen (unused)
      - `shopManager.c` - Rune shop popup shown after clearing a room
      - `spawnManager.c` - Enemy spawning
      - `timerManager.c` - Generic timers
    - `tutorial/`
      - `tutorialDialog.c` - Tutorial text popups
      - `tutorialScene.c` - Tutorial room
    - `UI/`
      - `bossTitle.c` - Boss name title card
      - `interactablePrompt.c` - Generates localized prompt labels (e.g. "Next Floor") drawn above interactables
      - `gameMenu.c` - Pause menu
      - `popups.c` - Reward/selection popups
      - `quitConfirmDialog.c` - Quit run confirmation
      - `runeHUD.c` - Rune icons on the HUD (used for certain runes that have active timers/counts to keep track of)
      - `runeInventory.c` - Full rune inventory list
      - `UI.c` - Health, spell cooldowns, and the rest of the HUD
    - `gameScene.c` - Manager for all game elements, runs every subsystem each frame
  - `globals/`
    - `fixedPoint.h` - Fixed point math helpers
    - `globals.h` - Shared constants (screen size, FOV, map size, etc.)
    - `helpers.c` - Queue, stack, set, fast math, and easing functions
    - `localization.c` - Loads strings from `localization.csv`
    - `saveSystem.c` - Saving and loading data
    - `sceneManager.c` - Scene management with circular wipe transitions
  - `title/`
    - `areaMapScene.c` - Area select map (shows between areas)
    - `classSelectScene.c` - Class and difficulty select, with class unlocks
    - `languageSelectScene.c` - English/Japanese language select (on first game launch)
    - `gameOverScene.c` - Run results
    - `loadingScene.c` - Loading screen
    - `roomMapScene.c` - Room select map between rooms (unused)
    - `titleScene.c` - Title screen
  - `main.c` - Sets up the Playdate event handler and update callback
- `tools/`
  - `release.ps1` / `release.sh` - Bumps the build number, builds device + simulator, and zips a release into `releaseBuilds/`
  - `sfx-designer.html` - Browser tool for faster Playdate synth SFX iteration
- `CMakeLists.txt` / `CMakePresets.json` - Build configuration for simulator and device

## Credits
- [3D Sage's raycasting tutorials](https://www.youtube.com/watch?v=gYRrGTC7GtA)
- [Lode Vandevenne's raycasting tutorial](https://lodev.org/cgtutor/raycasting.html)
- [KayKit Adventurers](https://kaylousberg.itch.io/kaykit-adventurers) 3D models by Kay Lousberg
- [1-Bit Patterns and Tiles](https://piiixl.itch.io/1-bit-patterns-and-tiles) by PiiiXL
- [Catacombs of the Damned](https://community.arduboy.com/t/catacombs-of-the-damned/6565) on Arduboy
- Fonts: [m5x7](https://managore.itch.io/m5x7) and [m6x11](https://managore.itch.io/m6x11) by managore, Metamorphous (title), and KH-Dot fonts (Japanese)

## AI Disclosure
First year of development (Feb 14th, 2025 to March 8th, 2026), no AI was used (3D rendering and base framework of the engine). If you would like to reference non-AI assisted code, please refer to commits on and before `6fa3085`. Starting on March 13, 2026 (commit `bd3468b`), AI was used to assist with code to add content until release (~6 months).

No AI was used for any assets (art, music, localization, etc.)

## License
All code is licensed under the terms of the MIT license.
