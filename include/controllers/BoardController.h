#pragma once

#include "models/BoardListModel.h"
#include "models/CardModel.h"
#include <string>
#include <vector>

namespace moka::ctrl {

class BoardController {
public:
  BoardController() = default;

  void load();
  void save();

  // ── List operations ──────────────────────────────────────────────────────────
  void addList(const std::string &title);
  void renameList(const std::string &listId, const std::string &newTitle);
  void deleteList(const std::string &listId);

  // ── Card operations ──────────────────────────────────────────────────────────
  void addCard(const std::string &listId, const std::string &title);
  void deleteCard(const std::string &cardId);
  void moveCard(const std::string &cardId, const std::string &targetListId, int targetPosition);
  void updateCard(const model::Card &card);

  // ── Accessors ────────────────────────────────────────────────────────────────
  [[nodiscard]] const std::vector<model::BoardList> &lists() const { return lists_; }

  [[nodiscard]] std::vector<const model::Card *> cardsInList(const std::string &listId) const;

  [[nodiscard]] model::Card *      findCard(const std::string &cardId);
  [[nodiscard]] const model::Card *findCard(const std::string &cardId) const;

private:
  [[nodiscard]] std::string generateId_() const;
  void                      reposition_(const std::string &listId);
  void                      repositionLists_();

  std::vector<model::BoardList> lists_;
  std::vector<model::Card>      cards_;
};

} // namespace moka::ctrl
