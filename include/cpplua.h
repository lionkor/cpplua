#include <any>
#include <atomic>
#include <initializer_list>
#include <lua.hpp>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Lua {

template<typename T>
struct Result {
    bool ok() { return error.empty(); }
    bool failed() { return !error.empty(); }
    std::string error;
    T value;
};

template<>
struct Result<void> {
    bool ok() { return error.empty(); }
    bool failed() { return !error.empty(); }
    std::string error {};
};

class Script {
public:
    ~Script();

    using Pointer = std::shared_ptr<Script>;
    static Pointer make() { return Pointer { new Script }; }

    // sets the filename - needs to happen before using the script
    void set_filename(const std::string& filename) { m_filename = filename; }
    // sets the lua script itself - needs to happen before using the script
    void set_buffer(std::vector<char>&& buffer) { std::swap(m_buffer, buffer); }
    const std::string& filename() const { return m_filename; }
    // gets the buffer. changing it does not affect the execution, so don't.
    const std::vector<char>& buffer() const { return m_buffer; }
    std::vector<char>& buffer() { return m_buffer; }
    // lua state for this file
    lua_State* state() { return m_state; }
    // loads & initializes the buffer.
    [[nodiscard]] Result<void> load();
    // calls the function, returns an error if it failed, or the function's return value
    // if it succeeded.
    // if the result is not a primitive like string, integer or number, it will be returned as void*.
    [[nodiscard]] Result<std::any> call_function(const std::string& str, std::initializer_list<std::any> args);

private:
    Script();
    std::string m_filename;
    std::vector<char> m_buffer;
    std::atomic_bool m_loaded { false };

    lua_State* m_state;
};

class Engine {
public:
    Engine();
    ~Engine();

    [[nodiscard]] Result<Script::Pointer> load_script(const std::string& filename);
    bool is_loaded(const std::string& filename);
    void unload_script(const Script::Pointer& script);
    void unload_script(const std::string& filename);
    Script::Pointer get_script_by_name(const std::string& filename);

    [[nodiscard]] std::unordered_map<std::string, Result<std::any>> call_in_all_scripts(const std::string& function_name, std::initializer_list<std::any> args, bool* all_ok);

private:
    std::shared_lock<std::shared_mutex> acquire_shared_lock() { return std::shared_lock<std::shared_mutex> { m_scripts_mutex }; }
    std::unique_lock<std::shared_mutex> acquire_unique_lock() { return std::unique_lock<std::shared_mutex> { m_scripts_mutex }; }

    // unsafe_* functions shall never lock any mutexes
    Script::Pointer unsafe_get_script_by_name(const std::string& filename);

    std::vector<Script::Pointer> m_scripts;
    std::shared_mutex m_scripts_mutex;
};

template<typename Fn>
struct Defer {
    Defer(Fn fn)
        : fn(fn) { }
    Defer(const Defer&) = delete;
    Defer(Defer&&) = delete;
    ~Defer() {
        fn();
    }
    Fn fn;
};

}
