#include "commands.hpp"
#include "logger.hpp"
#include "src/modules.hpp"
#include <algorithm>
#include <cstdlib>

int rdm::commands::list(Command, int, char *[]) {
    const auto availableModules = ModuleManager::getAvailableModules(RDM_DATA_DIR / "home");
    if (availableModules.empty()) {
        LOG("No rdm modules found in " << RDM_DATA_DIR / "home");
        return EXIT_SUCCESS;
    } else {
        LOG("Available modules:");
    }
    
    std::vector<std::string> orderedModules;
    orderedModules.reserve(availableModules.size());
    for (auto& [name, path] : availableModules) {
        orderedModules.push_back(name);
    }

    std::sort(orderedModules.begin(), orderedModules.end());
    for (auto& name : orderedModules) {
        LOG(" - " << name);
    }

    return EXIT_SUCCESS;
}