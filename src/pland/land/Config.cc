#include "pland/land/Config.h"
#include "pland/PLand.h"

#include "ll/api/Config.h"

#include "internal/ConfigMigrator.h"
#include "pland/utils/JsonUtil.h"

#include <filesystem>
#include <string>

namespace land {

namespace fs = std::filesystem;

bool Config::tryLoad() {
    auto path = land::PLand::getInstance().getSelf().getConfigDir() / FileName;

    if (!std::filesystem::exists(path)) {
        trySave();
        return true;
    }
    auto data = ll::file_utils::readFile(path);
    if (!data) {
        return false;
    }
    auto json = nlohmann::json::parse(*data);

    auto& migrator = internal::ConfigMigrator::getInstance();
    if (auto exp = migrator.migrate(json, SchemaVersion); !exp) {
        throw std::runtime_error{exp.error().message()};
    }
    json_util::json2structWithVersionPatch(json, cfg, true);
    return true;
}

bool Config::trySave() {
    auto dir = land::PLand::getInstance().getSelf().getConfigDir() / FileName;

    bool status = ll::config::saveConfig(cfg, dir);

    return status;
}
bool Config::ensureDimensionAllowed(int dimensionId) {
    auto& allowed = cfg.land.bought.allowDimensions;
    return std::find(allowed.begin(), allowed.end(), dimensionId) != allowed.end();
}
bool Config::ensureSubLandFeatureEnabled() { return cfg.land.subLand.enabled; }
bool Config::ensureOrdinaryLandEnabled(bool is3D) {
    return is3D ? cfg.land.bought.threeDimensionl.enabled : cfg.land.bought.twoDimensionl.enabled;
}
bool Config::ensureLeasingEnabled() { return cfg.land.leasing.enabled; }
bool Config::ensureLeasingDimensionAllowed(int dimensionId) {
    auto& allowed = cfg.land.leasing.allowDimensions;
    return std::find(allowed.begin(), allowed.end(), dimensionId) != allowed.end();
}
bool                  Config::ensureEconomySystemEnabled() { return cfg.economy.enabled; }
std::optional<double> Config::getLandDimensionMultipliers(LandDimid dimid) {
    auto& map  = cfg.land.bought.dimensionPriceCoefficients;
    auto  iter = map.find(std::to_string(dimid));
    if (iter != map.end()) {
        return iter->second;
    }
    return std::nullopt;
}
std::string const& Config::getLandPriceCalculateFormula(bool is3D) {
    return is3D ? cfg.land.bought.threeDimensionl.calculate : cfg.land.bought.twoDimensionl.calculate;
}
std::string const& Config::getSubLandPriceCalculateFormula() { return cfg.land.subLand.calculate; }


Config Config::cfg = {};

} // namespace land
