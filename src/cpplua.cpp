#include "cpplua.h"

// https://www.lua.org/manual/5.3/contents.html

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
    auto result = script->load();
    if (result.failed()) {
        return { result.error, nullptr };
    }
    auto lock = acquire_unique_lock();
    m_scripts.push_back(script);
    return { {}, script };
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

std::unordered_map<std::string, Lua::Result<std::any>> Lua::Engine::call_in_all_scripts(const std::string& function_name, std::initializer_list<std::any> args, bool* all_ok_ptr, Lua::CallFlags flags) {
    std::unordered_map<std::string, Lua::Result<std::any>> map;
    auto lock = acquire_shared_lock();
    if (all_ok_ptr) {
        *all_ok_ptr = true;
    }
    for (auto& script : m_scripts) {
        if (flags & CallFlags::IgnoreNotExists
            && !script->has_function_with_name(function_name)) {
            continue;
        }
        auto res = script->call_function(function_name, args);
        if (all_ok_ptr && res.failed()) {
            *all_ok_ptr = false;
        }
        map[script->filename()] = std::move(res);
    }
    return map;
}

void Lua::Engine::register_global_function(const std::string& name, lua_CFunction Fn) {
    auto lock = acquire_unique_lock();
    for (auto& script : m_scripts) {
        auto L = script->state();
        lua_register(L, name.c_str(), Fn);
    }
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

Lua::Script::~Script() {
    lua_close(m_state);
}

void Lua::Script::set_filename(const std::string& filename) {
    m_filename = filename;
}

void Lua::Script::set_buffer(std::vector<char>&& buffer) {
    std::swap(m_buffer, buffer);
}

Lua::Result<void> Lua::Script::load() {
    if (m_loaded) {
        return { "already loaded" };
    } else if (m_buffer.empty()) {
        return { "empty buffer" };
    } else if (m_filename.empty()) {
        return { "empty filename" };
    }
    int ret = luaL_loadstring(m_state, m_buffer.data());
    if (ret == LUA_OK) {
        ret = lua_pcall(m_state, 0, LUA_MULTRET, 0);
        if (ret == LUA_OK) {
            m_loaded = true;
            return {};
        } else if (ret == LUA_ERRRUN) { // runtime error
            const char* msg = lua_tostring(m_state, -1);
            std::string error_message = msg ? msg : "(null)";
            return { "runtime error: " + error_message };
        } else {
            const char* msg = lua_tostring(m_state, -1);
            std::string error_message = msg ? msg : "(null)";
            return { "error running lua file: " + error_message };
        }
    } else {
        const char* msg = lua_tostring(m_state, -1);
        std::string error_message = msg ? msg : "(null)";
        return { "could not load lua (syntax error?): " + error_message };
    }
}

Lua::Result<std::any> Lua::Script::call_function(const std::string& str, std::initializer_list<std::any> args) {
    if (!m_loaded) {
        return { "lua chunk was never loaded", {} };
    }
    int stack_top_before = lua_gettop(m_state);
    // get & push the function
    int type = lua_getglobal(m_state, str.c_str());
    if (type == LUA_TNIL) {
        return { "no such function \"" + str + "\"", {} };
    }
    if (type != LUA_TFUNCTION) {
        return { "attempt to call object that isn't a function", {} };
    }
    // push all arguments
    for (auto& arg : args) {
        if (arg.has_value()) {
            if (arg.type() == typeid(std::string)) {
                lua_pushstring(m_state, std::any_cast<std::string>(arg).c_str());
            } else if (arg.type() == typeid(lua_Integer)) {
                lua_pushinteger(m_state, std::any_cast<lua_Integer>(arg));
            } else if (arg.type() == typeid(lua_Number)) {
                lua_pushnumber(m_state, std::any_cast<lua_Number>(arg));
            } else if (arg.type() == typeid(bool)) {
                lua_pushboolean(m_state, std::any_cast<bool>(arg));
            } else {
                // reset stack to before
                lua_settop(m_state, stack_top_before);
                return { "tried to push a value which was not string, integer, number or bool", {} };
            }
        }
    }
    int ret = lua_pcall(m_state, int(args.size()), 1, 0);
    Defer defer_resetting_stack([&] { lua_settop(m_state, stack_top_before); });
    switch (ret) {
    case LUA_OK: {
        std::any result;
        int res_type = lua_type(m_state, -1);
        switch (res_type) {
        case LUA_TNIL:
            break;
        case LUA_TNUMBER:
            result = lua_Number(lua_tonumber(m_state, -1));
            break;
        case LUA_TBOOLEAN:
            result = bool(lua_toboolean(m_state, -1));
            break;
        case LUA_TSTRING: {
            const char* msg = lua_tostring(m_state, -1);
            result = std::string(msg ? msg : "(null)");
            break;
        }
        default:
            result = (void*)lua_topointer(m_state, -1);
        }
        return { {}, result };
    }
    case LUA_ERRRUN: { // runtime error
        const char* msg = lua_tostring(m_state, -1);
        std::string error_message = msg ? msg : "(null)";
        return { "runtime error: " + error_message, {} };
    }
    case LUA_ERRERR:
        // message handler error, can't happen since we don't specify one
        return { "message handler error, should never happen", {} };
    default:
        return { "unknown / generic error in pcall", {} };
    }
}

bool Lua::Script::has_function_with_name(const std::string& name) {
    int type = lua_getglobal(m_state, name.c_str());
    if (type != LUA_TFUNCTION) {
        return false;
    }
    return true;
}

Lua::Script::Script()
    : m_state(luaL_newstate()) {
    luaL_openlibs(m_state);
}
