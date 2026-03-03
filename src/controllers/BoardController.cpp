#include "controllers/BoardController.h"

#include "controllers/PersistenceService.h"
#include <algorithm>
#include <iomanip>
#include <random>
#include <ranges>
#include <sstream>

namespace moka::ctrl {

// ─── load / save ─────────────────────────────────────────────────────────────
void BoardController::load()
{
  auto data = PersistenceService::load(PersistenceService::defaultPath());
  lists_    = std::move(data.lists);
  cards_    = std::move(data.cards);
  repositionLists_();
}

void BoardController::save()
{
  PersistenceService::BoardData data;
  data.lists = lists_;
  data.cards = cards_;
  PersistenceService::save(PersistenceService::defaultPath(), data);
}

// ─── List operations ─────────────────────────────────────────────────────────
void BoardController::addList(const std::string &title)
{
  model::BoardList bl;
  bl.id       = generateId_();
  bl.title    = title;
  bl.position = static_cast<int>(lists_.size());
  lists_.push_back(std::move(bl));
}

void BoardController::renameList(const std::string &listId, const std::string &newTitle)
{
  if (auto it = std::ranges::find_if(lists_, [&](const auto &l) { return l.id == listId; }); it != lists_.end())
    it->title = newTitle;
}

void BoardController::deleteList(const std::string &listId)
{
  std::erase_if(cards_, [&](const model::Card &c) { return c.listId == listId; });
  std::erase_if(lists_, [&](const model::BoardList &l) { return l.id == listId; });
  repositionLists_();
}

// ─── Card operations ─────────────────────────────────────────────────────────
void BoardController::addCard(const std::string &listId, const std::string &title)
{
  // Find the highest position in the target list.
  int maxPos = -1;
  for (const auto &c : cards_ | std::views::filter([&](const auto &c) { return c.listId == listId; }))
    maxPos = std::max(maxPos, c.position);

  model::Card card;
  card.id       = generateId_();
  card.listId   = listId;
  card.position = maxPos + 1;
  card.title    = title;
  cards_.push_back(std::move(card));
}

void BoardController::deleteCard(const std::string &cardId)
{
  std::string listId;
  if (auto it = std::ranges::find_if(cards_, [&](const auto &c) { return c.id == cardId; }); it != cards_.end())
    listId = it->listId;

  std::erase_if(cards_, [&](const model::Card &c) { return c.id == cardId; });

  if (!listId.empty())
    reposition_(listId);
}

void BoardController::moveCard(const std::string &cardId, const std::string &targetListId, int targetPosition)
{
  model::Card *card = findCard(cardId);
  if (!card)
    return;

  const std::string srcListId = card->listId;
  card->listId                = targetListId;

  // Build ordered list of cards in the target list (excluding the moved card).
  std::vector<model::Card *> targetCards;
  for (auto &c : cards_)
    if (c.listId == targetListId && c.id != cardId)
      targetCards.push_back(&c);

  std::ranges::sort(targetCards, {}, &model::Card::position);

  targetPosition = std::clamp(targetPosition, 0, static_cast<int>(targetCards.size()));
  targetCards.insert(targetCards.begin() + targetPosition, card);

  for (int i = 0; i < static_cast<int>(targetCards.size()); ++i)
    targetCards[i]->position = i;

  if (srcListId != targetListId)
    reposition_(srcListId);
}

void BoardController::updateCard(const model::Card &card)
{
  if (auto it = std::ranges::find_if(cards_, [&](const auto &c) { return c.id == card.id; }); it != cards_.end())
    *it = card;
}

// ─── Accessors ───────────────────────────────────────────────────────────────
std::vector<const model::Card *> BoardController::cardsInList(const std::string &listId) const
{
  std::vector<const model::Card *> result;
  for (const auto &c : cards_)
    if (c.listId == listId)
      result.push_back(&c);

  std::ranges::sort(result, {}, &model::Card::position);
  return result;
}

model::Card *BoardController::findCard(const std::string &cardId)
{
  auto it = std::ranges::find_if(cards_, [&](const auto &c) { return c.id == cardId; });
  return it != cards_.end() ? &*it : nullptr;
}

const model::Card *BoardController::findCard(const std::string &cardId) const
{
  auto it = std::ranges::find_if(cards_, [&](const auto &c) { return c.id == cardId; });
  return it != cards_.end() ? &*it : nullptr;
}

// ─── Private helpers ─────────────────────────────────────────────────────────
std::string BoardController::generateId_() const
{
  static std::mt19937                            gen(std::random_device{}());
  static std::uniform_int_distribution<uint32_t> dis32;
  static std::uniform_int_distribution<uint16_t> dis16;

  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(8) << dis32(gen) << '-' << std::setw(4) << dis16(gen) << '-'
      << std::setw(4) << ((dis16(gen) & 0x0FFFu) | 0x4000u) << '-' << std::setw(4) << ((dis16(gen) & 0x3FFFu) | 0x8000u) << '-'
      << std::setw(8) << dis32(gen) << std::setw(4) << dis16(gen);
  return oss.str();
}

void BoardController::reposition_(const std::string &listId)
{
  std::vector<model::Card *> listCards;
  for (auto &c : cards_)
    if (c.listId == listId)
      listCards.push_back(&c);

  std::ranges::sort(listCards, {}, &model::Card::position);

  for (int i = 0; i < static_cast<int>(listCards.size()); ++i)
    listCards[i]->position = i;
}

void BoardController::repositionLists_()
{
  std::ranges::sort(lists_, {}, &model::BoardList::position);

  for (int i = 0; i < static_cast<int>(lists_.size()); ++i)
    lists_[i].position = i;
}

} // namespace moka::ctrl
