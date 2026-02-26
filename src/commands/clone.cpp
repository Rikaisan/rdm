#include "commands.hpp"
#include "src/Logger.hpp"
#include "src/menus.hpp"
#include "src/utils.hpp"
#include <cstdlib>
#include <git2.h>

// TODO: Make libgit2 an optional dependency
int rdm::commands::clone(Command, int argc, char **argv) {
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

    return EXIT_SUCCESS;
}