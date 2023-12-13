#include "botcommander.hpp"
#include "log.hpp"

namespace Bot
{
  void BotCommander::parseCallback(const TgBot::CallbackQuery::Ptr &query)
  {
    int64_t userID = query->from->id;
    if (query->from->isBot)
    {
      LOG_DEBUG("Skipping bot message\n");
      return;
    }

    int64_t chatID = query->message->chat->id;
    if (chatID < 0)
    {
      bot_.getApi().sendMessage(chatID, "Groups are not supported yet.");
      return;
    }
    LOG_DEBUG("User {} callback {}\n", userID, query->data);

    if (StringTools::startsWith(query->data, "Kana reading"))
    {
      std::thread([this, userID]()
                  { this->commandQuizKanaReading(userID); })
          .detach();
    }
    else if (StringTools::startsWith(query->data, "Word reading"))
    {
      std::thread([this, userID]()
                  { this->commandQuizWordReading(userID); })
          .detach();
    }
    else if (StringTools::startsWith(query->data, "Word meaning"))
    {
      std::thread([this, userID]()
                  { this->commandQuizWordMeaning(userID); })
          .detach();
    }
    else if (StringTools::startsWith(query->data, "Listening"))
    {
      std::thread([this, userID]()
                  { this->commandQuizListening(userID); })
          .detach();
    }
    else if (StringTools::startsWith(query->data, "Numerals"))
    {
      std::thread([this, userID]()
                  { this->commandQuizNumeralsRandomAsync(userID); })
          .detach();
    }
    else if (StringTools::startsWith(query->data, "Random test"))
    {
      std::thread([this, userID]()
                  { this->commandQuizRandomAsync(userID); })
          .detach();
    }
    else if (StringTools::startsWith(query->data, "Stop"))
    {
      if (userReplyStack_.find(userID) != userReplyStack_.end() && userReplyStack_[userID] == UserReply::waiting)
      {
        LOG_DEBUG("User {} wants to stop\n", userID);
        userReplyStack_[userID] = UserReply::stop;
      }
      commandStack_.erase(userID);
      bot_.getApi().sendMessage(userID, "Done.");
      return;
    }
    else if (StringTools::startsWith(query->data, "One more"))
    {
      LOG_DEBUG("User {} wants to continue\n", userID);
      if (userReplyStack_.find(userID) != userReplyStack_.end() && userReplyStack_[userID] == UserReply::waiting)
      {
        LOG_DEBUG("User {} reply stack is waiting\n", userID);
        userReplyStack_[userID] = UserReply::goon;
        return;
      }

      LOG_DEBUG("User {} reply stack is not waiting\n", userID);
      if (commandStack_[userID] == BotCommand::gameKanaReading)
      {
        LOG_DEBUG("User {} wants to continue quizKanaReading\n", userID);
        std::thread([this, userID]()
                    { this->commandQuizKanaReading(userID); })
            .detach();
      }
      else if (commandStack_[userID] == BotCommand::gameMeaning)
      {
        LOG_DEBUG("User {} wants to continue quizMeaning\n", userID);
        std::thread([this, userID]()
                    { this->commandQuizWordMeaning(userID); })
            .detach();
      }
      else if (commandStack_[userID] == BotCommand::gameReading)
      {
        LOG_DEBUG("User {} wants to continue quizReading\n", userID);
        std::thread([this, userID]()
                    { this->commandQuizWordReading(userID); })
            .detach();
      }
      else if (commandStack_[userID] == BotCommand::gameAudition)
      {
        LOG_DEBUG("User {} wants to continue Audition\n", userID);
        std::thread([this, userID]()
                    { this->commandQuizListening(userID); })
            .detach();
      }
      else if (commandStack_[userID] == BotCommand::gameNumerals)
      {
        LOG_DEBUG("User {} wants to continue quizNumerals\n", userID);
        std::thread([this, userID]()
                    { this->commandQuizNumeralsRandomAsync(userID); })
            .detach();
      }
      else
      {
        LOG_DEBUG("User {} wants to continue unknown command\n", userID);
        bot_.getApi().sendMessage(userID, "Nothing to continue. Select new command.", false, 0, std::make_shared<TgBot::GenericReply>(), "Markdown");
      }
    }
    else
    {
      if (commandStack_[userID] == BotCommand::gameNumerals)
      {
        std::thread([this, query]()
                    { this->commandQuizNumeralsCallback(query->from->id, query->data); })
            .detach();
      }
      else
      {
        LOG_DEBUG("Unknown callback {}\n", query->data);
      }
    }
  }

  void BotCommander::handleQuizReply(TgBot::PollAnswer::Ptr answer)
  {
    LOG_DEBUG("Received reply from user {} for quiz: {}\n", answer->user->id, answer->optionIds[0]);
  }
}
