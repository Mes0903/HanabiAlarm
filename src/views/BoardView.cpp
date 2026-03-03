#include "views/BoardView.h"

#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace moka::views {

// ─── Layout constants ─────────────────────────────────────────────────────────
namespace {
constexpr float LIST_WIDTH       = 280.0f;
constexpr float LIST_SPACING     = 10.0f;
constexpr float CARD_PAD_H       = 10.0f; // horizontal padding inside card
constexpr float CARD_PAD_V       = 7.0f;  // vertical padding top/bottom
constexpr float STRIP_W          = 5.0f;  // color strip width
constexpr float ADD_CARD_AREA_H  = 34.0f; // height reserved for the add-card section
constexpr const char *DRAG_TYPE  = "MOKA_CARD";

// Dark-theme palette
constexpr ImU32 COL_LIST_BG       = IM_COL32(30, 32, 40, 255);
constexpr ImU32 COL_LIST_HDR      = IM_COL32(38, 40, 52, 255);
constexpr ImU32 COL_CARD_BASE     = IM_COL32(44, 46, 60, 255);
constexpr ImU32 COL_CARD_HOVER    = IM_COL32(58, 60, 76, 255);
constexpr ImU32 COL_CARD_DRAG     = IM_COL32(72, 74, 92, 255);
constexpr ImU32 COL_TEXT_MAIN     = IM_COL32(220, 222, 232, 255);
constexpr ImU32 COL_TEXT_DIM      = IM_COL32(140, 144, 165, 255);
constexpr ImU32 COL_DROP_HINT     = IM_COL32(100, 160, 255, 80);
} // namespace

// ─── Constructor ─────────────────────────────────────────────────────────────
BoardView::BoardView(ctrl::BoardController &ctrl) : ctrl_(ctrl) {}

// ─── render ──────────────────────────────────────────────────────────────────
void BoardView::render()
{
  ImGuiIO &io = ImGui::GetIO();
  ImGui::SetNextWindowPos({0.0f, 0.0f});
  ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::SetNextWindowBgAlpha(1.0f);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(22, 24, 32, 255));

  constexpr ImGuiWindowFlags BOARD_FLAGS = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoScrollWithMouse;

  ImGui::Begin("##Board", nullptr, BOARD_FLAGS);
  ImGui::PopStyleColor();

  renderToolbar_();
  ImGui::Separator();

  // ── Horizontal scrollable area for lists ─────────────────────────────────────
  ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(22, 24, 32, 255));
  ImGui::BeginChild("##board_area", {0.0f, 0.0f}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::PopStyleColor();

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {LIST_SPACING, 0.0f});

  const auto &lists = ctrl_.lists();
  for (int li = 0; li < static_cast<int>(lists.size()); ++li) {
    auto cards = ctrl_.cardsInList(lists[li].id);
    if (li > 0)
      ImGui::SameLine(0.0f, LIST_SPACING);
    renderList_(lists[li], cards);
  }

  // ── New-list column ──────────────────────────────────────────────────────────
  if (!lists.empty())
    ImGui::SameLine(0.0f, LIST_SPACING);
  renderNewListEditor_();

  ImGui::PopStyleVar();
  ImGui::EndChild();

  ImGui::End();

  // ── Deferred operations ───────────────────────────────────────────────────────
  if (!pendingDeleteCardId_.empty()) {
    ctrl_.deleteCard(pendingDeleteCardId_);
    pendingDeleteCardId_.clear();
    pendingSave_ = true;
  }
  if (!pendingDeleteListId_.empty()) {
    ctrl_.deleteList(pendingDeleteListId_);
    pendingDeleteListId_.clear();
    pendingSave_ = true;
    if (pendingDeleteListId_ == addingCardToListId_)
      addingCardToListId_.clear();
  }
  if (pendingSave_) {
    ctrl_.save();
    pendingSave_ = false;
  }
}

// ─── renderToolbar_ ──────────────────────────────────────────────────────────
void BoardView::renderToolbar_()
{
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {6.0f, 4.0f});

  if (ImGui::Button("+ New List")) {
    addingNewList_     = true;
    focusNewListInput_ = true;
    newListBuf_[0]     = '\0';
  }

  // Right-aligned menu button
  constexpr float MENU_BTN_W = 60.0f;
  ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - MENU_BTN_W);
  if (ImGui::Button("Menu", {MENU_BTN_W, 0.0f}))
    ImGui::OpenPopup("##board_menu");

  if (ImGui::BeginPopup("##board_menu")) {
    if (ImGui::MenuItem("Save Board"))
      ctrl_.save();
    ImGui::EndPopup();
  }

  ImGui::PopStyleVar();
}

