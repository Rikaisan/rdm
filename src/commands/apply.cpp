#include "commands.hpp"
#include "src/Logger.hpp"
#include "src/modules.hpp"
#include "src/utils.hpp"
#include <cstdlib>
#include <fstream>

using namespace rdm;

int rdm::commands::apply(Command cmd, int argc, char **argv) {
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
    if (cmd == Command::APPLY_SAFE) setupBackupDir("home");

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
            } else if (modulesAndFlags.programFlags.contains(Flag::VERBOSE)) {
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
                                if (modulesAndFlags.programFlags.contains(Flag::VERBOSE))
                                    LOG_CUSTOM_INFO(moduleName, "Skipping " << file);
                                continue;
                            }

                            if (cmd == Command::APPLY_SAFE) {
                                if (modulesAndFlags.programFlags.contains(Flag::VERBOSE))
                                    LOG_CUSTOM_INFO(moduleName, "Creating backup of " << file);
                                backupEntry("home", file);
                                savedFiles++;
                            }

                            if (fs::is_directory(file)) {
                                skippedFiles++;
                                LOG_CUSTOM_ERR(moduleName, "Tried to replace a directory with a file at " << file << ", skipping to prevent data loss!");
                                continue;
                            }

                            if (modulesAndFlags.programFlags.contains(Flag::VERBOSE))
                                LOG_CUSTOM_WARN(moduleName, "Replacing " << file);

                            fs::remove(file);
                        } else {
                            if (modulesAndFlags.programFlags.contains(Flag::VERBOSE))
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
                                if (modulesAndFlags.programFlags.contains(Flag::VERBOSE))
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
                                if (modulesAndFlags.programFlags.contains(Flag::VERBOSE))
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
                                        if (modulesAndFlags.programFlags.contains(Flag::VERBOSE))
                                            LOG_CUSTOM_INFO(moduleName, "Skipping " << destinationFile);
                                        skippedFiles++;
                                        continue;
                                    }
                                    if (cmd == Command::APPLY_SAFE) {
                                        if (modulesAndFlags.programFlags.contains(Flag::VERBOSE))
                                            LOG_CUSTOM_INFO(moduleName, "Creating backup of " << destinationFile);
                                        backupEntry("home", destinationFile);
                                        savedFiles++;
                                    }
                                    if (modulesAndFlags.programFlags.contains(Flag::VERBOSE))
                                        LOG_CUSTOM_WARN(moduleName, "Replacing " << destinationFile);
                                    fs::remove(destinationFile);
                                } else {
                                    if (modulesAndFlags.programFlags.contains(Flag::VERBOSE))
                                        LOG_CUSTOM_INFO(moduleName, "Creating " << destinationFile);
                                }

                                copyFileOrSym(sourceFile, destinationFile);

                                if (shouldAlwaysExec ||
                                    (fileData.isExecutable() && fileMatchesPattern(destinationFile.filename(), fileData.getExecutablePattern()))
                                ) {
                                    if (modulesAndFlags.programFlags.contains(Flag::VERBOSE))
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
            if (modulesAndFlags.programFlags.contains(Flag::VERBOSE)) {
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

    return EXIT_SUCCESS;
}