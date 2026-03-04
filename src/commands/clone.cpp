#include "commands.hpp"
#include "src/Logger.hpp"
#include "src/menus.hpp"
#include "src/utils.hpp"
#include <cstdlib>
#include <dlfcn.h>
#include <git2.h>

typedef int (*git_libgit2_init_t)(void);
typedef int (*git_libgit2_shutdown_t)(void);
typedef const git_error* (*git_error_last_t)(void);
typedef int (*git_clone_t)(git_repository **, const char *, const char *, const git_clone_options *);

// TODO: implement SSH authentication
int rdm::commands::clone(Command, int argc, char **argv) {
    void* libgit2 = dlopen("libgit2.so", RTLD_LAZY);
    if (!libgit2) {
        LOG_ERR("Optional dependency libgit2 was not found on your system, please install it to use this feature.");
        return EXIT_FAILURE;
    }
    dlerror();

    git_libgit2_init_t git_libgit2_init = (git_libgit2_init_t) dlsym(libgit2, "git_libgit2_init");
    git_libgit2_shutdown_t git_libgit2_shutdown = (git_libgit2_shutdown_t) dlsym(libgit2, "git_libgit2_shutdown");
    git_error_last_t git_error_last = (git_error_last_t) dlsym(libgit2, "git_error_last");
    git_clone_t git_clone = (git_clone_t) dlsym(libgit2, "git_clone");

    char* error = dlerror();
    if(error != NULL) {
        LOG_ERR("Optional dependency libgit2 was found, but there were errors loading it: " << error);
        return EXIT_FAILURE;
    }

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
        dlclose(libgit2);
        return EXIT_FAILURE;
    } else {
        LOG_INFO("Done!");
        git_libgit2_shutdown();
        dlclose(libgit2);
    }

    return EXIT_SUCCESS;
}