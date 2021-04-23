#include <lua.hpp>
#include <memory>
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

private:
    Script();
    std::string m_filename;
    std::vector<char> m_buffer;
};

class Engine {
public:
    Engine();
    ~Engine();

    [[nodiscard]] Result<Script::Pointer> load_script(const std::string& filename);
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
