#include <unordered_set>
#include <string>

#include "modules.hpp"
#include "Logger.hpp"
#include "menus.hpp"
#include "utils.hpp"
#include <filesystem>
#include <fstream>

using namespace rdm;

const fs::path RDM_DATA_DIR = getDataDir();

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
    copyRDMLib();

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
                    LOG_CUSTOM(moduleName, fileKV.first << ":");
                    switch (fileKV.second.getDataType()) {
                        case FileDataType::Text:
                        LOG(fileKV.second.getContent());
                        break;
                        case FileDataType::RawData:
                        LOG("bytes");
                        break;
                        case FileDataType::Directory:
                        {
                            fs::path sourcePath = fileKV.second.getPath();
                            LOG("Copy of directory " << sourcePath.c_str() << ":");
                            for (auto& file : getDirectoryFilesRecursive(sourcePath)) {
                                fs::path extraPath = file.lexically_relative(sourcePath);
                                LOG(" - " << (fileKV.first / extraPath).c_str());
                            }
                        }
                        break;
                        default:
                        LOG_ERR("Received a file with an invalid data type: " << fileKV.first);
                        break;
                    }
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
                    FileDataType dataType = fileKV.second.getDataType();
                    fs::path file = fileKV.first;
                    
                    LOG_DEBUG("Processing: " << file);
                    LOG_DEBUG((fileKV.second.getDataType() == FileDataType::Directory ? "Directory: " : "File: ") << file.stem());
                    LOG_DEBUG("Destination: " << file.parent_path());

                    fs::create_directories(file.parent_path());
                    
                    if (cmd == "apply-soft") {
                        if (fileKV.second.getDataType() != FileDataType::Directory && fs::exists(file)) {
                            LOG_WARN("File " << file << " already present, skipping...");
                        } else {
                            if (fileKV.second.getDataType() != FileDataType::Directory) LOG_INFO("Creating file " << file << "...");
                            switch (dataType) {
                                case FileDataType::Text:
                                {
                                    std::ofstream handle = std::ofstream(file);
                                    handle << fileKV.second.getContent();
                                    handle.close();
                                }
                                break;
                                case FileDataType::RawData:
                                {
                                    fs::copy_file(fileKV.second.getPath(), file);
                                }
                                break;
                                case FileDataType::Directory:
                                {
                                    fs::path sourcePath = fileKV.second.getPath();
                                    fs::path destinationPath = file;
                                    for (auto& file : getDirectoryFilesRecursive(sourcePath)) {
                                        fs::path extraPath = file.lexically_relative(sourcePath);
                                        fs::path destinationFile = destinationPath / extraPath;

                                        if (fs::exists(destinationFile)) {
                                            LOG_WARN("File " << destinationFile << " already present, skipping...");
                                            continue;
                                        }
                                        LOG_INFO("Creating file " << destinationFile << "...");

                                        fs::create_directories(destinationFile.parent_path());
                                        fs::copy_file(sourcePath / extraPath, destinationFile);
                                    }
                                }
                                break;
                                default:
                                {
                                    LOG_ERR("Received a file with an invalid data type: " << file);
                                }
                                break;
                            }
                        }
                    } else {
                        if (fileKV.second.getDataType() != FileDataType::Directory) {
                            if (fs::exists(file)) { 
                                LOG_WARN("Replacing " << file << "...");
                                fs::remove(file);
                            } else {
                                LOG_INFO("Creating " << file << "...");
                            }
                        }

                        switch (dataType) {
                            case FileDataType::Text:
                            {
                                std::ofstream handle = std::ofstream(file);
                                handle << fileKV.second.getContent();
                                handle.close();
                            }
                            break;
                            case FileDataType::RawData:
                            {
                                fs::copy_file(fileKV.second.getPath(), file);
                            }
                            break;
                            case FileDataType::Directory:
                            {
                                fs::path sourcePath = fileKV.second.getPath();
                                fs::path destinationPath = file;
                                for (auto& file : getDirectoryFilesRecursive(sourcePath)) {
                                    fs::path extraPath = file.lexically_relative(sourcePath);
                                    fs::path destinationFile = destinationPath / extraPath;

                                    if (fs::exists(destinationFile)) {
                                        LOG_WARN("Replacing " << file << "...");
                                        fs::remove(destinationFile);
                                    } else {
                                        LOG_INFO("Creating " << file << "...");
                                    }

                                    fs::create_directories(destinationFile.parent_path());
                                    fs::copy_file(sourcePath / extraPath, destinationFile);
                                }
                            }
                            break;
                            default:
                            {
                                LOG_ERR("Received a file with an invalid data type: " << file);
                            }
                            break;
                        }
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
