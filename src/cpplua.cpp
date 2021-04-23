#include "cpplua.h"

#include <cstdio>
#include <filesystem>

Lua::Engine::Engine() {
}

Lua::Engine::~Engine() {
}

Lua::Result<Lua::Script::Pointer> Lua::Engine::load_script(const std::string& filename) {
    if (!std::filesystem::exists(filename)) {
        return { "file does not exist", nullptr };
    } else if (!std::filesystem::is_regular_file(filename)) {
        return { "given path is not a file", nullptr };
    }
    auto script = Lua::Script::make();
    script->set_filename(filename);
    // set up the buffer for reading into
    std::vector<char> buffer;
    buffer.resize(std::filesystem::file_size(filename));
    // let's read the entire file in one syscall, this way we don't have to bother
    // with c++'s streams and the unnecessary flexibility they provide which we don't need
    FILE* file = fopen(filename.c_str(), "rb");
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
    return { nullptr, script };
}

Lua::Script::Script() = default;
