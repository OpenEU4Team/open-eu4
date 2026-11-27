# Open EU4

Open EU4 is a community-built interpretation of Europa Universalis IV - our own
reading of how the game behaves, not its original source code. It runs alongside
your own copy of eu4 rather than replacing it, and gives players and modders a
window into what the game is actually doing while it runs.

Because this is an interpretation, we do not claim it behaves exactly as the
original engine did. We check it by launching the real game with it and watching
what happens. Differences and bugs are expected; if you find one, please tell
us.

The goal is a foundation the community can keep building on long after official
support ends: modders get room to add features the game never had, and players
keep a way to preserve and repair it, including long-standing problems such as
[ignored bug-reports](https://forum.paradoxplaza.com/forum/threads/game-crashes-on-macos-after-1-36-kok.1612265)
that will never be fixed. You paid for this game, and you should be able to keep 
playing it for as long as you want. That belief is what drives Open EU4.

## Setup Instructions:

### Requirements:

- Linux x86_64
- Your own copy of EU4 on Linux (we don't provide this)
- libeu4_override.so. See releases, or compile from source (see build instructions)

1. Place `libeu4_override.so` in the EU4 game installation path. On the Linux Steam build this is `~/.local/share/Steam/steamapps/common/Europa Universalis IV`. Check with `find / -type f -name "eu4" 2>/dev/null`
2. Launching:
	- Launch directly: `LD_PRELOAD=./libeu4_override.so ./eu4`
	- Or, to launch through the Paradox Launcher: edit `launcher-settings.json`'s `exePath` to point at a small script (See `runner.sh` in this repository)
3. Run the game like normal.

## Build Instructions:

### Requirements:

- git
- GNU make, g++ with C++17
- python3 and `readelf`

### Commands:

1. Clone the repository
2. cd ./open-eu4
3. Run:
	```sh
	make help			      # Show this command page
	make APP=/path/to/eu4                 # Build the production library under build/prod/libeu4_override.so
	make release APP=/path/to/eu4 TAG=v1  # Build and create a local annotated source tag

5. Copy `build/prod/libeu4_override.so` into your game installation folder and follow the setup instructions above.

## FAQ:

> Can I run this on Windows or Mac?

No. We only support Linux at the moment.

> Can I contribute?

Contribution and help is welcome. Reach out directly or join our discord for more info.

> Can I fork this and make my own changes?

Mods are welcome, but if you find your project overlapping with ours, consider contributing instead.

> Do you support older versions of the Game?

No. We only support v1.37.5. Other 1.37 versions may work; but don't count on it.

## Contact

Email: jsb1072@proton.me
Discord: https://discord.gg/fh3DAm9sW8
