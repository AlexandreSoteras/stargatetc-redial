# Building from source

[Back to the README](../README.md)

The bot is [jk_botti](https://github.com/Bots-United/jk_botti) plus one patch (`src/jk_botti_sgtc.patch`). The StargateTC module is `src/sgtc.cpp` and `src/sgtc.h`; both are also contained in the patch.

## Windows plugin (`jk_botti_mm.dll`)

You need MinGW-w64 for i686. The releases use WinLibs GCC with the msvcrt runtime.

```bat
git clone -c core.autocrlf=false https://github.com/Bots-United/jk_botti.git
cd jk_botti
git checkout b8d46629d330928b36f3a523d5f1c1f21b83c628
git apply ..\<this repo>\src\jk_botti_sgtc.patch
mingw32-make OSTYPE=win32 VARIANT=sse3 _build AR="ar rc" RANLIB=ranlib
strip --strip-debug jk_botti_mm_sse3.dll
copy jk_botti_mm_sse3.dll ..\<this repo>\payload\addons\jk_botti\dlls\jk_botti_mm.dll
```

- The base commit is in `src/BASE_COMMIT.txt`. The patch has LF line endings: clone with `core.autocrlf=false` as above, or `git apply` fails.
- `strip --strip-debug` removes the debug information, which would otherwise embed your build paths.

## Linux plugin (`jk_botti_mm_i386.so`)

`tools/build_linux.py` cross-compiles the patched tree from Windows with [Zig](https://ziglang.org/) 0.15.2 (target `x86-linux-gnu.2.17`, so the plugin runs on any Linux with glibc 2.17 or later):

```bat
py tools\build_linux.py <path to the patched jk_botti tree> --zig <path to zig.exe>
```

To run a Linux dedicated server:

1. Install HLDS (app 90) and copy your `stargatetc` folder there. Map, model and sound file names must keep their exact case.
2. In `liblist.gam`, set `gamedll_linux "addons/metamod/dlls/metamod.so"`.
3. Install Metamod-P 1.21p109 for Linux as `addons/metamod/dlls/metamod.so`.
4. Create `addons/metamod/config.ini` containing `gamedll dlls/stc_i386.so`.
5. Create `addons/metamod/plugins.ini` containing `linux addons/jk_botti/dlls/jk_botti_mm_i386.so`.
6. Start the server with `./hlds_run -game stargatetc +maxplayers 16 +map sg_newstrike`.

At startup, the server console should show `PlayerKilled crash fix installed`, `removed-weapon crash fix installed` and `round-restart corpse fix installed`.

## Menu stamp

`tools/stamp_splash.py` writes "STARGATETC REDIAL x.y.z" on the menu background, taking the version from `VERSION`. It needs Python and Pillow:

```bat
py tools\stamp_splash.py --game "<...>\Half-Life\stargatetc" --out payload --preview preview.png
```

## Tests

`tests\test-installer.ps1` runs install and uninstall on throwaway copies of a StargateTC folder and checks the result byte for byte.

## Versioning

The project uses [semantic versioning](https://semver.org/). The version is written only in `VERSION`. The installer, the menu stamp and the git tag (`v1.0.0`) all read it from there.