// ─── renderList_ ─────────────────────────────────────────────────────────────
void BoardView::renderList_(const model::BoardList &list, const std::vector<const model::Card *> &cards)
{
  ImGui::PushID(list.id.c_str());
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_LIST_BG);

  ImGui::BeginChild("##list", {LIST_WIDTH, 0.0f}, ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
  ImGui::PopStyleColor();

  // Header
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_LIST_HDR);
  ImGui::BeginChild("##hdr", {0.0f, 32.0f}, ImGuiChildFlags_None, ImGuiWindowFlags_None);
  ImGui::PopStyleColor();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
  renderListHeader_(list);
  ImGui::EndChild();

  ImGui::Separator();

  // Cards scroll area (leaves room for add-card section)
  const float cardsAreaH = -(ADD_CARD_AREA_H + ImGui::GetStyle().ItemSpacing.y * 2.0f + 1.0f /*sep*/);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_LIST_BG);
  ImGui::BeginChild("##cards", {0.0f, cardsAreaH}, ImGuiChildFlags_None, ImGuiWindowFlags_None);
  ImGui::PopStyleColor();
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 4.0f});

  for (const auto *card : cards)
    renderCard_(*card);

  // Drop zone at the very end of the list
  renderEndDropZone_(list.id, static_cast<int>(cards.size()));

  ImGui::PopStyleVar();
  ImGui::EndChild();

  ImGui::Separator();

  // Add-card section
  renderAddCardSection_(list.id);

  ImGui::EndChild();
  ImGui::PopID();
}

// ─── renderListHeader_ ───────────────────────────────────────────────────────
void BoardView::renderListHeader_(const model::BoardList &list)
{
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {4.0f, 2.0f});

  if (editingListId_ == list.id) {
    // Inline title editor
    if (focusListTitleInput_) {
      ImGui::SetKeyboardFocusHere();
      focusListTitleInput_ = false;
    }
    ImGui::SetNextItemWidth(LIST_WIDTH - 40.0f);
    const bool committed =
        ImGui::InputText("##list_title", listTitleBuf_, sizeof(listTitleBuf_), ImGuiInputTextFlags_EnterReturnsTrue);
    if (committed && listTitleBuf_[0] != '\0') {
      ctrl_.renameList(list.id, listTitleBuf_);
      pendingSave_ = true;
    }
    if (committed || ImGui::IsItemDeactivated())
      editingListId_.clear();
  } else {
    // Display title — double-click to edit
    ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_MAIN);
    ImGui::TextUnformatted(list.title.c_str());
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
      editingListId_        = list.id;
      focusListTitleInput_  = true;
      std::strncpy(listTitleBuf_, list.title.c_str(), sizeof(listTitleBuf_) - 1);
      listTitleBuf_[sizeof(listTitleBuf_) - 1] = '\0';
    }
  }

  // Options button (…)
  ImGui::SameLine(LIST_WIDTH - 32.0f);
  if (ImGui::SmallButton("..."))
    ImGui::OpenPopup("##list_opts");

  if (ImGui::BeginPopup("##list_opts")) {
    if (ImGui::MenuItem("Delete List"))
      pendingDeleteListId_ = list.id;
    ImGui::EndPopup();
  }

  ImGui::PopStyleVar();
}

