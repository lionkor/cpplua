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
    } else {
        std::cout << "ok!" << std::endl;
    }
}
