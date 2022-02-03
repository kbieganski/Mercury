#ifndef SCRIPTING_H_
#define SCRIPTING_H_
#define SOL_ALL_SAFETIES_ON 1
#define SOL_LUAJIT 1
#include <filesystem>
#include <sol/sol.hpp>
#include <string>

struct Scripting {
    struct Script {
        std::string path;
        std::filesystem::file_time_type last_modtime;
    };

    Scripting();
    void load_scripts(const std::string& path);

    sol::state lua;
    std::vector<Script> loaded;
};

#endif // SCRIPTING_H_
