#include <cstdlib>
#include "logger.hpp"
#include "menus.hpp"
#include "utils.hpp"
#include "commands/commands.hpp"

using namespace rdm;

int main(int argc, char* argv[]) {
    if(!copyRDMLib()) {
        LOG_DEBUG("Couldn't copy RDM lib.");
    }
    
    if (argc == 1) {
        menus::printMainHelp();
        return EXIT_SUCCESS;
    }

    return commands::runCommandHandler(commands::parseCommand(argv[1]), argc, argv);
}
