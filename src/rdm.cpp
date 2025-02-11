#include <unordered_set>
#include <string>

#include "modules.hpp"
#include "Logger.hpp"
#include "menus.hpp"


using namespace rdm;

struct ModulesAndFlags {
    std::unordered_set<std::string> modules;
    std::unordered_set<std::string> flags;
};

ModulesAndFlags parseModulesAndFlags(char* argv[], int count) {
    ModulesAndFlags maf;
    if (count == 0) return maf;

    std::vector<std::string> args;
    args.reserve(count);
    for (int i = 0; i < count; ++i) {
        args.emplace_back(argv[i]);
    }

    int currentArg = 0;

    while (currentArg < count) {
        auto& arg = args.at(currentArg++);
        if (arg.starts_with("-")) {
            if (arg == "-f" || arg == "--flags") break;
            continue;
        }
        maf.modules.insert(arg);
    }

    while (currentArg < count) {
        auto& arg = args.at(currentArg++);
        if (arg.starts_with("-")) continue;
        maf.flags.insert(arg);
    }

    return maf;
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        menus::printMainHelp();
        return EXIT_SUCCESS;
    }

    std::string cmd = argv[1];

    if (cmd == "preview") {
        if (argc < 3) {
            LOG_ERR("Not enough arguments supplied");
            menus::printPreviewHelp();
            return EXIT_FAILURE;
        }

        auto modulesAndFlags = parseModulesAndFlags(argv + 2, argc - 2);

        if (modulesAndFlags.modules.empty()) {
            LOG_WARN("No modules specified, defaulting to all modules");
        } else {
            #ifdef _DEBUG
            LOG_DEBUG("Parsed modules:");
            for (auto& module : modulesAndFlags.modules) {
                LOG(module);
            }
            #endif
        }

        if (modulesAndFlags.flags.empty()) {
            LOG_WARN("No flags specified");
        } else {
            #ifdef _DEBUG
            LOG_DEBUG("Parsed flags:");
            for (auto& flag : modulesAndFlags.flags) {
                LOG(flag);
            }
            #endif
        }


        ModuleManager moduleManager = ModuleManager("home", modulesAndFlags.flags);

        for (auto& moduleName : modulesAndFlags.modules) {
            if (!moduleManager.getModules().contains(moduleName)) {
                LOG_ERR("Couldn't find the module '" << moduleName << "', skipping...");
            }
        }

        for (auto& [moduleName, module]: moduleManager.getModules()) {
            if (modulesAndFlags.modules.empty() || modulesAndFlags.modules.contains(module.getName())) {
                FileContentMap generatedFiles = module.getGeneratedFiles();

                LOG_SEP();
                if (generatedFiles.empty()) {
                    if (module.getExitCode() == LUA_OK) {
                        LOG_WARN("The module '" << module.getName() << "' was found but returned no files, exit code: " << module.getExitCode());
                        continue;
                    } else {
                        LOG_ERR("The module '" << module.getName() << "' was found but had errors: " << module.getErrorString());
                        continue;
                    }
                }

                LOG_INFO("Module: " << moduleName);
                for (auto& fileKV : generatedFiles) {
                    LOG_SEP();
                    LOG(fileKV.first << ":");
                    LOG(fileKV.second);
                }
            }
        }
        LOG_SEP();
    } else if (cmd == "apply") {
        LOG_WARN("Unimplemented");
        menus::printApplyHelp();
        return EXIT_FAILURE;
    } else if (cmd == "init") {
        LOG_WARN("Unimplemented");
        menus::printInitHelp();
        return EXIT_FAILURE;
    } else {
        LOG("Unrecognized command '" << cmd << "'");
        menus::printMainHelp();
    }
    
    return EXIT_SUCCESS;
}
