#pragma once

#include "models/BoardListModel.h"
#include "models/CardModel.h"
#include <filesystem>
#include <vector>

namespace moka::ctrl {

class PersistenceService {
public:
  struct BoardData {
    std::vector<model::BoardList> lists;
    std::vector<model::Card>      cards;
  };

  [[nodiscard]] static BoardData              load(const std::filesystem::path &path);
  static void                                  save(const std::filesystem::path &path, const BoardData &data);
  [[nodiscard]] static std::filesystem::path  defaultPath();
};

} // namespace moka::ctrl
