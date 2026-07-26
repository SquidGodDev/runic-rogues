# TODO
- Check https://saffroncr.itch.io/katavatis/devlog/1534514/building-a-tiny-3d-renderer-for-a-tiny-handheld for perf improvement notes
- Add roar sfx to second boss phase
- Add language select on launch (or start on the title screen first?)

## Playthrough Notes
- Adjust move speed, health per class
- Change banshee to summoner class
- Black hole spell
- Slow effect doesn't feel impactful
- Projectile speed feels so-so (make base speed slower?)

Translation Review Order:
1. Title
2. Tutorial
3. Area names
4. Class names
5. Spell names
6. Class descriptions
7. Rune names
8. Rune descriptions
9. Settings

Implement the following runes. Any rune icons should copy the images/runes/empty.png as a placeholder. Any rune HUD icons should be a transparent empty square for now. Any numbers used in the runes should be stored as primary/secondary/tertiary values into the RuneData itself to make it easy to adjust.

Slow/Freeze:
- Blizzard: Killing an enemy slows nearby enemies.
- [Change] Frost Shard: Enemies have a (20%, 40%, 60%) of being slowed when damaged
- [Change] Ice Lance: Slowing effects stack until the enemy is frozen solid (5 stacks)
- Shatter: Deal bonus damage to slowed/frozen enemies
- Snowball: Every 5 seconds, throw out a (1, 2, 3) projectiles towards random enemies that slows and damages them
- Ice Cube: Dashing through enemies slows them

Dash:
- Thunder Boots: Dashing arcs lightning towards the nearest 1, 2, 3 enemie(s)
- Kunai: Dashing through enemies damages them
  - 5 damage
- Feather: Dash distance is doubled
- Locomotive: Deal increased damaged based on your current movement speed
  - Uncapped. Scale is that if the player is going 100% faster than their base speed (double their base speed), that equals 50% more damage

Mage:
- High Voltage: Lightning spell arcs to (1, 2, 3) more enemies

Knight:
- Throat Spray: Increases range and damage of Roaring Shout.

Ranger:
- Bomb Arrow: Max charge arrows have a 20% chance of dropping a bomb on the first enemy hit
- Quickdraw: Decrease bow draw time by (15%, 30%, 50%)

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
## Ascension difficulty
  1. Heal less after bosses (Heal 50% of missing health instead of 100% after each floor)
  2. Enemies are more challenging (non-boss enemies deal 20% more damage and attack cooldown is 20% less)
  3. Bosses are more challenging (use Difficulty 1 in the boss properties)
  4. Start each run with less health (Start at 80% health)
-8, +8 x
-16 y
  
  - Less health
  - Heal less after each area
  - No healing
  - Enemies attack faster
  - Enemies move faster
  - Enemies more challenging attack patterns
  - Enemies more damage
  - Enemies more health
  - Boss more challenging attack patterns
  - Boss even more challenging
## Credits
  - https://kaylousberg.itch.io/kaykit-adventurers
  - https://piiixl.itch.io/1-bit-patterns-and-tiles
  - https://community.arduboy.com/t/catacombs-of-the-damned/6565
  - 3D Sage raycasting
  - https://lodev.org/cgtutor/raycasting.html
  - Fonts
    - Metamorphous (title)