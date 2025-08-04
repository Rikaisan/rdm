#include <unordered_set>
#include <string>

#include "modules.hpp"
#include "Logger.hpp"
#include "menus.hpp"
#include "utils.hpp"
#include <filesystem>
#include <fstream>
#include "git2.h"

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
    if(!copyRDMLib()) {
        LOG_DEBUG("Couldn't copy RDM lib.");
    }
    
    if (argc == 1) {
        menus::printMainHelp();
        return EXIT_SUCCESS;
    }

    std::string cmd = argv[1];

    
    if (cmd == "help") {
        if (argc < 3) {
            menus::printHelpHelp();
        } else {
            std::string page = argv[2];
            if (page == "preview") {
                menus::printPreviewHelp();
            } else if (page == "apply" || page == "apply-soft") {
                menus::printApplyHelp();
            } else if (page == "init") {
                menus::printInitHelp();
            } else if (page == "clone") {
                menus::printCloneHelp();
            } else if (page == "dir") {
                menus::printDirHelp();
            } else {
                menus::printHelpHelp();
            }
        }
    } else if (cmd == "preview" || cmd == "apply" || cmd == "apply-soft") {
        if (!fs::exists(RDM_DATA_DIR) || fs::is_empty(RDM_DATA_DIR)) {
            LOG_ERR("RDM data dir is empty or doesn't exist, run either 'rdm init' or 'rdm clone' to initialize it before running this command");
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
        if (cmd == "preview") modulesAndFlags.flags.insert("preview");

        ModuleManager moduleManager = ModuleManager(RDM_DATA_DIR / "home", getUserHome(), modulesAndFlags);

        for (auto& moduleName : modulesAndFlags.modules) {
            if (!moduleManager.getModules().contains(moduleName)) {
                LOG_ERR("Couldn't find the module '" << moduleName << "', skipping...");
            }
        }

        for (auto& [moduleName, module]: moduleManager.getModules()) {
            if (moduleManager.shouldProcessModule(module.getName())) {
                FileContentMap generatedFiles = module.getGeneratedFiles();

                if (cmd == "preview") LOG_SEP();
                if (generatedFiles.empty()) {
                    if (module.getExitCode() == LUA_OK) {
                        LOG_DEBUG("The module '" << module.getName() << "' was found but returned no files.");
                        module.runDelayed();
                    } else {
                        LOG_ERR("The module '" << module.getName() << "' was found but had errors [" << module.getExitCode() << "]: " << module.getErrorString());
                    }
                    continue;
                } else {
                    LOG_INFO("Processing module: " << moduleName);
                }

                int skippedFiles = 0;
                for (auto& fileKV : generatedFiles) {
                    FileDataType dataType = fileKV.second.getDataType();
                    fs::path file = fileKV.first;
                    if (cmd == "preview") {
                        LOG_SEP();
                        LOG_CUSTOM(moduleName, file << ":");
                    }

                    LOG_DEBUG("Processing: " << file);
                    LOG_DEBUG((dataType == FileDataType::Directory ? "Directory: " : "File: ") << file.stem());
                    LOG_DEBUG("Destination: " << file.parent_path());
                    
                    if (cmd != "preview") {
                        fs::create_directories(file.parent_path());

                        if (fileKV.second.getDataType() != FileDataType::Directory) {
                            if (fs::exists(file)) {
                                if (cmd == "apply-soft") {
                                    skippedFiles++;
                                    continue;
                                }
                                LOG_WARN("Replacing " << file << "...");
                                fs::remove(file);
                            } else {
                                LOG_INFO("Creating " << file << "...");
                            }
                        }
                    }

                    switch (dataType) {
                        case FileDataType::Text:
                            if (cmd == "preview") {
                                LOG(fileKV.second.getContent());
                            } else {
                                std::ofstream handle = std::ofstream(file);
                                handle << fileKV.second.getContent();
                                handle.close();
                            }
                            break;
                        case FileDataType::RawData:
                            if (cmd == "preview") {
                                LOG("Raw Copy");
                            } else {
                                fs::copy_file(fileKV.second.getPath(), file);
                            }
                            break;
                        case FileDataType::Directory:
                            if (cmd == "preview") {
                                fs::path sourcePath = fileKV.second.getPath();
                                LOG_CUSTOM(moduleName, "Copy of directory " << sourcePath.c_str() << ":");
                                auto files = getDirectoryFilesRecursive(sourcePath);
                                size_t fileCount = files.size();
                                size_t filesToPrint = fileCount >= 16 ? 16 : fileCount;
                                for (size_t i{0}; i < filesToPrint; ++i) {
                                    fs::path extraPath = files.at(i).lexically_relative(sourcePath);
                                    LOG(" - " << (fileKV.first / extraPath).c_str());
                                }
                                if (fileCount > filesToPrint) {
                                    LOG(" + " << fileCount - filesToPrint << " more...");
                                }
                            } else {
                                fs::path sourcePath = fileKV.second.getPath();
                                fs::path destinationPath = file;
                                for (auto& file : getDirectoryFilesRecursive(sourcePath)) {
                                    fs::path extraPath = file.lexically_relative(sourcePath);
                                    fs::path destinationFile = destinationPath / extraPath;
                                    fs::path sourceFile = sourcePath / extraPath;

                                    if (fs::exists(fs::symlink_status(destinationFile))) {
                                        if (cmd == "apply-soft") {
                                            skippedFiles++;
                                            continue;
                                        }
                                        LOG_WARN("Replacing " << destinationFile << "...");
                                        fs::remove(destinationFile);
                                    } else {
                                        LOG_INFO("Creating " << destinationFile << "...");
                                    }

                                    fs::create_directories(destinationFile.parent_path());

                                    if (fs::is_symlink(sourceFile)) {
                                        fs::copy_symlink(sourceFile, destinationFile);
                                    } else {
                                        fs::copy_file(sourceFile, destinationFile);
                                    }
                                }
                            }
                            break;
                        default:
                           LOG_ERR("Received a file with an invalid data type: " << fileKV.first);
                    }
                }
                if (skippedFiles > 0) LOG_INFO("Skipped " << skippedFiles << " files that were already present");
                module.runDelayed();
            }
        }
        if (cmd == "preview") LOG_SEP();
    } else if (cmd == "init") {
        ensureDataDirExists(true);
        LOG("Initialized rdm at " << RDM_DATA_DIR.c_str());
        if (argc > 2) {
            LOG_WARN("Detected an extra argument, if you meant to initialize from a repository, use 'rdm clone <url>' instead");
        }
    } else if (cmd == "clone") {
        if (argc < 3) {
            menus::printCloneHelp();
            return EXIT_FAILURE;
        }

        if (argc > 3) {
            std::string extraArg = argv[3];
            if (extraArg == "--replace") {
                LOG_WARN("Deleting all files in " << RDM_DATA_DIR.c_str() << "...");
                fs::remove_all(RDM_DATA_DIR);
            }
        }

        ensureDataDirExists(false);

        std::string repoURL = argv[2];
        git_libgit2_init();

        git_clone_options options = GIT_CLONE_OPTIONS_INIT;
        git_repository* repo = nullptr;

        LOG_INFO("Attempting to clone repository into " << RDM_DATA_DIR.c_str() << "...");
        int cloneResult = git_clone(&repo, repoURL.c_str(), RDM_DATA_DIR.c_str(), &options);

        if (cloneResult != 0) {
            LOG_CUSTOM_ERR("git", "Clone error: " << git_error_last()->message);
            if (cloneResult == GIT_EEXISTS) {
                LOG_CUSTOM("rdm", "Use --replace to force the clone");
                menus::printCloneHelp();
            }
            git_libgit2_shutdown();
            return EXIT_FAILURE;
        } else {
            LOG_INFO("Done!");
            git_libgit2_shutdown();
        }
    } else if (cmd == "dir") {
        LOG(RDM_DATA_DIR.c_str());
    } else {
        LOG("Unrecognized command '" << cmd << "'");
        menus::printMainHelp();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
