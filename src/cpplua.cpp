#include "cpplua.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>

Lua::Engine::Engine() = default;

Lua::Engine::~Engine() = default;

Lua::Result<Lua::Script::Pointer> Lua::Engine::load_script(const std::string& filename) {
    if (!std::filesystem::exists(filename)) {
        return { "file does not exist", nullptr };
    } else if (!std::filesystem::is_regular_file(filename)) {
        return { "given path is not a file", nullptr };
    }
    std::filesystem::path fullpath = std::filesystem::absolute(filename);
    auto script = Lua::Script::make();
    script->set_filename(fullpath);
    // set up the buffer for reading into
    std::vector<char> buffer;
    buffer.resize(std::filesystem::file_size(fullpath));
    // let's read the entire file in one syscall, this way we don't have to bother
    // with c++'s streams and the unnecessary flexibility they provide which we don't need
    FILE* file = fopen(fullpath.c_str(), "rb");
    if (!file) {
        return { "could not open file", nullptr };
    }
    // RAII defer to close the file on all early returns
    Defer defer_close_file([&file] { fclose(file); });
    size_t n = fread(buffer.data(), sizeof(char), buffer.size(), file);
    if (n != buffer.size()) {
        return { "error reading file", nullptr };
    }
    script->set_buffer(std::move(buffer));
    auto lock = acquire_unique_lock();
    m_scripts.push_back(script);
    return { nullptr, script };
}

bool Lua::Engine::is_loaded(const std::string& filename) {
    auto script = get_script_by_name(filename);
    if (script) {
        return true;
    } else {
        return false;
    }
}

void Lua::Engine::unload_script(const Lua::Script::Pointer& script) {
    unload_script(script->filename());
}

void Lua::Engine::unload_script(const std::string& filename) {
    auto lock = acquire_unique_lock();
    auto iter = unsafe_get_script_by_name(filename);
}

Lua::Script::Pointer Lua::Engine::get_script_by_name(const std::string& filename) {
    auto lock = acquire_shared_lock();
    return unsafe_get_script_by_name(filename);
}

Lua::Script::Pointer Lua::Engine::unsafe_get_script_by_name(const std::string& filename) {
    std::filesystem::path fullpath = std::filesystem::absolute(filename);
    auto iter = std::find_if(m_scripts.begin(), m_scripts.end(),
        [&fullpath](const Script::Pointer& ptr) -> bool {
            return ptr->filename() == fullpath;
        });
    if (iter == m_scripts.end()) {
        return nullptr;
    } else {
        return *iter;
    }
}

Lua::Script::Script()
    : m_state(luaL_newstate()) {
}
