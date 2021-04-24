#include <cpplua.h>

#include <iostream>

extern "C" int lua_ping(lua_State* state) {
    std::cout << state << ": PONG :)" << std::endl;
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 1) {
        std::cout << argv[0] << ": expected argument <filename>" << std::endl;
        return 1;
    }
    Lua::Engine engine;
    auto result = engine.load_script(argv[1]);
    if (result.failed()) {
        std::cout << "error: " << result.error << std::endl;
        return 1;
    }
    std::cout << "ok!" << std::endl;
    std::cout << (engine.is_loaded(argv[1]) ? "true" : "false") << std::endl;

    bool ok;
    engine.register_global_function("ping", lua_ping);
    auto result_map = engine.call_in_all_scripts("Test", {}, &ok, Lua::CallFlags::IgnoreNotExists);
    if (!ok) {
        std::cout << "not all returned OK" << std::endl;
        for (auto& pair : result_map) {
            if (pair.second.failed()) {
                std::cout << pair.first << ": " << pair.second.error << std::endl;
            } else {
                std::cout << pair.first << ": OK" << std::endl;
            }
        }
    } else {
        std::cout << "all OK!" << std::endl;
    }
}
