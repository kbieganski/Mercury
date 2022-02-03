#include "scripting.h"
#include <filesystem>

Scripting::Scripting() {
    lua.open_libraries(sol::lib::base);
    lua.open_libraries(sol::lib::math);
    lua["time"] = []() { return std::time(nullptr); };
}

void Scripting::load_scripts(const std::string& path) {
    loaded.clear();
    for (auto& p :
         std::filesystem::recursive_directory_iterator("assets/scripts")) {
        loaded.push_back({p.path(), std::filesystem::last_write_time(p)});
    }
    std::sort(loaded.begin(), loaded.end(), [](auto& a, auto& b) { return a.path < b.path; });
    for (const auto& script : loaded)
        lua.script_file(script.path);
}
