# TODO
## Playthrough Notes
- Change banshee to summoner class
- Black hole spell
- Increase enemy health in echoing caverns
- Make bomber enemy throw bombs
- Put enemies in attack lockout when spawned
- Custom Rat AI
- Slow effect doesn't feel impactful
- Projectile speed feels so-so (make base speed slower?)
- Adjust rogue damage

Slow/Freeze:
- Blizzard: Killing an enemy slows nearby enemies.
- [Change] Frost Shard: Enemies have a (20%, 40%, 60%) of being slowed when damaged
- [Change] Ice Lance: Slowing effects stack until the enemy is frozen solid (5 stacks)
- Shatter: Deal bonus damage to slowed/frozen enemies
- Snowball: Every 5 seconds, throw out a (1, 2, 3) projectiles towards random enemies that slows and damages them
- Ice Cube: Dashing through enemies slows them

Implement the following runes. For any rune HUD icons, make it a black square temporarily.
Tank:
- Bulwark: Reduce the first instance of damage you take in each room to 1.
  - Make a rune HUD icon for this when active.
- Lightning Rod: Taking damage arcs lightning towards the nearest 5 enemies
- Thorns: Taking damage summons roots around you (in a tight ring 1 cell out, 8 total)
  - This is an existing one, but rewrite the behavior to this.
- Healing Potion: Enemies have a (5%, 10%, 20%) chance of dropping a healing potion
- Stone Golem: Take 20% less damage when standing still
  - Make a rune HUD icon for this when active
- Adrenaline: Below 33% HP, spell cooldowns are halved.
  - Make a rune HUD icon for this when active

Dash:
- Thunder Boots: Dashing arcs lightning towards the nearest 1, 2, 3 enemie(s)
- Kunai: Dashing through enemies damages them (5 damage)
- Feather: Dash distance is doubled
- Locomotive: Deal increased damaged based on your current movement speed

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