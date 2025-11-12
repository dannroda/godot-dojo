#!/usr/bin/env python
import os
import subprocess
import glob
import shutil
import platform as host_platform
import sys
import re

# --- IDE helper imports for SCons (no effect during actual scons run) ---
try:
    from SCons.Script import GetOption, Return, Exit, SConscript, COMMAND_LINE_TARGETS, Builder, Depends, Alias, Default, AddPostAction
except ImportError: # pragma: no cover
    # Dummy implementations for IDE type hinting
    def GetOption(name: str): return None
    def Return(*args, **kwargs): pass
    def Exit(code=0): sys.exit(code)
    def SConscript(*args, **kwargs): return {}
    def Builder(*args, **kwargs): return None
    def Depends(*args, **kwargs): pass
    def Alias(*args, **kwargs): pass
    def Default(*args, **kwargs): pass
    def AddPostAction(*args, **kwargs): pass
    COMMAND_LINE_TARGETS = sys.argv

# --- Setup de Colores y Emojis ---
G, B, R, Y, X = '\033[92m', '\033[94m', '\033[91m', '\033[1;33m', '\033[0m'

is_windows = host_platform.system().lower() == "windows"
if is_windows: # pragma: no cover
    rocket, broom, check, package, clipboard, party, cross = ">", "-", "+", "#", "=", "!", "x"
else:
    rocket, broom, check, package, clipboard, party, cross = "🚀", "🧹", "✅", "📦", "📋", "🎉", "❌"

print(f"{B}{rocket} Building godot-dojo{X}")

# --- Lógica de Limpieza ---
if GetOption('clean'): # pragma: no cover
    print(f"{Y}{broom} Starting cleanup process...{X}")

    # Limpiar proyecto Rust godot-dojo-core
    print(f"{B}  -> Cleaning Rust project: godot-dojo-core{X}")
    try:
        subprocess.run(["cargo", "clean"], cwd="godot-dojo-core", check=True, capture_output=False, text=True)
        print(f"{G}     {check} godot-dojo-core cleanup complete.{X}\n")
    except subprocess.CalledProcessError as e:
        print(f"{R}     {cross} godot-dojo-core cleanup failed: {e.stderr}{X}")

    # Eliminar directorio del addon
    print(f"{B}  -> Deleting addon directory: demo/addons/godot-dojo{X}")
    shutil.rmtree("demo/addons/godot-dojo", ignore_errors=True)
    print(f"{G}     {check} Addon directory deleted.{X}\n")

    # Eliminar temporales
    print(f"{B}  -> Deleting temporary directory: build{X}")
    shutil.rmtree("build", ignore_errors=True)
    print(f"{G}     {check} Temporary directory deleted.{X}\n")

    # Eliminar dojo/controller bindings
    print(f"{B}  -> Deleting bindings directory: bindings{X}")
    shutil.rmtree("bindings", ignore_errors=True)
    print(f"{G}     {check} Bindings directory deleted.{X}\n")

    # Limpiar submódulo godot-cpp
    print(f"{B}  -> Cleaning godot-cpp submodule...{X}")
    subprocess.run(["scons", "-C", "external/godot-cpp", "--clean"], check=False, capture_output=False)
    print(f"{G}     {check} godot-cpp cleanup complete.{X}\n")

    print(f"{G}{check} Cleanup process finished.{X}")
    Return()

# --- Setup Inicial ---
os.makedirs("demo/addons/godot-dojo", exist_ok=True)

env = SConscript("external/godot-cpp/SConstruct")
platform = env["platform"]
arch = env["arch"]
target = env.get("target", "template_debug")

# --- Validación de Comandos ---
if "assemble-ios" in COMMAND_LINE_TARGETS and platform != "ios":
    print(f"{R}{cross} The 'assemble-ios' target requires 'platform=ios'. Please run 'scons platform=ios assemble-ios'.{X}")
    Exit(1)

build_info = f"{platform} ({arch}) - {target}"
print(f"{B}Building: {build_info}{X}")

# --- Carga de Configuración Específica de la Plataforma ---
try:
    platform_config = SConscript(f"buildsystem/{platform}.scons", exports="env")
except Exception:
    print(f"{R}{cross} Failed to load configuration for platform '{platform}'. Make sure 'buildsystem/{platform}.scons' exists.{X}")
    Exit(1)

