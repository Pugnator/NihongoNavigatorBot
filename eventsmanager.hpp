#pragma once
#include <tgbot/tgbot.h>

namespace Bot
{
  class EventsManager
  {
  public:
    using Ptr = std::unique_ptr<EventsManager>;
    EventsManager(TgBot::Bot &bot);
    ~EventsManager() = default;
    EventsManager(const EventsManager &) = delete;
    EventsManager &operator=(const EventsManager &) = delete;

  private:
    TgBot::Bot &bot_;
  };
}