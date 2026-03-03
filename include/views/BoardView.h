#pragma once

#include "controllers/BoardController.h"
#include "imgui.h"
#include <string>

namespace moka::views {

class BoardView {
public:
  explicit BoardView(ctrl::BoardController &ctrl);

  void render();

private:
  // ── Sub-renderers ────────────────────────────────────────────────────────────
  void renderToolbar_();
  void renderList_(const model::BoardList &list, const std::vector<const model::Card *> &cards);
  void renderListHeader_(const model::BoardList &list);
  void renderCard_(const model::Card &card);
  void renderAddCardSection_(const std::string &listId);
  void renderEndDropZone_(const std::string &listId, int cardCount);
  void renderNewListEditor_();

  // ── Helpers ──────────────────────────────────────────────────────────────────
  [[nodiscard]] float  calcCardHeight_(const model::Card &card) const;
  [[nodiscard]] ImVec4 parseColor_(const std::string &hex) const;

  // ── State ────────────────────────────────────────────────────────────────────
  ctrl::BoardController &ctrl_;

  // "Add Card" inline editor
  std::string addingCardToListId_;
  char        newCardBuf_[256]    = {};
  bool        focusNewCardInput_  = false;

  // List title inline editor
  std::string editingListId_;
  char        listTitleBuf_[256]  = {};
  bool        focusListTitleInput_ = false;

  // "New List" inline editor
  bool addingNewList_       = false;
  char newListBuf_[256]     = {};
  bool focusNewListInput_   = false;

  // Deferred operations (processed after the ImGui frame to avoid iterator invalidation)
  std::string pendingDeleteCardId_;
  std::string pendingDeleteListId_;
  bool        pendingSave_ = false;
};

} // namespace moka::views
