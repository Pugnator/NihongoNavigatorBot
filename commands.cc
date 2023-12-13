#include <future>
#include <thread>

#include "botcommander.hpp"
#include "log.hpp"

namespace Bot
{
  void BotCommander::parseCommand(const TgBot::Message::Ptr &message)
  {
    int64_t userID = message->from->id;
    if (message->from->isBot)
    {
      LOG_DEBUG("Skipping bot message\n");
      return;
    }

    LOG_INFO("User {} commanded {}\n", userID, message->text);

    // Global commands
    if (StringTools::startsWith(message->text, "/start"))
    {
      start(userID);
      return;
    }
    if (!userManager_->userExists(userID))
    {
      try
      {
        bot_.getApi().sendMessage(userID, "You are not registered. Use /start to register.");
      }
      catch (const TgBot::TgException &e)
      {
        if (StringTools::startsWith(e.what(), "Forbidden:"))
        {
          LOG_DEBUG("User {} has blocked the bot\n", userID);
          return;
        }
      }

      return;
    }

    if (StringTools::startsWith(message->text, "/help"))
    {
      help(userID);
      return;
    }
    else if (StringTools::startsWith(message->text, "/settings"))
    {
      settings(userID);
      return;
    }

    // Custom commands
    else if (StringTools::startsWith(message->text, "/search"))
    {
      std::thread([this, message]()
                  { this->commandSearchWord(message); })
          .detach();
    }
    else if (StringTools::startsWith(message->text, "/example"))
    {
      std::thread([this, message]()
                  { this->commandSearchExample(message); })
          .detach();
    }
    else if (StringTools::startsWith(message->text, "/quiz"))
    {
      bot_.getApi().sendMessage(userID, "Choose what to train", false, 0, quizKeyboard_);
    }
    else if (StringTools::startsWith(message->text, "/info_word"))
    {
      LOG_DEBUG("User {} wants to explain a record\n", userID);
      std::thread([this, message]()
                  { this->commandWordAllInfo(message); })
          .detach();
      return;
    }
    else
    {
      bot_.getApi().sendMessage(userID, "Unknown command. /help for help.");
    }
  }

  void BotCommander::parseUserInput(const TgBot::Message::Ptr &message)
  {
    int64_t userID = message->from->id;
    if (message->from->isBot)
    {
      LOG_DEBUG("Skipping bot message\n");
      return;
    }

    int64_t chatID = message->chat->id;
    if (chatID < 0)
    {
      bot_.getApi().sendMessage(chatID, "Groups are not supported yet.");
      return;
    }
    LOG_DEBUG("User {} input {}\n", userID, message->text);
    if (commandStack_.find(userID) == commandStack_.end())
    {
      bot_.getApi().sendMessage(userID, "Give me a command first. Use Menu or direct commands. /help for help.");
      return;
    }

    if (commandStack_[userID] == BotCommand::searchSingleWord)
    {
      std::thread([this, message]()
                  { this->commandSearchWord(message); })
          .detach();
    }
    else if (commandStack_[userID] == BotCommand::searchExample)
    {
      std::thread([this, message]()
                  { this->commandSearchExample(message); })
          .detach();
    }
    else if (commandStack_[userID] == BotCommand::gameKanaReading)
    {
      std::thread([this, message]()
                  { this->commandQuizKanaReading(message); })
          .detach();
    }
    else
    {
      LOG_DEBUG("User input: {}\n", message->text);
    }
  }

  void BotCommander::parseUserInputAsync(const TgBot::Message::Ptr &message)
  {
    auto future = std::async(std::launch::async, &BotCommander::parseUserInput, this, message);
  }

  void BotCommander::parseInlineQuery(const TgBot::InlineQuery::Ptr &query)
  {
    LOG_DEBUG("User {} inline query {}\n", query->from->id, query->query);
  }

  void BotCommander::parseChosenInlineResult(const TgBot::ChosenInlineResult::Ptr &result)
  {
    LOG_DEBUG("User {} chose inline result {}\n", result->from->id, result->resultId);
  }

  void BotCommander::parseEditedMessage(const TgBot::Message::Ptr &message)
  {
    LOG_DEBUG("User {} edited message {}\n", message->from->id, message->text);
  }
}