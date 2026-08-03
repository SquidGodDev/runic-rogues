# 1.0.0 Changelog
General Changes:
- Made non-boss rooms smaller (7x7 instead of 9x9)
- Swapped skip shop button with reroll (limited to 2 per floor)
- Lock rogue, knight, and ranger classes - requires winning a run with a previous class to unlock 
- Changed exit arrow to be a 3D arrow with the stem to be more clear
Enemy Changes:
- Increased enemy movement speeds across the board
- Change big slime attack pattern to firing a ring of projectiles
- Made serpent tail attack faster
- Increase skeleton move speed and attack range
- Changed imp attack pattern to fire an arc of projectiles
- Changed flying tome attack pattern to firing a ring of projectiles
- Changed living armor attack pattern to a lunging dash
- Changed bomber bombs - now creates a ring of lingering projectiles upon detonation
Boss Changes:
- Made Elder boss root attack a bit faster
- Decrease all bosses health by a bit
- Buffed Wraith boss projectile attacks a bit
- Added wall to wall projectile sweep attack to Dragon boss
- General buffs overall to boss difficulty
Player Changes:
- Reduced knight attack knockback
- Slightly reduced player movement speed
- Added debouncing to the player dash, so accidental dashes caused by hardware sending an extra ghost input should be ignored more often
Rune Changes:
- Added Revive rune
- Buffed Black Hole Rune (requires less kills)
- Nerfed Viking Rune
- Nerf movespeed runes (swift, rapier, crescendo)
- Nerfed "Enchanted Amulet" and "Arcane Staff" runes - change to fire an additional time on every N casts
Onboarding Changes:
- Added language selection screen on first launch
- Show arrow pointing to settings menu in tutorial
- Added tutorial skip confirmation dialog
Bug Fixes:
- Changed exit prompt from "Next Room" to "Next Floor"
- Got rid of all PO (pocket operator) synth sound effects which were causing frame drops (banshee, baby dragon, mummy, imp)

# TODO
- Check https://saffroncr.itch.io/katavatis/devlog/1534514/building-a-tiny-3d-renderer-for-a-tiny-handheld for perf improvement notes
- Add roar sfx to second boss phase
- Add new phases to bosses?
- Increase movement speed and movement patterns of bosses inbetween attacks

# Changes
- Change to every X instead of percent chance (black hole, potion, etc)
- More enemy attack patterns
- Campfire/bonus rune choice

# Balance
- Buff glass dagger

# New features
- Save feature
- Endless mode
- Achievements
- Class locking
- Pause menu map

# Maybe features
- Challenge room
- Meta currency? Coins?

# Rune Ideas
- Rune to buff ranger net
- Slow synergy runes
- Blowing in mic to push enemies back
- Gladiator: Swing crank to do a spin attack (5 second cooldown)
- Revive
- Quick Draw: Faster charge rate for ranger (disable pocket watch for ranger)
- Time freeze/slow time
- Runes when you're out of dashes
- Fire laser when standing still for 3 seconds
- Every x something shoot out ring of projectiles

## Playthrough Notes
- Adjust move speed, health per class
- Change banshee to summoner class
- Slow effect doesn't feel impactful
- Projectile speed feels so-so (make base speed slower?)

Implement the following runes. Any rune icons should copy the images/runes/empty.png as a placeholder. Any rune HUD icons should be a transparent empty square for now. Any numbers used in the runes should be stored as primary/secondary/tertiary values into the RuneData itself to make it easy to adjust.

Slow/Freeze:
- Blizzard: Killing an enemy slows nearby enemies.
- [Change] Frost Shard: Enemies have a (20%, 40%, 60%) of being slowed when damaged
- [Change] Ice Lance: Slowing effects stack until the enemy is frozen solid (5 stacks)
- Shatter: Deal bonus damage to slowed/frozen enemies
- Snowball: Every 5 seconds, throw out a (1, 2, 3) projectiles towards random enemies that slows and damages them
- Ice Cube: Dashing through enemies slows them

Dash:
- Locomotive: Deal increased damaged based on your current movement speed
  - Uncapped. Scale is that if the player is going 100% faster than their base speed (double their base speed), that equals 50% more damage

Knight:
- Throat Spray: Increases range and damage of Roaring Shout.

## Runes
  - Start rune for each class
  - Trigger runes
    - On basic cast
    - On special cast
    - On dash
    - On damaged
    - On damage enemy
    - On kill enemy
    - On dodge with dash (check invincibility in player damage function)
  - Rune ideas
    - Black hole (every X specials)
    - On kill lower special cooldown
    - More damage the closer you are to enemies
    - Clear nearby projectiles
    - Pet
    - Less range, double damage
    - More damage low health
    - More damage full health
    - Spawn random buff areas/pickups around map
## Credits
  - https://kaylousberg.itch.io/kaykit-adventurers
  - https://piiixl.itch.io/1-bit-patterns-and-tiles
  - https://community.arduboy.com/t/catacombs-of-the-damned/6565
  - 3D Sage raycasting
  - https://lodev.org/cgtutor/raycasting.html
  - Fonts
    - Metamorphous (title)