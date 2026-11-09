#pragma once

#include <QString>

#include <filesystem>
#include <string>
#include <vector>

#include "backend/GuiTypes.hpp"
#include "smm/doctor.hpp"
#include "smm/types.hpp"

/// Conversion between `smm_core` value types and the Qt projections the GUI uses.
///
/// This is the only place allowed to know both sides, which keeps path encoding in one
/// spot: mixing `QString::toStdWString()` and `toStdString()` at call sites silently broke
/// non-ASCII staging paths.
namespace smm::gui {

namespace fs = std::filesystem;

/// UTF-8 / native-separator QString to a filesystem path.
fs::path toStdPath(const QString& qpath);

/// Filesystem path to a native-separator QString suitable for display and QML.
QString toQString(const fs::path& path);

ModEntry toModEntry(const smm::StagedMod& mod);
AssetEntry toAssetEntry(const smm::AssetEntry& asset);
ConflictEntry toConflictEntry(const smm::ConflictRecord& record);
PlanInfo toPlanInfo(const smm::DeployPlan& plan);
HealthInfo toHealthInfo(const smm::HealthReport& report);

/// Converts a list of ids, dropping duplicates while keeping the caller's order.
std::vector<std::string> toIdVector(const QStringList& ids);

} // namespace smm::gui
