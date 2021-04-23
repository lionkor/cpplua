#include <lua.hpp>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

namespace Lua {

template<typename T>
struct Result {
    const char* error;
    T value;
};

class Script {
public:
    using Pointer = std::shared_ptr<Script>;
    static Pointer make() { return Pointer { new Script }; }

    void set_filename(const std::string& filename) { m_filename = filename; }
    void set_buffer(std::vector<char>&& buffer) { std::swap(m_buffer, buffer); }
    const std::string& filename() const { return m_filename; }
    const std::vector<char>& buffer() const { return m_buffer; }
    std::vector<char>& buffer() { return m_buffer; }
    lua_State* state() { return m_state; }

private:
    Script();
    std::string m_filename;
    std::vector<char> m_buffer;

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
