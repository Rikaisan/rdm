#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <string>

#include "modules.hpp"
#include "Logger.hpp"
#include "menus.hpp"
#include "utils.hpp"
#include "git2.h"

using namespace rdm;

const fs::path RDM_DATA_DIR = getDataDir();

enum class Command {
    UNKNOWN,
    APPLY,
    APPLY_SAFE,
    APPLY_SOFT,
    CLONE,
    DIR,
    HELP,
    INIT,
    LIST,
    PREVIEW,
    RESTORE
};

std::unordered_map<std::string, Command> COMMAND_MAP = {
    { "apply",      Command::APPLY      },
    { "apply-safe", Command::APPLY_SAFE },
    { "apply-soft", Command::APPLY_SOFT },
    { "clone",      Command::CLONE      },
    { "dir",        Command::DIR        },
    { "help",       Command::HELP       },
    { "init",       Command::INIT       },
    { "list",       Command::LIST       },
    { "preview",    Command::PREVIEW    },
    { "restore",    Command::RESTORE    },
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
    if (COMMAND_MAP.contains(raw_cmd)) cmd = COMMAND_MAP.at(raw_cmd);
    
    if (cmd == Command::HELP) {
        if (argc < 3) {
            menus::printHelpHelp();
        } else {
            typedef void (*HelpFunction)(void);

            std::unordered_map<std::string, HelpFunction> HELP_MAP = {
                { "apply",      menus::printApplyHelp   },
                { "apply-safe", menus::printApplyHelp   },
                { "apply-soft", menus::printApplyHelp   },
                { "clone",      menus::printCloneHelp   },
                { "dir",        menus::printDirHelp     },
                { "help",       menus::printHelpHelp    },
                { "init",       menus::printInitHelp    },
                { "list",       menus::printListHelp    },
                { "preview",    menus::printPreviewHelp },
                { "restore",    menus::printRestoreHelp },
            };

            std::string page = argv[2];
            HelpFunction helpFunction = HELP_MAP.at("help");
            if (HELP_MAP.contains(page)) helpFunction = HELP_MAP.at(page);
            helpFunction();
        }
    } else if (cmd == Command::PREVIEW || cmd == Command::APPLY || cmd == Command::APPLY_SOFT || cmd == Command::APPLY_SAFE) {
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

        LOG_SEP();
        LOG_CUSTOM("Stage", "Loading all requested modules...");
        LOG_SEP();
        ModuleManager moduleManager = ModuleManager(RDM_DATA_DIR / "home", getUserHome(), modulesAndFlags);

        for (auto& moduleName : modulesAndFlags.modules) {
            if (!moduleManager.getModules().contains(moduleName)) {
                LOG_ERR("Couldn't find the module '" << moduleName << "', skipping...");
            }
        }

        LOG_CUSTOM("Safety", "Cleaning backup directory...");
        if (cmd == Command::APPLY_SAFE) setupBackupDir();

        LOG_SEP();
        LOG_CUSTOM("Stage", "Running init operations...");
        LOG_SEP();
        moduleManager.runInits();

        LOG_SEP();
        LOG_CUSTOM("Stage", "Running file operations...");
        LOG_SEP();
        int processedModules = 0;
        for (auto& [moduleName, module]: moduleManager.getModules()) {
            if (moduleManager.shouldProcessModule(moduleName)) {
                std::optional<FileContentMap> generatedFiles = module.getGeneratedFiles();
                processedModules++;

                if (cmd == Command::PREVIEW) LOG_SEP();

                if (!generatedFiles.has_value()) {
                    LOG_CUSTOM_ERR(moduleName, "The module '" << moduleName << "' was found but had errors [" << module.getExitCode() << "]: " << module.getErrorString());
                    continue;
                } else if (generatedFiles.value().empty()) {
                    LOG_CUSTOM_DEBUG(moduleName, "The module '" << moduleName << "' was found but returned no files.");
                    continue;
                } else if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags)) {
                    LOG_CUSTOM_INFO(moduleName, "Started processing");
                }

                int skippedFiles = 0;
                int modifiedFiles = 0;
                int processedFiles = 0;
                int savedFiles = 0;
                for (auto& fileKV : generatedFiles.value()) {
                    const FileData& fileData = fileKV.second;
                    const FileDataType& dataType = fileData.getDataType();
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
                            processedFiles++;
                            if (fs::exists(file)) {
                                if (cmd == Command::APPLY_SOFT) {
                                    skippedFiles++;
                                    if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                        LOG_CUSTOM_INFO(moduleName, "Skipping " << file);
                                    continue;
                                }

                                if (cmd == Command::APPLY_SAFE) {
                                    if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                        LOG_CUSTOM_INFO(moduleName, "Creating backup of " << file);
                                    backupEntry(file);
                                    savedFiles++;
                                }

                                if (fs::is_directory(file)) {
                                    skippedFiles++;
                                    LOG_CUSTOM_ERR(moduleName, "Tried to replace a directory with a file at " << file << ", skipping to prevent data loss!");
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
                                LOG(fileData.getContent());
                            } else {
                                std::ofstream handle = std::ofstream(file);
                                handle << fileData.getContent();
                                handle.close();
                                
                                if (fileData.isExecutable()) {
                                    if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                        LOG_CUSTOM_INFO(moduleName, "Making " << file << " executable");
                                    std::filesystem::permissions(file, std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec, std::filesystem::perm_options::add);
                                }
                                
                                modifiedFiles++;
                            }
                            break;
                        case FileDataType::RawData:
                            if (cmd == Command::PREVIEW) {
                                LOG("Raw Copy");
                            } else {
                                if (fs::exists(file) && fs::is_directory(file)) {
                                    LOG_CUSTOM_ERR(moduleName, "Tried to replace a directory with a file: '" << file << "' skipping to prevent data loss!");
                                    break;
                                }

                                copyFileOrSym(fileData.getPath(), file);

                                if (fileData.isExecutable()) {
                                    if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                        LOG_CUSTOM_INFO(moduleName, "Making " << file << " executable");
                                    std::filesystem::permissions(file, std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec, std::filesystem::perm_options::add);
                                }

                                modifiedFiles++;
                            }
                            break;
                        case FileDataType::Directory:
                            if (cmd == Command::PREVIEW) {
                                fs::path sourcePath = fileData.getPath();
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
                                bool shouldAlwaysExec = fileData.isExecutable() && (fileData.getExecutablePattern().empty() || fileData.getExecutablePattern() == "*");

                                fs::path sourcePath = fileData.getPath();
                                fs::path destinationPath = file;
                                for (auto& file : getDirectoryFilesRecursive(sourcePath)) {
                                    processedFiles++;
                                    fs::path extraPath = file.lexically_relative(sourcePath);
                                    fs::path destinationFile = destinationPath / extraPath;
                                    fs::path sourceFile = sourcePath / extraPath;

                                    if (fs::exists(fs::symlink_status(destinationFile))) {
                                        if (cmd == Command::APPLY_SOFT) {
                                            if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                                LOG_CUSTOM_INFO(moduleName, "Skipping " << destinationFile);
                                            skippedFiles++;
                                            continue;
                                        }
                                        if (cmd == Command::APPLY_SAFE) {
                                            if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                                LOG_CUSTOM_INFO(moduleName, "Creating backup of " << destinationFile);
                                            backupEntry(destinationFile);
                                            savedFiles++;
                                        }
                                        if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                            LOG_CUSTOM_WARN(moduleName, "Replacing " << destinationFile);
                                        fs::remove(destinationFile);
                                    } else {
                                        if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                            LOG_CUSTOM_INFO(moduleName, "Creating " << destinationFile);
                                    }

                                    copyFileOrSym(sourceFile, destinationFile);

                                    if (shouldAlwaysExec ||
                                        (fileData.isExecutable() && fileMatchesPattern(destinationFile.filename(), fileData.getExecutablePattern()))
                                    ) {
                                        if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags))
                                            LOG_CUSTOM_INFO(moduleName, "Making " << destinationFile << " executable");
                                        std::filesystem::permissions(destinationFile, std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec, std::filesystem::perm_options::add);
                                    }

                                    modifiedFiles++;
                                }
                            }
                            break;
                        default:
                           LOG_CUSTOM_ERR(moduleName, "Received a file with an invalid data type: " << file);
                    }
                }

                if (cmd != Command::PREVIEW) LOG_CUSTOM_INFO(moduleName, "Processed " << processedFiles << " total files");
                if (cmd != Command::PREVIEW) LOG_CUSTOM_INFO(moduleName, "Created or modified " << modifiedFiles << " files");
                if (cmd == Command::APPLY_SAFE) LOG_CUSTOM_INFO(moduleName, "Backed up " << savedFiles << " files that were already present");
                if (cmd != Command::PREVIEW && skippedFiles > 0) LOG_CUSTOM_INFO(moduleName, "Skipped " << skippedFiles << " files that were already present");
                if (isFlagPresent(Flag::VERBOSE, modulesAndFlags.program_flags)) {
                    if(cmd == Command::PREVIEW) LOG_SEP();
                    LOG_CUSTOM_INFO(moduleName, "Finished processing");
                }
            }
        }

        LOG_SEP();
        LOG_CUSTOM("Stage", "Running delayed operations...");
        LOG_SEP();
        moduleManager.runDelayeds();
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
    } else if (cmd == Command::LIST) {
        const auto availableModules = ModuleManager::getAvailableModules(RDM_DATA_DIR / "home");
        if (availableModules.empty()) {
            LOG("No rdm modules found in " << RDM_DATA_DIR / "home");
        } else {
            LOG("Available modules:");
        }
        
        std::vector<std::string> orderedModules;
        orderedModules.reserve(availableModules.size());
        for (auto& [name, path] : availableModules) {
            orderedModules.push_back(name);
        }

        std::sort(orderedModules.begin(), orderedModules.end());
        for (auto& name : orderedModules) {
            LOG(" - " << name);
        }
    } else if (cmd == Command::RESTORE) {
        fs::path backupDir = getBackupDir();
        if (!fs::exists(backupDir) || (fs::exists(backupDir) && fs::is_empty(backupDir))) {
            LOG_INFO("Nothing to restore");
            return EXIT_SUCCESS;
        }

        int restoredFiles = 0;

        auto files = getDirectoryFilesRecursive(backupDir);
        LOG_INFO("Attempting to restore " << files.size() << " files...");


        for (auto& sourceFile : getDirectoryFilesRecursive(backupDir)) {
            fs::path relativeFile = sourceFile.lexically_relative(backupDir);
            LOG_INFO("Restoring file: " << relativeFile);
            fs::path destinationFile = getUserHome() / relativeFile;
            if (fs::exists(destinationFile))
                fs::remove(destinationFile);
            copyFileOrSym(sourceFile, destinationFile);
            restoredFiles++;
        }

        LOG_INFO("Finished restoring " << restoredFiles << " files");
    } else {
        LOG("Unrecognized command '" << raw_cmd << "'");
        menus::printMainHelp();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
