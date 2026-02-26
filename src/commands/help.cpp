#include "commands.hpp"
#include "src/menus.hpp"
#include <cstdlib>

int rdm::commands::help(Command, int argc, char **argv) {
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

    return EXIT_SUCCESS;
}