// ─── renderCard_ ─────────────────────────────────────────────────────────────
void BoardView::renderCard_(const model::Card &card)
{
  ImGui::PushID(card.id.c_str());

  const float cardW = ImGui::GetContentRegionAvail().x - 2.0f;
  const float cardH = calcCardHeight_(card);

  // Interaction zone (InvisibleButton = drag-source + drop-target anchor)
  ImGui::InvisibleButton("##btn", {cardW, cardH});
  const bool hovered  = ImGui::IsItemHovered();
  const bool isActive = ImGui::IsItemActive();
  ImVec2     pMin     = ImGui::GetItemRectMin();
  ImVec2     pMax     = ImGui::GetItemRectMax();

  // Right-click context menu
  if (ImGui::BeginPopupContextItem("##ctx")) {
    if (ImGui::MenuItem("Delete Card"))
      pendingDeleteCardId_ = card.id;
    ImGui::EndPopup();
  }

  // Drag source
  if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
    ImGui::SetDragDropPayload(DRAG_TYPE, card.id.c_str(), card.id.size() + 1);
    ImGui::TextUnformatted(card.title.c_str());
    ImGui::EndDragDropSource();
  }

  // Drop target: insert BEFORE this card
  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(DRAG_TYPE)) {
      const std::string draggedId(static_cast<const char *>(payload->Data));

      // Compute the correct insert index (sorted list, dragged card excluded)
      auto listCards = ctrl_.cardsInList(card.listId);
      int  insertIdx = 0;
      for (const auto *c : listCards) {
        if (c->id == draggedId)
          continue;
        if (c->id == card.id)
          break;
        ++insertIdx;
      }
      ctrl_.moveCard(draggedId, card.listId, insertIdx);
      pendingSave_ = true;
    }
    ImGui::EndDragDropTarget();
  }

  // ── Draw card using split draw-list channels ───────────────────────────────
  ImDrawList *dl = ImGui::GetWindowDrawList();
  dl->ChannelsSplit(2);

  // Channel 1 — foreground text content
  dl->ChannelsSetCurrent(1);
  {
    float tx = pMin.x + STRIP_W + CARD_PAD_H;
    float ty = pMin.y + CARD_PAD_V;

    // Tags row
    if (!card.tags.empty()) {
      ImGui::SetCursorScreenPos({tx, ty});
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {3.0f, 1.0f});
      ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(60, 100, 180, 180));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(60, 100, 180, 180));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(60, 100, 180, 180));
      ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_MAIN);
      for (const auto &tag : card.tags) {
        ImGui::SmallButton(tag.c_str());
        ImGui::SameLine(0.0f, 4.0f);
      }
      ImGui::PopStyleColor(4);
      ImGui::PopStyleVar();
      ty += ImGui::GetFrameHeightWithSpacing();
    }

    // Title
    ImGui::SetCursorScreenPos({tx, ty});
    ImGui::PushTextWrapPos(pMax.x - 6.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_MAIN);
    ImGui::TextUnformatted(card.title.c_str());
    ImGui::PopStyleColor();
    ImGui::PopTextWrapPos();
    ty += ImGui::GetTextLineHeightWithSpacing();

    // Checkbox progress
    if (!card.checkboxes.empty()) {
      const int checked = static_cast<int>(
          std::ranges::count_if(card.checkboxes, [](const auto &cb) { return cb.checked; }));
      char buf[32];
      std::snprintf(buf, sizeof(buf), "[%d/%d] subtasks", checked, static_cast<int>(card.checkboxes.size()));
      ImGui::SetCursorScreenPos({tx, ty});
      ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
      ImGui::TextUnformatted(buf);
      ImGui::PopStyleColor();
    }
  }

  // Channel 0 — background
  dl->ChannelsSetCurrent(0);
  {
    const ImU32 bgCol = isActive ? COL_CARD_DRAG : (hovered ? COL_CARD_HOVER : COL_CARD_BASE);
    dl->AddRectFilled(pMin, pMax, bgCol, 6.0f);

    // Color strip
    const ImVec4 stripF   = parseColor_(card.color);
    const ImU32  stripCol = ImGui::ColorConvertFloat4ToU32(stripF);
    dl->AddRectFilled(pMin, {pMin.x + STRIP_W, pMax.y}, stripCol, 6.0f);
    // Fill the non-rounded right half of the strip
    dl->AddRectFilled({pMin.x + STRIP_W / 2.0f, pMin.y}, {pMin.x + STRIP_W, pMax.y}, stripCol, 0.0f);

    // Drop-target highlight
    if (ImGui::GetDragDropPayload() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
      dl->AddRectFilled(pMin, {pMax.x, pMin.y + 3.0f}, COL_DROP_HINT, 0.0f);
  }

  dl->ChannelsMerge();

  // Restore cursor to below the card (+ 4px gap)
  ImGui::SetCursorScreenPos({pMin.x, pMax.y + 4.0f});

  ImGui::PopID();
}

