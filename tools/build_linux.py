"""Cross-build jk_botti (SGTC port) for 32-bit Linux from Windows with Zig.

Mirrors the upstream Makefile's Linux full-plugin variant (same sources, flags
and zlib with Z_PREFIX), with zig/clang instead of g++ -m32:
  - target x86-linux-gnu.2.17: i386, glibc symbol versions <= 2.17;
  - linked with `zig cc` like upstream links with $(CC): no C++ runtime at all
    (jk_botti uses no new/delete/RTTI/exceptions), so no libstdc++ / libc++;
  - -z defs: any symbol missing from libc/libm/libdl fails the link;
  - only the metamod entry points are exported (-fvisibility=hidden + version script).

Usage:
  py -3 -I build_linux.py <jk_botti repo> [--variant sse3|x87] [--out <file.so>] [-j N] [--zig <zig.exe>]
Default output: <repo>/jk_botti_mm_i386.so (the sse3 variant, as installed on Windows).
"""
import argparse
import concurrent.futures
import os
import pathlib
import subprocess
import sys
import zipfile

HERE = pathlib.Path(__file__).resolve().parent
ZIG_VER = "0.15.2"
ZIG_DIR = HERE / "toolchain" / ("zig-x86_64-windows-" + ZIG_VER)
ZIG_ZIP = HERE / "toolchain" / "dl" / ("zig-x86_64-windows-" + ZIG_VER + ".zip")
TARGET = "x86-linux-gnu.2.17"

SRC = """bot.cpp bot_chat.cpp bot_client.cpp bot_combat.cpp bot_trace.cpp bot_config_init.cpp
bot_models.cpp bot_navigate.cpp bot_query_hook.cpp bot_query_hook_linux.cpp bot_query_hook_win32.cpp
bot_skill.cpp bot_sound.cpp bot_weapons.cpp commands.cpp dll.cpp engine.cpp h_export.cpp random_num.cpp
safe_snprintf.cpp util.cpp waypoint.cpp sgtc.cpp""".split()
ZLIB = """adler32 compress crc32 deflate gzclose gzlib gzread gzwrite infback inffast inflate
inftrees trees uncompr zutil""".split()
EXPORTS = """GiveFnptrsToDll Meta_Query Meta_Attach Meta_Detach GetEntityAPI2 GetEntityAPI2_POST
GetEngineFunctions GetEngineFunctions_POST""".split()

TARGETFLAGS = {
    # upstream's clang flags without -mtune=generic, which zig rejects
    "sse3": ["-march=i686", "-msse3"],
    "x87": ["-march=i686"],
}


def zig_exe(arg):
    if arg:
        return arg
    exe = ZIG_DIR / "zig.exe"
    if not exe.exists():
        # something on this PC deletes .exe files under %TEMP%: put it back from the zip
        with zipfile.ZipFile(ZIG_ZIP) as z:
            z.extract("zig-x86_64-windows-%s/zig.exe" % ZIG_VER, HERE / "toolchain")
    return str(exe)


def run(cmd, cwd):
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    return r.returncode, (r.stdout + r.stderr).strip(), cmd


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("repo")
    ap.add_argument("--variant", default="sse3", choices=sorted(TARGETFLAGS))
    ap.add_argument("--out")
    ap.add_argument("-j", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--zig")
    a = ap.parse_args()

    repo = pathlib.Path(a.repo).resolve()
    zig = zig_exe(a.zig)
    env_cache = HERE / "toolchain" / "cache"
    os.environ.setdefault("ZIG_GLOBAL_CACHE_DIR", str(env_cache / "global"))
    os.environ.setdefault("ZIG_LOCAL_CACHE_DIR", str(env_cache / "local"))
    objdir = repo / "obj" / ("linux-zig-" + a.variant)
    (objdir / "zlib").mkdir(parents=True, exist_ok=True)
    out = pathlib.Path(a.out).resolve() if a.out else repo / "jk_botti_mm_i386.so"

    try:
        note = subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=repo, capture_output=True,
                              text=True).stdout.strip() or "unknown"
    except OSError:
        note = "unknown"
    suffix = "" if a.variant == "shim" else "_" + a.variant
    arch = ["-target", TARGET, "-fPIC"] + TARGETFLAGS[a.variant]
    opt = ["-O2", "-fomit-frame-pointer", "-g", "-fno-semantic-interposition"]
    base = ["-Wall", "-Wno-write-strings", "-DVER_MAJOR=0", "-DVER_MINOR=0", '-DVER_NOTE="git%s"' % note,
            '-DVARIANT_VERSION_SUFFIX="%s"' % suffix, "-fno-strict-aliasing", "-fno-strict-overflow",
            "-fvisibility=hidden",
            # clang/zig only: zig makes __DATE__ an error (dll.cpp's plugin info uses it); the
            # memset warning is about upstream's posdata_list.h, not new code
            "-Wno-date-time", "-Wno-nontrivial-memcall"]
    inc = ["-I./metamod", "-I./common", "-I./dlls", "-I./engine", "-I./pm_shared"]
    cxxflags = ["-fno-rtti", "-fno-exceptions"] + base + opt + arch + inc
    # zlib: upstream runs zlib/configure, which only adds -O3 and -D_LARGEFILE64_SOURCE-style tests;
    # the source zconf.h already has the configured Z_HAVE_UNISTD_H/Z_HAVE_STDARG_H
    cflags = opt + arch + ["-DZ_PREFIX", "-fvisibility=hidden", "-w"]

    jobs = []
    for f in SRC:
        jobs.append([zig, "c++"] + cxxflags + ["-c", f, "-o", str(objdir / (f[:-4] + ".o"))])
    for f in ZLIB:
        jobs.append([zig, "cc"] + cflags + ["-c", "zlib/%s.c" % f, "-o", str(objdir / "zlib" / (f + ".o"))])

    failed = False
    with concurrent.futures.ThreadPoolExecutor(a.j) as ex:
        for code, text, cmd in ex.map(lambda c: run(c, repo), jobs):
            name = cmd[cmd.index("-c") + 1]
            if code:
                failed = True
                print("FAILED", name, "\n", text)
            elif text:
                print("--", name, "\n", text)
    if failed:
        sys.exit(1)

    vs = objdir / "exports.map"
    vs.write_text("{\n  global:\n" + "".join("    %s;\n" % e for e in EXPORTS) + "  local:\n    *;\n};\n")
    objs = [str(objdir / (f[:-4] + ".o")) for f in SRC] + [str(objdir / "zlib" / (f + ".o")) for f in ZLIB]
    link = [zig, "cc", "-target", TARGET, "-shared", "-fPIC", "-g", "-o", str(out)] + objs + [
        "-ldl", "-lm", "-Wl,-z,defs", "-Wl,-z,noexecstack", "-Wl,-z,relro", "-Wl,--hash-style=both",
        "-Wl,--version-script," + str(vs), "-Wl,-soname,jk_botti_mm_i386.so"]
    code, text, _ = run(link, repo)
    if text:
        print(text)
    if code:
        print("LINK FAILED")
        sys.exit(1)
    print("built", out, out.stat().st_size, "bytes")


if __name__ == "__main__":
    main()
