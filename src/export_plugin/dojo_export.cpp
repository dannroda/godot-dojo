#include "export_plugin/dojo_export.h"

#include "debug_macros.h"
#include "godot_cpp/classes/engine.hpp"

void DojoExportPlugin::_bind_methods() {
    // No necesitamos bind methods para este caso
}

DojoExportPlugin::DojoExportPlugin() {
    LOG_INFO("📦 DojoExportPlugin inicializado");
    export_path = "";

}

DojoExportPlugin::~DojoExportPlugin() {
    LOG_INFO("📦 DojoExportPlugin destruido");
}

String DojoExportPlugin::_get_name() const {
    return "Godot Dojo Windows Plugin";
}

void DojoExportPlugin::_export_begin(const PackedStringArray &features, bool is_debug, const String &path, int flags) {
    export_path = path.get_base_dir();

    LOG_INFO("🚀 Iniciando exportación desde consola/editor:");
    LOG_INFO("🖥️  Modo headless detectado:", Variant(Engine::get_singleton()->is_editor_hint()));
    LOG_INFO("📁 Path:", path);
    LOG_INFO("🏗️  Export dir:", export_path);

    // LOG_INFO("Features:", features);
    LOG_INFO("Debug:", Variant(is_debug));
    LOG_INFO("Path:", path);
    LOG_INFO("Export dir:", export_path);
    
    // Copiar archivos específicos según la plataforma
    if (features.has("linux")) {
        copy_files_from_folder("addons/dojo/libs/linux", "");
        copy_file_to_export("addons/dojo/config/dojo_config.json", "dojo_config.json");
    } else if (features.has("windows")) {
        copy_files_from_folder("addons/dojo/libs/windows", "");
        copy_file_to_export("addons/dojo/config/dojo_config.json", "dojo_config.json");
    } else if (features.has("macos")) {
        copy_files_from_folder("addons/dojo/libs/macos", "");
        copy_file_to_export("addons/dojo/config/dojo_config.json", "dojo_config.json");
    }
}

void DojoExportPlugin::_export_file(const String &path, const String &type, const PackedStringArray &features) {
    // Aquí puedes interceptar archivos específicos si necesitas modificarlos
    // Por ejemplo, si quieres copiar archivos .dll específicos:
    
    if (path.ends_with(".dll") && path.begins_with("addons/dojo/")) {
        LOG_INFO("📦 Procesando archivo DLL: ", path);
        // El archivo se incluirá automáticamente en la exportación
    }
}

void DojoExportPlugin::_export_end() {
    LOG_INFO("✅ Exportación completada");
}

bool DojoExportPlugin::copy_file_to_export(const String &source_path, const String &dest_relative_path) {
    if (export_path.is_empty()) {
        LOG_INFO("❌ Error: export_path no está definido");
        return false;
    }
    
    // Verificar que el archivo fuente existe
    Ref<FileAccess> source_file = FileAccess::open(source_path, FileAccess::READ);
    if (source_file.is_null()) {
        LOG_INFO("❌ No se puede leer el archivo fuente: ", source_path);
        return false;
    }
    
    // Leer contenido del archivo
    PackedByteArray file_data = source_file->get_buffer(source_file->get_length());
    source_file->close();
    
    // Crear ruta de destino
    String dest_path = export_path.path_join(dest_relative_path);
    
    // Asegurar que el directorio existe
    String dest_dir = dest_path.get_base_dir();
    if (!ensure_directory_exists(dest_dir)) {
        return false;
    }
    
    // Escribir archivo en destino
    Ref<FileAccess> dest_file = FileAccess::open(dest_path, FileAccess::WRITE);
    if (dest_file.is_null()) {
        LOG_INFO("❌ No se puede escribir el archivo destino: ", dest_path);
        return false;
    }
    
    dest_file->store_buffer(file_data);
    dest_file->close();
    
    LOG_INFO("✅ Archivo copiado: ", source_path, " -> ", dest_path);
    return true;
}

bool DojoExportPlugin::ensure_directory_exists(const String &dir_path) {
    Ref<DirAccess> dir = DirAccess::open(".");
    if (dir.is_null()) {
        LOG_INFO("❌ Error accediendo al sistema de archivos");
        return false;
    }
    
    Error err = dir->make_dir_recursive(dir_path);
    if (err != OK && err != ERR_ALREADY_EXISTS) {
        LOG_INFO("❌ Error creando directorio: ", dir_path);
        return false;
    }
    
    return true;
}

void DojoExportPlugin::copy_files_from_folder(const String &source_folder, const String &dest_folder) {
    Ref<DirAccess> dir = DirAccess::open(source_folder);
    if (dir.is_null()) {
        LOG_INFO("❌ No se puede acceder a la carpeta: ", source_folder);
        return;
    }
    
    dir->list_dir_begin();
    String file_name = dir->get_next();
    
    while (!file_name.is_empty()) {
        if (!dir->current_is_dir()) {
            String source_file_path = source_folder.path_join(file_name);
            String dest_file_path = dest_folder.is_empty() ? file_name : dest_folder.path_join(file_name);
            
            copy_file_to_export(source_file_path, dest_file_path);
        }
        file_name = dir->get_next();
    }
    
    dir->list_dir_end();
}