// ─── renderAddCardSection_ ───────────────────────────────────────────────────
void BoardView::renderAddCardSection_(const std::string &listId)
{
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {4.0f, 4.0f});

  if (addingCardToListId_ == listId) {
    if (focusNewCardInput_) {
      ImGui::SetKeyboardFocusHere();
      focusNewCardInput_ = false;
    }
    ImGui::SetNextItemWidth(-1.0f);
    const bool committed =
        ImGui::InputText("##new_card", newCardBuf_, sizeof(newCardBuf_), ImGuiInputTextFlags_EnterReturnsTrue);
    if (committed) {
      if (newCardBuf_[0] != '\0') {
        ctrl_.addCard(listId, newCardBuf_);
        pendingSave_ = true;
      }
      newCardBuf_[0]       = '\0';
      addingCardToListId_.clear();
    } else if (ImGui::IsItemDeactivated()) {
      newCardBuf_[0]       = '\0';
      addingCardToListId_.clear();
    }
  } else {
    if (ImGui::Button("+ Add Card", {-1.0f, 0.0f})) {
      addingCardToListId_ = listId;
      focusNewCardInput_  = true;
      newCardBuf_[0]      = '\0';
    }
  }

  ImGui::PopStyleVar();
}

// ─── renderEndDropZone_ ──────────────────────────────────────────────────────
void BoardView::renderEndDropZone_(const std::string &listId, int cardCount)
{
  // A thin invisible target at the end of the list for appending cards.
  ImGui::InvisibleButton("##dz_end", {LIST_WIDTH - 16.0f, 12.0f});

  // Highlight when a drag is in progress
  if (ImGui::GetDragDropPayload() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
    ImVec2 pMin = ImGui::GetItemRectMin();
    ImVec2 pMax = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRectFilled(pMin, pMax, COL_DROP_HINT, 3.0f);
  }

  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(DRAG_TYPE)) {
      const std::string draggedId(static_cast<const char *>(payload->Data));
      // Insert at the end: count cards in list excluding the dragged card.
      auto listCards = ctrl_.cardsInList(listId);
      int  insertIdx = static_cast<int>(
          std::ranges::count_if(listCards, [&](const auto *c) { return c->id != draggedId; }));
      ctrl_.moveCard(draggedId, listId, insertIdx);
      pendingSave_ = true;
    }
    ImGui::EndDragDropTarget();
  }
}

// ─── renderNewListEditor_ ────────────────────────────────────────────────────
void BoardView::renderNewListEditor_()
{
  ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_LIST_BG);

  if (addingNewList_) {
    ImGui::BeginChild("##new_list_col", {LIST_WIDTH, 64.0f}, ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
    ImGui::PopStyleColor();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);

    if (focusNewListInput_) {
      ImGui::SetKeyboardFocusHere();
      focusNewListInput_ = false;
    }
    ImGui::SetNextItemWidth(-1.0f);
    const bool committed =
        ImGui::InputText("##new_list", newListBuf_, sizeof(newListBuf_), ImGuiInputTextFlags_EnterReturnsTrue);
    if (committed) {
      if (newListBuf_[0] != '\0') {
        ctrl_.addList(newListBuf_);
        pendingSave_ = true;
      }
      newListBuf_[0] = '\0';
      addingNewList_ = false;
    } else if (ImGui::IsItemDeactivated()) {
      newListBuf_[0] = '\0';
      addingNewList_ = false;
    }
    ImGui::EndChild();
  } else {
    // Ghost "New List" button column
    ImGui::BeginChild("##new_list_btn", {LIST_WIDTH, 48.0f}, ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
    ImGui::PopStyleColor();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 20));
    if (ImGui::Button("+ New List", {-1.0f, 0.0f})) {
      addingNewList_     = true;
      focusNewListInput_ = true;
      newListBuf_[0]     = '\0';
    }
    ImGui::PopStyleColor(2);
    ImGui::EndChild();
  }
}

// ─── calcCardHeight_ ─────────────────────────────────────────────────────────
float BoardView::calcCardHeight_(const model::Card &card) const
{
  const float rowH = ImGui::GetTextLineHeightWithSpacing();
  float       h    = CARD_PAD_V * 2.0f + rowH; // top pad + title + bottom pad
  if (!card.tags.empty())
    h += ImGui::GetFrameHeightWithSpacing();
  if (!card.checkboxes.empty())
    h += rowH;
  return h;
}

// ─── parseColor_ ─────────────────────────────────────────────────────────────
ImVec4 BoardView::parseColor_(const std::string &hex) const
{
  if (hex.size() != 7 || hex[0] != '#')
    return {0.29f, 0.56f, 0.85f, 1.0f}; // default blue

  try {
    const int r = std::stoi(hex.substr(1, 2), nullptr, 16);
    const int g = std::stoi(hex.substr(3, 2), nullptr, 16);
    const int b = std::stoi(hex.substr(5, 2), nullptr, 16);
    return {r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
  } catch (...) {
    return {0.29f, 0.56f, 0.85f, 1.0f};
  }
}

} // namespace moka::views
