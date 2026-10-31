# Ship Straight Assist

Hey! This is a small Geode mod for Geometry Dash 2.2 on mobile (Android).

**What it does:** when you spam-click in ship, the ship stays in a straight line instead of
bouncing up and down everywhere. That's it. Simple.

I'm a beginner at this so it's not perfect, but it works and it's easy to tweak, ok?
If something feels off, just change the settings below. cmon, it takes like 10 seconds.

## What's new in v1.1.0 (big QoL update)
Everything from before still works the same, all your old settings are still there. New stuff:
- **Pause menu button** (bottom left): tap **Assist** and a clean menu opens, so you never have to
  go into the Geode mod menu again.
- **Presets**: Soft, Balanced or Strong in one tap. Strong is the old v1.0 feel.
- **Soft ramp**: eases the assist in instead of snapping on. Feels smoother.
- **Hard flatten**: holds the ship perfectly level while it's on.
- **Cooldown**: stops it re-triggering straight away after it turns off.
- **Practice mode only**: only runs in practice, so it can't touch normal runs.
- **Affect player 2**: also helps the second ship in dual.
- **Quick toggle button**: optional little SA button to switch it on/off mid level (dim = off, hidden when paused). No text pops up on your screen.
- **Session stats**: the menu shows how many times it triggered and how long it helped.
- **Reset button**: puts every setting back to default if you mess it up (we've all done it).

## How it works (the easy version)
- You click fast in ship, the mod notices.
- Once you click enough times fast enough, the assist turns on and calms the ship down.
- When you stop clicking for a moment, it turns itself off again.
- It only touches the ship. Other gamemodes are totally normal.
- It resets when you restart or leave the level.

## Best settings for control
These are the defaults now, and they're what I'd start with:

| Setting | Value | Why |
| --- | --- | --- |
| Clicks to trigger | 3 | Not too jumpy, not too slow to kick in |
| Click window | 0.5 s | Clicks have to be quick to count |
| Stop delay | 0.2 s | Lets go fast so you can steer again |
| Assist strength | 0.5 | Helps flatten the ship but you still feel your clicks |

The new settings (cooldown, soft ramp, hard flatten and so on) are all off or neutral by default, so nothing changes until you turn them on.

Tuning it yourself:
- Ship feels **stuck / too flat**? Turn **strength down** (try 0.3).
- Ship still **bounces too much**? Turn **strength up** (try 0.7).
- Assist turns on **by accident**? Raise **clicks to trigger** to 4 or 5.
- Assist turns off **too early** and you wobble? Raise **stop delay** a little.

I haven't been able to test every level, so play with it and see what feels best for you.

## How to build it (no PC needed, I did it on my phone)
1. Make a free GitHub repo.
2. Upload `main.cpp`, `mod.json` and `CMakeLists.txt` with **Add file > Upload files**.
3. Make one file called `.github/workflows/build.yml` (type that whole name in the box, GitHub makes the folders for you) and paste in the `build.yml` text.
4. Go to **Actions**, run **Build Geode Mod**, wait a few minutes.
5. Download **Build Output**, unzip it, put the `.geode` file in your Geode mods folder, restart the game. Done!

If the build goes red, don't panic. Copy the error text and send it to someone who can help.

## Files
- `main.cpp` the mod itself
- `mod.json` name, version and settings
- `CMakeLists.txt` build setup
- `build.yml` the GitHub build file (makes both 32-bit and 64-bit Android)
