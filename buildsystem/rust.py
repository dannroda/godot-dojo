import os
import subprocess

# --- Setup de Colores y Emojis ---
G, B, R, Y, X = '\033[92m', '\033[94m', '\033[91m', '\033[1;33m', '\033[0m'

is_windows = os.name == "nt"
if is_windows:
    package, check = "#", "+"
else:
    package, check = "📦", "✅"

def _get_git_submodule_version(submodule_path):
    """Obtiene la versión de un submódulo de git."""
    repo_path = os.path.join(os.getcwd(), submodule_path)
    try:
        return subprocess.check_output(["git", "describe", "--tags", "--abbrev=0"], cwd=repo_path, stderr=subprocess.DEVNULL, text=True).strip()
    except Exception:
        try:
            return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], cwd=repo_path, stderr=subprocess.DEVNULL, text=True).strip()
        except Exception:
            return "unknown"

def _compile_rust_library(lib_name, lib_path, is_release, rust_target_list, platform, cargo_flags=None, rustc_flags=None):
    """Compila la librería Rust, manejando builds normales y universales."""
    lib_version = _get_git_submodule_version(lib_path) or "unknown"
    print(f"{Y}{package} Compiling {lib_name} ({lib_version})...{X}")

    build_mode = "release" if is_release else "debug"
    env_vars = os.environ.copy()
    current_rustflags = env_vars.get("RUSTFLAGS", "")
    if rustc_flags:
        current_rustflags += " " + " ".join(rustc_flags)
    env_vars["RUSTFLAGS"] = current_rustflags.strip()

    # Universal builds (macOS/iOS)
    if len(rust_target_list) > 1:
        print(f"{Y}Starting universal build for {lib_name} on {platform}...{X}")
        libs_to_lipo = []
        for rt in rust_target_list:
            print(f"{Y}Compiling {lib_name} for target: {rt}...{X}")
            cmd = ["cargo", "build", "--target", rt]
            if is_release: cmd.append("--release")
            if cargo_flags: cmd.extend(cargo_flags)

            # Añadir flags de deployment para macOS
            if platform == "macos":
                deployment_target = os.environ.get("MACOSX_DEPLOYMENT_TARGET", "14.0")
                env_vars["MACOSX_DEPLOYMENT_TARGET"] = deployment_target
                rustflags = env_vars.get("RUSTFLAGS", "")
                if f"-mmacosx-version-min={deployment_target}" not in rustflags:
                    rustflags += f" -C link-arg=-mmacosx-version-min={deployment_target}"
                env_vars["RUSTFLAGS"] = rustflags.strip()

            subprocess.run(cmd, check=True, cwd=lib_path, env=env_vars)
            libs_to_lipo.append(f"{lib_path}/target/{rt}/{build_mode}/lib{lib_name}.a")

        print(f"{Y}Creating universal library for {lib_name} with lipo...{X}")
        universal_dir = f"{lib_path}/target/universal/{build_mode}"
        os.makedirs(universal_dir, exist_ok=True)
        universal_lib_path = f"{universal_dir}/lib{lib_name}.a"
        subprocess.run(["lipo", "-create"] + libs_to_lipo + ["-output", universal_lib_path], check=True)
        print(f"{G}{check} Universal library created at: {universal_lib_path}{X}")
    else: # Standard build
        rt = rust_target_list[0]
        cmd = ["cargo", "build", "--target", rt]
        if is_release: cmd.append("--release")
        if cargo_flags: cmd.extend(cargo_flags)

        if platform == "macos":
            deployment_target = os.environ.get("MACOSX_DEPLOYMENT_TARGET", "14.0")
            env_vars['MACOSX_DEPLOYMENT_TARGET'] = deployment_target
            rustflags = env_vars.get("RUSTFLAGS", "")
            if f"-mmacosx-version-min={deployment_target}" not in rustflags:
                rustflags += f" -C link-arg=-mmacosx-version-min={deployment_target}"
            env_vars["RUSTFLAGS"] = rustflags.strip()

        print(f"{Y}Running cargo command for {lib_name}: {' '.join(cmd)}{X}")
        subprocess.run(cmd, check=True, cwd=lib_path, env=env_vars)

    print(f"{G}{check} Rust library and bindings compiled successfully.{X}")

def build_rust_dependency(is_release, rust_target_list, platform):
    """
    Función principal para orquestar la compilación de la dependencia de Rust.
    Se ejecuta de forma imperativa antes de que SCons compile C++.
    """
    # 1. Asegurar que los targets de Rust estén instalados
    try:
        result = subprocess.run(["rustup", "target", "list", "--installed"], capture_output=True, text=True, check=True)
        installed_targets = result.stdout.splitlines()
        for rt in rust_target_list:
            if rt not in installed_targets:
                print(f"{Y}Installing Rust target {rt}...{X}")
                subprocess.run(["rustup", "target", "add", rt], check=True)
    except subprocess.CalledProcessError as e:
        print(f"{R}❌ Failed to check or install Rust target: {e}{X}")
        # Usamos sys.exit en lugar de Exit de SCons porque estamos fuera de su flujo principal
        import sys
        sys.exit(1)

    # 2. Compilar la librería
    _compile_rust_library("godot_dojo_core", "godot-dojo-core", is_release, rust_target_list, platform)