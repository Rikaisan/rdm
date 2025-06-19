#include "Logger.hpp"

namespace rdm::menus {
    void printMainHelp() {
        LOG("Usage: rdm <command> [args...]");
        LOG(" apply             Runs the scripts");
        LOG(" apply-soft        Runs the scripts but doesn't replace existing files");
        LOG(" init              Initializes rdm with a git repository");
        LOG(" preview           Preview a specific module output");
    }
    
    void printApplyHelp() {
        LOG("Usage: rdm apply|apply-soft [modules...] [options...]");
        LOG(" module            The name of the module to apply (e.g. rdm-hyprland.lua -> hyprland), leave empty for all modules");
        LOG("Options:");
        LOG(" -f,--flags        A space separated list of flags that should be passed to the modules");
    }

    void printInitHelp() {
        LOG("Usage: rdm init [repo]");
        LOG(" repo              A git repository to glone as the rdm home directory");
        LOG("If no repository is specified, the base directory structure will be created");
    }

    void printPreviewHelp() {
        LOG("Usage: rdm preview <module>");
        LOG(" module            The name of the module to apply (e.g. rdm-hyprland.lua -> hyprland), leave empty for all modules");
    }
}