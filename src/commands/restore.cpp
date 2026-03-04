#include "commands.hpp"
#include "src/utils.hpp"
#include "logger.hpp"
#include <cstdlib>

int rdm::commands::restore(Command, int, char*[]) {
    fs::path backupDir = getBackupDir("home");
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
    return EXIT_SUCCESS;
}