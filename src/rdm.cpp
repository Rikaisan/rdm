#include <unordered_set>
#include <string>

#include "modules.hpp"
#include "Logger.hpp"
#include "menus.hpp"
#include "utils.hpp"
#include <filesystem>
#include <fstream>

using namespace rdm;

const std::filesystem::path RDM_DATA_DIR = getDataDir();

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
    ensureDataDirExists();

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

        // To let users choose to not run logic if previewing
        modulesAndFlags.flags.insert("preview");

        ModuleManager moduleManager = ModuleManager(RDM_DATA_DIR / "home", getUserHome(), modulesAndFlags);

        for (auto& moduleName : modulesAndFlags.modules) {
            if (!moduleManager.getModules().contains(moduleName)) {
                LOG_ERR("Couldn't find the module '" << moduleName << "', skipping...");
            }
        }

        for (auto& [moduleName, module]: moduleManager.getModules()) {
            if (moduleManager.shouldProcessAllModules() || moduleManager.shouldProcessModule(module.getName())) {
                FileContentMap generatedFiles = module.getGeneratedFiles();

                LOG_SEP();
                if (generatedFiles.empty()) {
                    if (module.getExitCode() == LUA_OK) {
                        LOG_WARN("The module '" << module.getName() << "' was found but returned no files.");
                    } else {
                        LOG_ERR("The module '" << module.getName() << "' was found but had errors [" << module.getExitCode() << "]: " << module.getErrorString());
                    }
                    continue;
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
    } else if (cmd == "apply" || cmd == "apply-soft") {
        if (argc < 3) {
            LOG_ERR("Not enough arguments supplied");
            menus::printApplyHelp();
            return EXIT_FAILURE;
        }

        auto modulesAndFlags = parseModulesAndFlags(argv + 2, argc - 2);

        if (modulesAndFlags.modules.empty()) {
            LOG_INFO("No modules specified, defaulting to all modules");
        } else {
            #ifdef _DEBUG
            LOG_DEBUG("Parsed modules:");
            for (auto& module : modulesAndFlags.modules) {
                LOG(module);
            }
            #endif
        }

        if (modulesAndFlags.flags.empty()) {
            LOG_DEBUG("No flags specified");
        } else {
            #ifdef _DEBUG
            LOG_DEBUG("Parsed flags:");
            for (auto& flag : modulesAndFlags.flags) {
                LOG(flag);
            }
            #endif
        }


        ModuleManager moduleManager = ModuleManager(RDM_DATA_DIR / "home", getUserHome(), modulesAndFlags);

        // Log that some modules were passed by the user that do not exist
        for (auto& moduleName : modulesAndFlags.modules) {
            if (!moduleManager.getModules().contains(moduleName)) {
                LOG_ERR("Couldn't find the module '" << moduleName << "', skipping...");
            }
        }

        for (auto& [moduleName, module]: moduleManager.getModules()) {
            if (moduleManager.shouldProcessAllModules() || moduleManager.shouldProcessModule(module.getName())) {
                FileContentMap generatedFiles = module.getGeneratedFiles();

                if (generatedFiles.empty()) {
                    if (module.getExitCode() == LUA_OK) {
                        LOG_DEBUG("The module '" << module.getName() << "' was found but returned no files.");
                    } else {
                        LOG_ERR("The module '" << module.getName() << "' was found but had errors [" << module.getExitCode() << "]: " << module.getErrorString());
                    }
                    continue;
                }

                // Do something with the module files
                for (auto& fileKV : generatedFiles) {
                    std::filesystem::path file = fileKV.first;
                    std::string fileContent = fileKV.second;

                    LOG_DEBUG("Processing: " << file);
                    LOG_DEBUG("File: " << file.stem());
                    LOG_DEBUG("Destination: " << file.parent_path());

                    std::filesystem::create_directories(file.parent_path());

                    if (cmd == "apply-soft") {
                        if (std::filesystem::exists(file)) {
                            LOG_WARN("File " << file << " already present, skipping...");
                        } else {
                            LOG_INFO("Creating file " << file << "...");
                            std::ofstream handle = std::ofstream(file);
                            handle << fileContent;
                            handle.close();
                        }
                    } else {
                        if (std::filesystem::exists(file)) { 
                            LOG_WARN("Replacing file " << file << "...");
                            std::filesystem::remove(file);
                        } else {
                            LOG_INFO("Creating file " << file << "...");
                        }
                        std::ofstream handle = std::ofstream(file);
                        handle << fileContent;
                        handle.close();
                    }
                }
            }
        }
    } else if (cmd == "init") {
        LOG_ERR("Unimplemented");
        menus::printInitHelp();
        return EXIT_FAILURE;
    } else {
        LOG("Unrecognized command '" << cmd << "'");
        menus::printMainHelp();
    }
    
    return EXIT_SUCCESS;
}
