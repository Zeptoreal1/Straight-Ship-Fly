# Ship Straight Assist

A Geode mod for Geometry Dash 2.2 (Android). When you spam-click in ship mode, the ship holds a
straight line instead of bouncing up and down.

## How it works
- Every ship click is timestamped.
- When you click **Clicks to trigger** times within the **Click window**, the assist turns on.
- While it is on, the ship's vertical speed is damped each tick by **Assist strength**.
- It turns itself off **Stop delay** seconds after your last click.
- It only affects the ship, and resets when you restart or leave the level.

## Settings
| Setting | Default | What it does |
| --- | --- | --- |
| Enabled | on | Turns the assist on or off |
| Clicks to trigger | 3 | Fast clicks needed to activate (2-10) |
| Click window (seconds) | 0.6 | Time the clicks must happen within |
| Stop delay (seconds) | 0.3 | How long after your last click it switches off |
| Assist strength | 0.8 | 1.0 = perfectly flat, lower = gentler help |

## Build (no PC needed)
1. Make a GitHub repo and upload `main.cpp`, `mod.json` and `CMakeLists.txt`.
2. Create `.github/workflows/build.yml` (paste the contents of `build.yml`).
3. Open **Actions**, run **Build Geode Mod**, then download **Build Output**.
4. Put the `.geode` file in your Geode mods folder and restart the game.

## Files
- `main.cpp` the mod
- `mod.json` metadata and settings
- `CMakeLists.txt` build config
- `build.yml` GitHub Actions build (Android 32 and 64-bit)
