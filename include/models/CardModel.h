#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace moka::model {

// ─── CheckboxItem ─────────────────────────────────────────────────────────────
struct CheckboxItem {
  std::string label;
  bool        checked = false;

  static CheckboxItem fromJson(const nlohmann::json &j) { return {j.value("label", ""), j.value("checked", false)}; }

  [[nodiscard]] nlohmann::json toJson() const { return {{"label", label}, {"checked", checked}}; }
};

// ─── ScheduleAction ───────────────────────────────────────────────────────────
struct ScheduleAction {
  std::string program;
  std::string arguments;

  static ScheduleAction fromJson(const nlohmann::json &j) { return {j.value("program", ""), j.value("arguments", "")}; }

  [[nodiscard]] nlohmann::json toJson() const { return {{"program", program}, {"arguments", arguments}}; }
};

// ─── TriggerType ──────────────────────────────────────────────────────────────
enum class TriggerType { Daily, Weekly, MonthlyByWeekday, Once };

// ─── ScheduleTrigger ──────────────────────────────────────────────────────────
struct ScheduleTrigger {
  TriggerType      type        = TriggerType::Daily;
  std::vector<int> daysOfWeek; // Weekly: 0=Sun … 6=Sat
  int              weekOfMonth = 1; // MonthlyByWeekday: 1–5
  int              dayOfWeek   = 0; // MonthlyByWeekday: 0=Sun … 6=Sat
  int              year        = 0; // Once
  int              month       = 1; // Once (1–12)
  int              day         = 1; // Once (1–31)
  int              hour        = 9;
  int              minute      = 0;

  static TriggerType typeFromString(const std::string &s)
  {
    if (s == "weekly") return TriggerType::Weekly;
    if (s == "monthly_by_weekday") return TriggerType::MonthlyByWeekday;
    if (s == "once") return TriggerType::Once;
    return TriggerType::Daily;
  }

  static std::string typeToString(TriggerType t)
  {
    switch (t) {
    case TriggerType::Weekly:           return "weekly";
    case TriggerType::MonthlyByWeekday: return "monthly_by_weekday";
    case TriggerType::Once:             return "once";
    default:                            return "daily";
    }
  }

  static ScheduleTrigger fromJson(const nlohmann::json &j)
  {
    ScheduleTrigger st;
    st.type        = typeFromString(j.value("type", "daily"));
    st.weekOfMonth = j.value("week_of_month", 1);
    st.dayOfWeek   = j.value("day_of_week", 0);
    st.year        = j.value("year", 0);
    st.month       = j.value("month", 1);
    st.day         = j.value("day", 1);
    st.hour        = j.value("hour", 9);
    st.minute      = j.value("minute", 0);
    if (j.contains("days_of_week") && j["days_of_week"].is_array())
      st.daysOfWeek = j["days_of_week"].get<std::vector<int>>();
    return st;
  }

  [[nodiscard]] nlohmann::json toJson() const
  {
    nlohmann::json j;
    j["type"]   = typeToString(type);
    j["hour"]   = hour;
    j["minute"] = minute;
    if (type == TriggerType::Weekly) {
      j["days_of_week"] = daysOfWeek;
    } else if (type == TriggerType::MonthlyByWeekday) {
      j["week_of_month"] = weekOfMonth;
      j["day_of_week"]   = dayOfWeek;
    } else if (type == TriggerType::Once) {
      j["year"]  = year;
      j["month"] = month;
      j["day"]   = day;
    }
    return j;
  }
};

// ─── Schedule ─────────────────────────────────────────────────────────────────
struct Schedule {
  bool                         enabled = false;
  ScheduleAction               action;
  std::vector<ScheduleTrigger> triggers;

  static Schedule fromJson(const nlohmann::json &j)
  {
    Schedule s;
    s.enabled = j.value("enabled", false);
    if (j.contains("action") && j["action"].is_object())
      s.action = ScheduleAction::fromJson(j["action"]);
    if (j.contains("triggers") && j["triggers"].is_array())
      for (const auto &t : j["triggers"])
        s.triggers.push_back(ScheduleTrigger::fromJson(t));
    return s;
  }

  [[nodiscard]] nlohmann::json toJson() const
  {
    nlohmann::json j;
    j["enabled"]  = enabled;
    j["action"]   = action.toJson();
    j["triggers"] = nlohmann::json::array();
    for (const auto &t : triggers)
      j["triggers"].push_back(t.toJson());
    return j;
  }
};

// ─── Card ─────────────────────────────────────────────────────────────────────
struct Card {
  std::string              id;
  std::string              listId;
  int                      position    = 0;
  std::string              title;
  std::string              description;
  std::vector<CheckboxItem> checkboxes;
  std::string              color       = "#4A90D9";
  std::vector<std::string> tags;
  float                    noteX       = 100.0f;
  float                    noteY       = 200.0f;
  float                    noteW       = 300.0f;
  float                    noteH       = 220.0f;
  bool                     noteVisible = false;
  std::optional<Schedule>  schedule;

  static Card fromJson(const nlohmann::json &j)
  {
    Card c;
    c.id          = j.value("id", "");
    c.listId      = j.value("list_id", "");
    c.position    = j.value("position", 0);
    c.title       = j.value("title", "");
    c.description = j.value("description", "");
    c.color       = j.value("color", "#4A90D9");
    if (j.contains("tags") && j["tags"].is_array())
      c.tags = j["tags"].get<std::vector<std::string>>();
    if (j.contains("checkboxes") && j["checkboxes"].is_array())
      for (const auto &cb : j["checkboxes"])
        c.checkboxes.push_back(CheckboxItem::fromJson(cb));
    if (j.contains("note_pos") && j["note_pos"].is_object()) {
      c.noteX = j["note_pos"].value("x", 100.0f);
      c.noteY = j["note_pos"].value("y", 200.0f);
    }
    if (j.contains("note_size") && j["note_size"].is_object()) {
      c.noteW = j["note_size"].value("w", 300.0f);
      c.noteH = j["note_size"].value("h", 220.0f);
    }
    c.noteVisible = j.value("note_visible", false);
    if (j.contains("schedule") && j["schedule"].is_object())
      c.schedule = Schedule::fromJson(j["schedule"]);
    return c;
  }

  [[nodiscard]] nlohmann::json toJson() const
  {
    nlohmann::json j;
    j["id"]          = id;
    j["list_id"]     = listId;
    j["position"]    = position;
    j["title"]       = title;
    j["description"] = description;
    j["color"]       = color;
    j["tags"]        = tags;
    j["checkboxes"]  = nlohmann::json::array();
    for (const auto &cb : checkboxes)
      j["checkboxes"].push_back(cb.toJson());
    j["note_pos"]    = {{"x", noteX}, {"y", noteY}};
    j["note_size"]   = {{"w", noteW}, {"h", noteH}};
    j["note_visible"] = noteVisible;
    j["schedule"]    = schedule ? schedule->toJson() : nlohmann::json(nullptr);
    return j;
  }
};

} // namespace moka::model
