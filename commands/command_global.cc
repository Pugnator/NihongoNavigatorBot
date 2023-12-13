#include "botcommander.hpp"
#include "log.hpp"

namespace Bot
{
  const BotCommander &BotCommander::start(int64_t userId)
  {
    if (userManager_->userExists(userId))
    {
      bot_.getApi().sendMessage(userId, "You've already registered.");
      LOG_INFO("User {} already exists\n", userId);
    }
    else
    {
      userManager_->createUserEntry(userId);
      LOG_INFO("User {} created\n", userId);
      bot_.getApi().sendMessage(userId, "The entry for you has been created.");
    }
    return *this;
  }

  const BotCommander &BotCommander::help(int64_t userId)
  {
    std::string response = "Available commands:\n";
    bot_.getApi().sendMessage(userId, response);
    return *this;
  }

  const BotCommander &BotCommander::settings(int64_t userId)
  {
    LOG_DEBUG("User {} wants to change settings\n", userId);
    bot_.getApi().sendMessage(userId, "Not implemented yet.");
    return *this;
  }

}