#include "Logger.hpp"

namespace rdm::menus {
    void printMainHelp() {
        LOG("Usage: rdm <command> [args...]");
        LOG(" apply             Runs the scripts");
        LOG(" apply-soft        Runs the scripts but doesn't replace existing files");
        LOG(" clone             Populates the rdm data directory with a git repository");
        LOG(" dir               Print RDM_DATA_DIR to stdout, useful to quickly cd with 'cd $(rdm dir)'");
        LOG(" help              Prints the help menu of a command");
        LOG(" init              Initializes the rdm data directory");
        LOG(" preview           Preview an apply command, displays files returned by modules and sets the 'preview' flag");
    }
    
    void printApplyHelp() {
        LOG("Usage: rdm apply|apply-soft [modules...] [options...]");
        LOG(" module            The name of the module to apply (e.g. rdm-hyprland.lua -> hyprland), leave empty for all modules");
        LOG("Options:");
        LOG(" -v,--verbose      Print more information about what RDM is doing");
        LOG(" -f,--flags        A space separated list of flags that should be passed to the modules");
        LOG("Examples:");
        LOG(" rdm apply                                       -> Applies all modules without any flags set");
        LOG(" rdm apply -f es setup                           -> Applies all modules with the flags 'es' and 'setup' set");
        LOG(" rdm apply hyprland wallpapers -v -f laptop arch -> Applies the hyprland and wallpapers modules with the flags 'laptop' and 'arch' set and enables verbose mode");
        LOG(" rdm soft-apply wallpapers                       -> Applies the wallpapers module without replacing any existing files");
    }

    void printDirHelp() {
        LOG("Usage: rdm dir");
        LOG(" Print RDM_DATA_DIR to stdout, useful to quickly cd with 'cd $(rdm dir)'");
    }

    void printHelpHelp() {
        LOG("Usage: rdm help <command>");
        LOG("Valid commands: apply, apply-soft, clone, dir, init, preview");
    }

    void printInitHelp() {
        LOG("Usage: rdm init");
        LOG("Creates and populates the data directory with the rdm lua meta file");
    }

    void printCloneHelp() {
        LOG("Usage: rdm clone <repo> [--replace]");
        LOG(" repo              A git repository to clone as the rdm data directory");
        LOG("Options:");
        LOG(" --replace         Deletes the data directory if it exists before attempting to clone");
    }

    void printPreviewHelp() {
        LOG("Usage: rdm preview [modules...] [options...]");
        LOG(" module            The name of the module to apply (e.g. rdm-hyprland.lua -> hyprland), leave empty for all modules");
        LOG("Options:");
        LOG(" -f,--flags        A space separated list of flags that should be passed to the modules");
        LOG("Notes:");
        LOG(" Works exactly like apply, except it sets the 'preview' flag and will display the files instead of creating or replacing them");
    }
}