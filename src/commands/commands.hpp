#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>

namespace fs = std::filesystem;

namespace rdm::commands {
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

    typedef int (*CommandHandler)(Command, int, char*[]);

    extern const std::unordered_map<std::string, Command> COMMAND_MAP;
    extern const std::unordered_map<Command, CommandHandler> COMMAND_HANDLER_MAP;
    extern const fs::path RDM_DATA_DIR;

    // In commands.cpp
    int dir(Command cmd, int argc, char* argv[]);
    int init(Command cmd, int argc, char* argv[]);
    int unknown(Command cmd, int argc, char* argv[]);
    int runCommandHandler(Command cmd, int argc, char* argv[]);
    Command parseCommand(std::string cmd);

    // External
    int apply(Command cmd, int argc, char* argv[]);
    int clone(Command cmd, int argc, char* argv[]);
    int help(Command cmd, int argc, char* argv[]);
    int list(Command cmd, int argc, char* argv[]);
    int restore(Command cmd, int argc, char* argv[]);
}