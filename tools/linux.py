import os


def exists(env):
    return True


def generate(env):
    """
    Función de configuración para la plataforma Linux.
    Se encarga de añadir las flags de compilador y enlazador específicas para Linux.
    """
    # Esta lógica se ha movido desde SConstruct_bak

    # Habilitar excepciones y C++17
    # -Wno-template-id-cdtor suprime warnings de godot-cpp con GCC.
    env.Append(CXXFLAGS=['-fexceptions', '-std=c++17', '-Wno-template-id-cdtor'])

    # Enlazado de la librería Rust y sus dependencias
    is_release_build = env.get("target") == "template_release" or os.environ.get("FORCE_RUST_RELEASE", "0") == "1"
    rust_build_mode = "release" if is_release_build else "debug"
    rust_target = os.environ.get("CARGO_BUILD_TARGET", "x86_64-unknown-linux-gnu")
    rust_lib_path = f"godot-dojo-core/target/{rust_target}/{rust_build_mode}/libgodot_dojo_core.a"

    env.Append(_LIBFLAGS=['-Wl,--start-group', rust_lib_path, '-Wl,--end-group'])

    # Usar pkg-config para enlazar dbus-1, una dependencia de la librería Rust en Linux
    try:
        env.ParseConfig('pkg-config --cflags --libs dbus-1')
    except Exception:
        print("Advertencia: 'pkg-config --cflags --libs dbus-1' falló. Enlazando directamente con '-ldbus-1'.")
        env.Append(LIBS=['dbus-1'])