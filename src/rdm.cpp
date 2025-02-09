#include "Logger.hpp"
#include "modules.hpp"

using namespace rdm;

void printHelpMenu() {
    LOG("Usage: rdm <command> [args...]");
    LOG(" apply         Runs the scripts");
    LOG(" init          Initializes rdm with a git repository");
    LOG(" preview       Preview a specific module output");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printHelpMenu();
        return EXIT_SUCCESS;
    }

    std::string cmd = argv[1];

    if (cmd == "preview") {
        if (argc < 3) {
            LOG_ERR("Not enough arguments supplied");
            LOG("Usage: rdm preview <module_name>");
            LOG(" <module_name>      rdm script with no prefix or extension (e.g. rdm-hyprland.lua -> hyprland)");
            return EXIT_FAILURE;
        }

        ModuleManager moduleManager = ModuleManager("home");
        std::string moduleName = argv[2];

        for (auto& module: moduleManager.getModules()) {
            if (module.getName() == moduleName) {
                FileContentMap generatedFiles = module.getGeneratedFiles();

                if (generatedFiles.empty()) {
                    LOG("The module '" << moduleName << "' was found but returned no files");
                    return EXIT_SUCCESS;
                }

                LOG_SEP();
                for (auto& fileKV : generatedFiles) {
                    LOG(fileKV.first << ":");
                    LOG(fileKV.second);
                    LOG_SEP();
                }

                return EXIT_SUCCESS;
            }
        }

        LOG_ERR("Couldn't find module '" << moduleName << "'");
    } else if (cmd == "apply") {
        LOG_WARN("Unimplemented");
        return EXIT_FAILURE;
    } else if (cmd == "init") {
        LOG_WARN("Unimplemented");
        return EXIT_FAILURE;
    } else {
        LOG("Unrecognized command '" << cmd << "'");
        printHelpMenu();
    }
    
    return EXIT_SUCCESS;
}
