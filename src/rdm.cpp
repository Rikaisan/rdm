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

enum class Command {
    UNKNOWN,
    HELP,
    PREVIEW,
    APPLY,
    APPLY_SOFT,
    INIT,
    CLONE,
    DIR
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
            maf.program_flags.insert(arg);
            continue;
        }
        maf.modules.insert(arg);
    }

    while (currentArg < count) {
        auto& arg = args.at(currentArg++);
        if (arg.starts_with("-")) {
            maf.program_flags.insert(arg);
            continue;
        }
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

    std::string raw_cmd = argv[1];
    Command cmd = Command::UNKNOWN;
    if (raw_cmd == "help") cmd = Command::HELP;
    else if (raw_cmd == "preview") cmd = Command::PREVIEW;
    else if (raw_cmd == "apply") cmd = Command::APPLY;
    else if (raw_cmd == "apply-soft") cmd = Command::APPLY_SOFT;
    else if (raw_cmd == "init") cmd = Command::INIT;
    else if (raw_cmd == "clone") cmd = Command::CLONE;
    else if (raw_cmd == "dir") cmd = Command::DIR;

    
    if (cmd == Command::HELP) {
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
    } else if (cmd == Command::PREVIEW || cmd == Command::APPLY || cmd == Command::APPLY_SOFT) {
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
        if (cmd == Command::PREVIEW) modulesAndFlags.flags.insert("preview");

        ModuleManager moduleManager = ModuleManager(RDM_DATA_DIR / "home", getUserHome(), modulesAndFlags);

        for (auto& moduleName : modulesAndFlags.modules) {
            if (!moduleManager.getModules().contains(moduleName)) {
                LOG_ERR("Couldn't find the module '" << moduleName << "', skipping...");
            }
        }

        int processedModules = 0;
        for (auto& [moduleName, module]: moduleManager.getModules()) {
            if (moduleManager.shouldProcessModule(moduleName)) {
                FileContentMap generatedFiles = module.getGeneratedFiles();
                processedModules++;

                if (cmd == Command::PREVIEW) LOG_SEP();

                if (generatedFiles.empty()) {
                    if (module.getExitCode() == LUA_OK) {
                        LOG_CUSTOM_DEBUG(moduleName, "The module '" << moduleName << "' was found but returned no files.");
                        module.runDelayed();
                    } else {
                        LOG_CUSTOM_ERR(moduleName, "The module '" << moduleName << "' was found but had errors [" << module.getExitCode() << "]: " << module.getErrorString());
                    }
                    continue;
                } else {
                    LOG_CUSTOM_INFO(moduleName, "Started processing module");
                }

                int skippedFiles = 0;
                for (auto& fileKV : generatedFiles) {
                    const FileDataType dataType = fileKV.second.getDataType();
                    const fs::path file = fileKV.first;
                    if (cmd == Command::PREVIEW) {
                        LOG_SEP();
                        LOG_CUSTOM(moduleName, file << ":");
                    }

                    LOG_CUSTOM_DEBUG(moduleName, "Processing: " << file);
                    LOG_CUSTOM_DEBUG(moduleName, (dataType == FileDataType::Directory ? "Directory: " : "File: ") << file.stem());
                    LOG_CUSTOM_DEBUG(moduleName, "Destination: " << file.parent_path());
                    
                    if (cmd != Command::PREVIEW) {
                        fs::create_directories(file.parent_path());

                        if (dataType != FileDataType::Directory) {
                            if (fs::exists(file)) {
                                if (cmd == Command::APPLY_SOFT) {
                                    skippedFiles++;
                                    if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                        LOG_CUSTOM_INFO(moduleName, "Skipping " << file);
                                    continue;
                                }

                                if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                    LOG_CUSTOM_WARN(moduleName, "Replacing " << file);

                                fs::remove(file);
                            } else {
                                if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                    LOG_CUSTOM_INFO(moduleName, "Creating " << file);
                            }
                        }
                    }

                    switch (dataType) {
                        case FileDataType::Text:
                            if (cmd == Command::PREVIEW) {
                                LOG(fileKV.second.getContent());
                            } else {
                                std::ofstream handle = std::ofstream(file);
                                handle << fileKV.second.getContent();
                                handle.close();
                            }
                            break;
                        case FileDataType::RawData:
                            if (cmd == Command::PREVIEW) {
                                LOG("Raw Copy");
                            } else {
                                fs::copy_file(fileKV.second.getPath(), file);
                            }
                            break;
                        case FileDataType::Directory:
                            if (cmd == Command::PREVIEW) {
                                fs::path sourcePath = fileKV.second.getPath();
                                LOG_CUSTOM(moduleName, "Copy of directory " << sourcePath.c_str() << ":");
                                auto files = getDirectoryFilesRecursive(sourcePath);
                                size_t fileCount = files.size();
                                size_t filesToPrint = fileCount >= 16 ? 16 : fileCount;
                                for (size_t i{0}; i < filesToPrint; ++i) {
                                    fs::path extraPath = files.at(i).lexically_relative(sourcePath);
                                    LOG(" - " << (file / extraPath).c_str());
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
                                        if (cmd == Command::APPLY_SOFT) {
                                            skippedFiles++;
                                            if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                                LOG_CUSTOM_INFO(moduleName, "Skipping " << file);
                                            continue;
                                        }
                                        if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                            LOG_CUSTOM_WARN(moduleName, "Replacing " << destinationFile);
                                        fs::remove(destinationFile);
                                    } else {
                                        if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                            LOG_CUSTOM_INFO(moduleName, "Creating " << destinationFile);
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
                           LOG_CUSTOM_ERR(moduleName, "Received a file with an invalid data type: " << file);
                    }
                }
                if (skippedFiles > 0) LOG_CUSTOM_INFO(moduleName, "Skipped " << skippedFiles << " files that were already present");
                module.runDelayed();
                if (skippedFiles > 0) LOG_CUSTOM_INFO(moduleName, "Finished processing module");
            }
        }
        if (cmd == Command::PREVIEW && processedModules > 0) LOG_SEP();
    } else if (cmd == Command::INIT) {
        ensureDataDirExists(true);
        LOG("Initialized rdm at " << RDM_DATA_DIR.c_str());
        if (argc > 2) {
            LOG_WARN("Detected an extra argument, if you meant to initialize from a repository, use 'rdm clone <url>' instead");
        }
    } else if (cmd == Command::CLONE) {
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
    } else if (cmd == Command::DIR) {
        LOG(RDM_DATA_DIR.c_str());
    } else {
        LOG("Unrecognized command '" << raw_cmd << "'");
        menus::printMainHelp();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
