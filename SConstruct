#!/usr/bin/env python
import os
import subprocess
import glob
import shutil
import platform as host_platform
import sys

# Colores
G, B, R, Y, X = '\033[92m', '\033[94m', '\033[91m', '\033[1;33m', '\033[0m'

# Check if running on Windows to avoid encoding issues
is_windows = host_platform.system().lower() == "windows"
if is_windows:
    # Use ASCII alternatives on Windows
    rocket = ">"
    broom = "-"
    check = "+"
    package = "#"
    clipboard = "="
    party = "!"
    cross = "x"
else:
    # Use emojis on other platforms
    rocket = "🚀"
    broom = "🧹"
    check = "✅"
    package = "📦"
    clipboard = "📋"
    party = "🎉"
    cross = "❌"

print(f"{B}{rocket} Building godot-dojo{X}")

# Limpieza
if GetOption('clean'):
    print(f"{Y}{broom} Cleaning...{X}")
    try:
        subprocess.run(["cargo", "clean"], cwd="external/dojo.c", check=True)
        shutil.rmtree("demo/bin", ignore_errors=True)
        subprocess.run(["scons", "-C", "external/godot-cpp", "--clean"], check=False)
    except:
        pass
    print(f"{G}{check} Cleanup complete{X}")
    Return()

# Setup
os.makedirs("demo/bin", exist_ok=True)
env = SConscript("external/godot-cpp/SConstruct")
platform, arch, target = env["platform"], env["arch"], env.get("target", "template_debug")

print(f"{B}Building: {platform} ({arch}) - {target}{X}")

# Compilar Rust
print(f"{Y}{package} Compiling dojo.c...{X}")

# Detectar configuración del host y toolchain
is_host_windows = host_platform.system().lower() == "windows"
use_mingw = env.get("use_mingw", False)

targets = {
    ("windows", "x86_64"): "x86_64-pc-windows-gnu" if (not is_host_windows or use_mingw) else "x86_64-pc-windows-msvc",
    ("linux", "x86_64"): "x86_64-unknown-linux-gnu",
    ("linux", "arm64"): "aarch64-unknown-linux-gnu",
    ("macos", "x86_64"): "x86_64-apple-darwin",
    ("macos", "arm64"): "aarch64-apple-darwin",
    ("web", "wasm32"): "wasm32-unknown-unknown",
    ("android", "arm64"): "aarch64-linux-android"
}
# Use CARGO_BUILD_TARGET from environment if set, otherwise use the target from the platform/arch
rust_target = os.environ.get("CARGO_BUILD_TARGET", targets.get((platform, arch), "x86_64-unknown-linux-gnu"))

# Override for macOS x86_64 to ensure it uses the correct target
if platform == "macos" and arch == "x86_64":
    rust_target = "x86_64-apple-darwin"
elif platform == "macos" and arch == "arm64":
    rust_target = "aarch64-apple-darwin"

# Ensure the Rust target is installed
try:
    print(f"{Y}Checking if Rust target {rust_target} is installed...{X}")
    result = subprocess.run(["rustup", "target", "list", "--installed"], capture_output=True, text=True, check=True)
    installed_targets = result.stdout.splitlines()

    if rust_target not in installed_targets:
        print(f"{Y}Installing Rust target {rust_target}...{X}")
        subprocess.run(["rustup", "target", "add", rust_target], check=True)
        print(f"{G}{check} Rust target {rust_target} installed{X}")
    else:
        print(f"{G}{check} Rust target {rust_target} is already installed{X}")
except subprocess.CalledProcessError as e:
    print(f"{R}{cross} Failed to check or install Rust target: {e}{X}")
    # Continue anyway, as cargo will show a more specific error if needed

cmd = ["cargo", "build", "--target", rust_target]
if target == "template_release":
    cmd.append("--release")

