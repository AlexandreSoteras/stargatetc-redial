# StargateTC: Redial — details

[Back to the README](../README.md)

- [What the installer changes](#what-the-installer-changes)
- [Installing by hand](#installing-by-hand)
- [Installer options](#installer-options)
- [Server settings](#server-settings)
- [Bot commands](#bot-commands)
- [Maps](#maps)
- [Crash fixes](#crash-fixes)
- [Known issues](#known-issues)
- [Troubleshooting](#troubleshooting)

## What the installer changes

All paths are in `...\steamapps\common\Half-Life\stargatetc\`.

| File | Change | Why |
|---|---|---|
| `config.cfg` (+ `autoexec.cfg`, `userconfig.cfg`) | `+portrait` binds removed | HL25 runs `-portrait` at startup, before the mod's UI exists, and `client.dll` crashes |
| `Titles.txt` | CRLF → LF line endings | HL25 keeps the `\r`, which ends up in an image file name and trips a file-system assertion when you join a team |
| `Liblist.gam` | `gamedll` → Metamod | Metamod loads the bot plugin, then the mod's own `dlls/hl.dll` |
| `addons\metamod\` | new: `dlls\metamod.dll`, `config.ini`, `plugins.ini` | if a Metamod is already installed, it is kept and `plugins.ini` gets one more line |
| `addons\jk_botti\` | new | the bot plugin, its config, chat lines, names and waypoints |
| `resource\background\800_3_c_loading.tga` | version stamp | "STARGATETC REDIAL x.y.z" in the bottom-right corner of the menu |
| `commandmenu.txt` | "Bots" submenus in "Server Admin" and "Rcon" | control the bots from the in-game menu |
| `server.cfg`, `listenserver.cfg` | `mp_wepselmode 2`, only with `-PackWeapons` | class weapon packs |

- **Backups:** each file is backed up once, as `<file>.sgtcbots.bak`, so the backup is always your original. The list of changes is kept in `sgtcbots_manifest.json`.
- **Uninstalling:** `uninstall.bat` restores the backups and deletes what the installer added.
  - The game rewrites `config.cfg` every time you quit. If it changed since the install, your current copy is kept as `config.cfg.sgtcbots.prev`.
  - Restoring the original `config.cfg` and `Titles.txt` brings back the HL25 crashes. `-KeepFixes` keeps them fixed.

## Installing by hand

Back up each file before you edit it.

1. **`config.cfg`:** delete the line `bind "KP_LEFTARROW" "+portrait"`, and any other `+portrait` bind.
2. **`Titles.txt`:** convert it to LF (Unix) line endings, for example with Notepad++ (Edit > EOL Conversion > Unix).
3. **`Liblist.gam`:** replace `gamedll "dlls/hl.dll"` with `gamedll "addons/metamod/dlls/metamod.dll"`.
4. **The release files:** copy the zip's `payload\addons` and `payload\resource` folders into `stargatetc`.
5. **Metamod:** put Metamod-P's `metamod.dll` in `addons\metamod\dlls\`.
6. **`addons\metamod\config.ini`:** create it, containing `gamedll dlls/hl.dll`.
7. **`addons\metamod\plugins.ini`:** create it, containing `win32 addons/jk_botti/dlls/jk_botti_mm.dll`.
8. **Optional:** set `mp_wepselmode 2` in `server.cfg` and `listenserver.cfg`.

## Installer options

```bat
install.bat -GameDir "D:\Games\Steam\steamapps\common\Half-Life\stargatetc"
install.bat -PackWeapons      :: also set mp_wepselmode 2 (class weapon packs)
install.bat -DetectOnly       :: only print the folder it would use
install.bat -Force            :: also replace your jk_botti.cfg, waypoints, or another Metamod build
install.bat -DownloadMetamod  :: if the zip has no metamod.dll: fetch Metamod-P 1.19p32 and check its SHA-256
```

## Server settings

The "Create Server" dialog of HL25 no longer shows the StargateTC options. They are set in `server.cfg` (dedicated server) and `listenserver.cfg` (your own server). The game reads both at every map change, so keep them identical.

- **`mp_gamemode`:** 0 Normal, 1 Ticket, 2 Conflict, 3 Unas, 4 Captain.
  - **Normal** is the mode the bots are made for: key and bomb, C4, goals.
  - **Ticket** respawns players, but it turns the key and bomb off, and the bots still play attack and defense.
  - Conflict, Unas and Captain were not tested with bots.
- **`mp_wepselmode`:** 0 knife only, 1 point-buy, 2 class packs. Bots get a class pack in every mode.
- **`mp_roundtime`:** in Normal mode, the dead wait for the next round.
  - When the first human picks a class while only bots are playing, the plugin restarts the round, so that human plays right away.
  - Turn it off with `jk_botti sgtc_humanreset 0`.

Starting a server:

- **Listen server:** `hl.exe -game stargatetc +maxplayers 16 +map sg_loop`.
- **Dedicated server:** `hlds.exe -console -game stargatetc +maxplayers 16 +map sg_loop`.

## Bot commands

Type them in the server console, or in your own console on a listen server.

| Command | What it does |
|---|---|
| `jk_botti addbot ["" "" <skill>]` | add a bot |
| `jk_botti botskill <1-5>` | default skill of new bots (1 = best) |
| `jk_botti min_bots <n>` / `max_bots <n>` | automatic count: at least n bots, and bots + humans = max. Set both. |
| `jk_botti bot_chat_percent <0-100>` | how chatty the bots are (0 = silent) |
| `jk_botti sgtc_humanreset 0/1` | the first human restarts the round (on by default) |
| `jk_botti sgtc_dial 0/1` | stop or allow the bots dialing stargates |
| `jk_botti sgtc_dump` | every player's team, class, health, position and weapons |
| `kick "<name>"` | remove a bot |
| `meta list` | check that `jk_botti` is `RUN` |

- **At every map start,** `addons\jk_botti\jk_botti.cfg` runs, and each `addbot` line adds one bot.
- **For one map only,** you can put a different set in `jk_botti_<mapname>.cfg`.
- **Bot names** are picked at random from `jk_botti_names.txt`.
- **Chat lines** are in `jk_botti_chat.txt`.

## Maps

| Map | Objective | Bots |
|---|---|---|
| sg_newstrike | key → bomb | ✔ take the key and arm the bomb |
| sg_chateau | key → bomb, rings | ✔ ride the rings, carry the key up |
| sg_stargate | key → bomb, C4, gates, rings | ✔ dial and travel; rings crush a bot now and then |
| sg_nox | goal through the stargate | ✔ the Goa'uld dial gate 1 and go through |
| sg_temple | goal, stargates | ✔ gate 2 is dialed and used |
| sg_land3 | goal | ✔ |
| sg_loop | goals, C4 walls | ✔ plant C4 and blow the walls |
| sg_quietvillage | team deathmatch | ✔ |
| sg_crypt | team deathmatch | ✔ jump the broken bridge (a few still fall) |
| sg_thor | team deathmatch | ✔ |
| sg_shunky | goal behind a Tau'ri-only door | ✔ busy fights; the goal door is rarely opened |
| sg_hina | key → bomb, capture zone, obelisk | ✔ take the key, use the obelisk |
| sg_scorpus | C4 zones, lift, portal | ✔ ride the lift, plant C4, come back through the portal |
| sg_strike3 | C4 zones, rings, ships | ✔ open the spawn doors, plant C4 |
| sg_prison | key → bomb, rings | ✔ open the doors, ring between the areas |
| sg_tokra2 | key → bomb, ring transporters | ✔ ring between the underground and the surface |
| sg_target2a | team deathmatch | ✔ use the teleporter pads |
| sg_tekluhanak | goal | ✔ large open map, slower fights |
| ctp_conquest | capture the point | ✔ very busy fights |
| ctp_frozen | 3 capture points | ✔ |
| sg_conquest | steal the access card | ◐ the Tau'ri don't get through the SGC gate |
| sg_pyramide | goal, stargate | ◐ the Tau'ri rarely get into the pyramid: its only way in is a crouch-jump step |
| ctp_roast1–3 | hidden-tile maze | ✘ not supported, leave them out of the rotation |

## Crash fixes

The plugin guards against these bugs of StargateTC 2.0 itself, with or without bots:

- **`PlayerKilled`:** when a player is killed by a pit, a kawoosh room or a train, it writes past the end of the killer object and corrupts memory.
  - The crash comes later, at round end: `hl.dll+0xbc420` and nearby.
- **The round restart deletes weapons that players still hold** (`hl.dll+0x85607`).
- **`SetupVisibility`** dereferences a missing player while a client connects.
- **Linux:** the game removes a player who has no team yet, which leaves a "ghost" that crashes the next map change.
- **Linux:** the round restart removes a corpse's flames after they are already gone (`CStargateTeamDM::NewRound`).

## Known issues

- **Linux servers:** they work, and the public server runs on Linux. The release zip is Windows-only, though: build the Linux plugin from source ([BUILDING.md](BUILDING.md)).
- **Maps:** see the ◐ entries above. On sg_strike3, C4 plants around the sarcophagus often fail. On sg_target2a, bots sometimes fall off the Goa'uld walkways.
- **Bot names:** the engine removes a leading `#` from player names, so use `[#tag] Name`, not `#tag Name`.
- **A limit of the mod itself:** two players dialing both gates of a map at once can crash the server.

## Troubleshooting

- **The game closes right after starting:** a key is bound to `+portrait` again. Run `install.bat` again, and never bind `+portrait`.
- **"Assertion failed: !strchr(filename, '\r')" when joining a team:** `Titles.txt` has CRLF line endings again. Run `install.bat` again.
- **No bots:** `meta list` should show `jk_botti`.
  - If `meta` is an unknown command, `Liblist.gam` doesn't load Metamod.
  - If jk_botti is missing, check `addons\metamod\plugins.ini` and that `addons\jk_botti\dlls\jk_botti_mm.dll` exists.
- **The game restarted in software mode after a crash:** set the renderer back to OpenGL in Options > Video.
- **Finding the cause of a crash:**
  - **Game client:** Windows Event Viewer > Windows Logs > Application, "Application Error": it gives the faulting module and offset.
  - **Dedicated server:** start it with `-condebug +log on`. It writes `qconsole.log` and `stargatetc\logs\`.
