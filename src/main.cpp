#include <cpplua.h>

#include <iostream>

int main(int argc, char** argv) {
    if (argc == 1) {
        std::cout << argv[0] << ": expected argument <filename>" << std::endl;
        return 1;
    }
    Lua::Engine engine;
    auto result = engine.load_script(argv[1]);
    if (result.error) {
        std::cout << "error: " << result.error << std::endl;
        return 1;
    }
    std::cout << "ok!" << std::endl;
    std::cout << (engine.is_loaded(argv[1]) ? "true" : "false") << std::endl;

    bool ok;
    auto result_map = engine.call_in_all_scripts("Test", {}, &ok);
    if (!ok) {
        std::cout << "not all returned OK" << std::endl;
        for (auto& pair : result_map) {
            if (pair.second.error) {
                std::cout << pair.first << ": " << pair.second.error;
                if (pair.second.value.has_value()) {
                    std::cout << ": " << std::any_cast<std::string>(pair.second.value);
                }
                std::cout << std::endl;
            } else {
                std::cout << pair.first << ": OK" << std::endl;
            }
        }
    } else {
        std::cout << "all OK!" << std::endl;
    }
}
