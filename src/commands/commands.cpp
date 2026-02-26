#include "commands.hpp"
#include "src/Logger.hpp"
#include "src/menus.hpp"
#include "src/utils.hpp"
#include <cstdlib>
#include <filesystem>

namespace rdm::commands {
    const fs::path RDM_DATA_DIR = getDataDir();
    const std::unordered_map<std::string, Command> COMMAND_MAP = {
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

    const std::unordered_map<Command, CommandHandler> COMMAND_HANDLER_MAP = {
        { Command::APPLY,      apply   },
        { Command::APPLY_SAFE, apply   },
        { Command::APPLY_SOFT, apply   },
        { Command::CLONE,      clone   },
        { Command::DIR,        dir     },
        { Command::HELP,       help    },
        { Command::INIT,       init    },
        { Command::LIST,       list    },
        { Command::PREVIEW,    apply   },
        { Command::RESTORE,    restore },
        { Command::UNKNOWN,    unknown },
    };

    int dir(Command, int, char*[]) {
        LOG(RDM_DATA_DIR.c_str());
        return EXIT_SUCCESS;
    }

    int init(Command, int argc, char*[]) {
        ensureDataDirExists(true);
        LOG("Initialized rdm at " << RDM_DATA_DIR.c_str());
        if (argc > 2) {
            LOG_WARN("Detected an extra argument, if you meant to initialize from a repository, use 'rdm clone <url>' instead");
        }
        return EXIT_SUCCESS;
    }

    int unknown(Command, int, char* argv[]) {
        std::string raw_cmd = argv[1];
        LOG("Unrecognized command '" << raw_cmd << "'");
        menus::printMainHelp();
        return EXIT_FAILURE;
    }

    int runCommandHandler(Command cmd, int argc, char* argv[]) {
        CommandHandler handler = COMMAND_HANDLER_MAP.at(Command::UNKNOWN);
        if (COMMAND_HANDLER_MAP.contains(cmd)) handler = COMMAND_HANDLER_MAP.at(cmd);
        return handler(cmd, argc, argv);
    }

    Command parseCommand(std::string raw_cmd) {
        Command cmd = Command::UNKNOWN;
        if (COMMAND_MAP.contains(raw_cmd)) cmd = COMMAND_MAP.at(raw_cmd);
        return cmd;
    }
}