# Variables de entorno para WebAssembly
if env["platform"] == "web":
    env_vars = os.environ.copy()
    rustflags = "-C target-feature=+atomics,+bulk-memory,+mutable-globals"
    rustflags += " -C relocation-model=pic"
    rustflags += " --print=native-static-libs"
    # rustflags += " -C panic=abort"
    # rustflags += " -C opt-level=z"

    env_vars["RUSTFLAGS"] = rustflags

    # env_vars["CC"] = "emcc"
    # env_vars["CXX"] = "em++"
    # env_vars["AR"] = "emar"
    # env.Append(LINKFLAGS=[
        # '-sALLOW_MEMORY_GROWTH',
        # '-sWASM=1',
        # '-sEXPORTED_FUNCTIONS=["_malloc","_free"]',
        # '-sEXPORTED_RUNTIME_METHODS=["ccall","cwrap"]',
        # '-sMODULARIZE=1',
        # '-sEXPORT_NAME="GodotDojo"',
        # '--no-entry'
    # ])


    subprocess.run(cmd, check=True, cwd="external/dojo.c", env=env_vars)
else:
    env_vars = os.environ.copy()
    subprocess.run(cmd, check=True, cwd="external/dojo.c", env=env_vars)

# Configurar biblioteca
if platform != "android":
    # Android build requires lib prefix
    env['SHLIBPREFIX'] = ''
prefix = env.subst('$SHLIBPREFIX')
env.Append(CPPPATH=["src/", "include/", "external/dojo.c"])

if platform == "linux":
    env.Append(LINKFLAGS=['-ldbus-1'])
    # Va forzado porque por alguna razón me usa un standard mas viejo
    env.Append(
        CXXFLAGS=["-std=c++17"]
    )

# Linkear librerías de Rust
build_mode = "release" if target == "template_release" else "debug"
rust_lib_dir = f"external/dojo.c/target/{rust_target}/{build_mode}"

if platform == "windows":
    rust_lib = ""
    target_dll_name = f"dojo_c.dll"
    target_dll_path = f"demo/bin/{target_dll_name}"

    if use_mingw:
        rust_lib = f"{rust_lib_dir}/dojo_c.dll"

    elif is_host_windows:
        rust_lib = f"{rust_lib_dir}/dojo_c.dll.lib"
        env.Append(LINKFLAGS=['/NODEFAULTLIB:MSVCRT'])

    if os.path.exists(rust_lib):
        dll_dest = f"{rust_lib_dir}/{target_dll_name}"
        shutil.copy2(dll_dest, target_dll_path)
        print(f"{G}{clipboard} Copied {dll_dest} -> {target_dll_path}{X}")
        # rust_lib = target_dll_path

elif platform == "web":
    rust_lib = f"{rust_lib_dir}/libdojo_c.rlib"
else:
    rust_lib = f"{rust_lib_dir}/libdojo_c.a"

if os.path.exists(rust_lib):
    env.Append(LIBS=[File(rust_lib)])
else:
    print(f"{R}{cross} Rust library not found: {rust_lib}{X}")

sources = sorted(glob.glob("src/**/*.cpp", recursive=True))

# Crear biblioteca
suffix_map = {
    "linux": f".linux.{target}.{arch}.so",
    "windows": f".windows.{target}.{arch}.dll",
    "macos": f".macos.{target}.{arch}.dylib",
    "web": f".web.{target}.wasm32.wasm"
}

lib_name = f"demo/bin/{prefix}godot-dojo{suffix_map.get(platform, f'.{platform}.{target}.{arch}.so')}"
library = env.SharedLibrary(target=lib_name, source=sources)

# Generar .gdextension
with open("plugin_template.gdextension.in", 'r') as f:
    template = f.read()

gdext = template.replace("${PROJECT_NAME}", "godot-dojo")
gdext = gdext.replace("${ENTRY_POINT}", "dojoc_library_init")
gdext = gdext.replace("${GODOT_MIN_REQUIREMENT}", "4.2")

with open("demo/bin/godot-dojo.gdextension", 'w') as f:
    f.write(gdext)

def build_complete_callback(target, source, env):
    print(f"{G}{party} Build complete!{X}")
    return None

env.AddPostAction(library, build_complete_callback)

env.Default(library)
