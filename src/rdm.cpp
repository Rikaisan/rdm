#include "Logger.hpp"
#include "modules.hpp"
#include "menus.hpp"

using namespace rdm;

int main(int argc, char **argv) {
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
