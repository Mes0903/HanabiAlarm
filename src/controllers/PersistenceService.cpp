#include "controllers/PersistenceService.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <windows.h>

namespace moka::ctrl {

// ─── load ─────────────────────────────────────────────────────────────────────
PersistenceService::BoardData PersistenceService::load(const std::filesystem::path &path)
{
  BoardData data;
  if (!std::filesystem::exists(path))
    return data;

  std::ifstream ifs(path);
  if (!ifs.is_open())
    return data;

  try {
    nlohmann::json j;
    ifs >> j;

    if (j.contains("lists") && j["lists"].is_array())
      for (const auto &l : j["lists"])
        data.lists.push_back(model::BoardList::fromJson(l));

    if (j.contains("cards") && j["cards"].is_array())
      for (const auto &c : j["cards"])
        data.cards.push_back(model::Card::fromJson(c));
  } catch (...) {
    // Return whatever was parsed; ignore malformed entries.
  }

  return data;
}

// ─── save ─────────────────────────────────────────────────────────────────────
void PersistenceService::save(const std::filesystem::path &path, const BoardData &data)
{
  std::filesystem::create_directories(path.parent_path());

  nlohmann::json j;
  j["lists"] = nlohmann::json::array();
  for (const auto &l : data.lists)
    j["lists"].push_back(l.toJson());

  j["cards"] = nlohmann::json::array();
  for (const auto &c : data.cards)
    j["cards"].push_back(c.toJson());

  std::ofstream ofs(path);
  ofs << j.dump(2);
}

// ─── defaultPath ──────────────────────────────────────────────────────────────
std::filesystem::path PersistenceService::defaultPath()
{
  wchar_t exePath[MAX_PATH] = {};
  GetModuleFileNameW(nullptr, exePath, MAX_PATH);
  return std::filesystem::path(exePath).parent_path() / "data" / "board.json";
}

} // namespace moka::ctrl
