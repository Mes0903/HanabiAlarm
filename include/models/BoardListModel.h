#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace moka::model {

struct BoardList {
  std::string id;
  std::string title;
  int         position = 0;

  static BoardList fromJson(const nlohmann::json &j)
  {
    BoardList bl;
    bl.id       = j.value("id", "");
    bl.title    = j.value("title", "");
    bl.position = j.value("position", 0);
    return bl;
  }

  [[nodiscard]] nlohmann::json toJson() const { return {{"id", id}, {"title", title}, {"position", position}}; }
};

} // namespace moka::model