# --- Funciones Auxiliares ---
def _get_git_submodule_version(submodule_path):
    repo_path = os.path.join(os.getcwd(), submodule_path)
    try:
        return subprocess.check_output(["git", "describe", "--tags", "--abbrev=0"], cwd=repo_path, stderr=subprocess.DEVNULL, text=True).strip()
    except Exception:
        try:
            return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], cwd=repo_path, stderr=subprocess.DEVNULL, text=True).strip()
        except Exception:
            return "unknown"

def _detect_godot_min_requirement():
    version_source = _get_git_submodule_version("external/godot-cpp")
    m = re.search(r"(\d+)\.(\d+)", version_source)
    return f"{m.group(1)}.{m.group(2)}" if m else "4.2"

# --- Configuración de la Compilación ---

# 1. Compilación de Rust (Paso Previo)
from buildsystem.rust import build_rust_dependency

rust_target_or_list = platform_config['get_rust_target'](arch, env)
rust_target_list = rust_target_or_list if isinstance(rust_target_or_list, list) else [rust_target_or_list]
force_rust_release = os.environ.get("FORCE_RUST_RELEASE", "0") == "1"
is_release_build = target == "template_release" or force_rust_release

# Invocamos la compilación de Rust de forma imperativa.
# Esto bloquea la ejecución hasta que Rust termine, garantizando que los artefactos estén listos.
build_rust_dependency(is_release_build, rust_target_list, platform)

# 2. Configuración de C++
if env["platform"] != "android":
    env['SHLIBPREFIX'] = ''
prefix = env.subst('$SHLIBPREFIX')

env.Append(CPPPATH=["src/", "include/", "bindings/", "external/boost/include", "#bindings/"])
platform_config['configure_compiler'](env)

# 3. Definición de Fuentes C++
rust_build_mode = "release" if is_release_build else "debug"
rust_lib_target_dir = "universal" if len(rust_target_list) > 1 else rust_target_list[0]
rust_lib_path = platform_config['get_rust_lib_path'](rust_lib_target_dir, rust_build_mode)

sources = sorted(glob.glob("src/**/*.cpp", recursive=True)) + [
    "bindings/controller/controller.cpp",
]

# Añadir documentación si es necesario
if target in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=glob.glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

# 4. Configuración del enlazador
_godot_tag = _get_git_submodule_version("external/godot-cpp")
print(f"{Y}{clipboard} Building GDExtension with godot-cpp version: {_godot_tag}.{X}")
platform_config['configure_linking'](env, rust_lib_path)

# 5. Creación de la Librería Compartida
output_dir, lib_name = platform_config['get_output_paths'](env, prefix, target, arch)
os.makedirs(output_dir, exist_ok=True)

# La compilación de C++ comenzará aquí. Como el script de Rust ya se ejecutó,
# los bindings en `bindings/` y la librería estática en `rust_lib_path` ya existen.
library = env.SharedLibrary(target=lib_name, source=sources)

target_out_dir = target if target == "editor" else target.split('_', 1)[1]
Alias(f"godot-dojo-{platform}-{target_out_dir}", library)

# --- Pasos Post-Compilación ---
def post_build_actions(target, source, env):
    # 1. Mensaje de finalización
    print(f"{G}{party} Build complete for {str(target[0])}!{X}")

    # 2. Generar .gdextension
    print(f"{Y}{clipboard} Generating .gdextension file...{X}")
    with open("plugin_template.gdextension.in", 'r') as f:
        template = f.read()

    gdext = template.replace("${PROJECT_NAME}", "godot-dojo")
    gdext = gdext.replace("${ENTRY_POINT}", "dojoc_library_init")
    gdext = gdext.replace("${GODOT_MIN_REQUIREMENT}", _detect_godot_min_requirement())

    with open("demo/addons/godot-dojo/godot-dojo.gdextension", 'w') as f:
        f.write(gdext)
    print(f"{G}{check} .gdextension file generated successfully.{X}")
    return None

AddPostAction(library, post_build_actions)

# 3. Lógica específica de la plataforma (ej. XCFramework para iOS)
platform_config['post_build_logic'](env, COMMAND_LINE_TARGETS, target, arch)

# --- Target por Defecto ---
Default(library)