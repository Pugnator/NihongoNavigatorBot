#include <random>
#include <future>
#include <thread>
#include "utils.hpp"
#include "botcommander.hpp"
#include "metrics/profiler.hpp"
#include "log.hpp"

namespace Bot
{
  const BotCommander &BotCommander::commandQuizWordReading(int64_t userID)
  {
    LOG_DEBUG("User {} wants to train word reading\n", userID);
    Training::JlptTraining::Ptr training;
    try
    {
      training = std::make_unique<Training::JlptTraining>(5);
    }
    catch (const std::runtime_error &e)
    {
      bot_.getApi().sendMessage(userID, std::format("Error: {}", e.what()));
      LOG_DEBUG("Error: {}\n", e.what());
      bot_.getApi().sendMessage(userID, "BUG: Failed to create training");
      return *this;
    }
    std::string exampleWord = training->getRandomWord();
    if (exampleWord.empty())
    {
      LOG_DEBUG("Couldn't get random word\n");
      bot_.getApi().sendMessage(userID, "BUG: Couldn't get random word");
      return *this;
    }

    auto readings = training->prepareQuizReadingsForWord(exampleWord, 4);
    if (readings.empty())
    {
      LOG_DEBUG("Couldn't get readings for word {}\n", exampleWord);
      bot_.getApi().sendMessage(userID, "BUG: Couldn't get readings for word");
      return *this;
    }

    auto wordReadings = training->getWordReadings(exampleWord);
    if (wordReadings.empty())
    {
      LOG_DEBUG("Couldn't get word readings for word {}\n", exampleWord);
      bot_.getApi().sendMessage(userID, "BUG: Couldn't get word readings");
      return *this;
    }

    int index = 1;
    for (const auto &r : wordReadings)
    {
      index = findMatchingIndex(readings, r);
      if (index != -1)
        break;
    }

    if (index == -1)
    {
      LOG_DEBUG("Error: No matching index found\n");
      bot_.getApi().sendMessage(userID, "BUG: No matching index found");
      return *this;
    }

    LOG_DEBUG("Word: {}, Matching index: {} [{}]\n", exampleWord, index, readings[index]);
    bot_.getApi().sendMessage(userID, "_How does this read?_", false, 0, std::make_shared<TgBot::GenericReply>(), "Markdown");
    bot_.getApi().sendPoll(userID, exampleWord, readings, false, 0, std::make_shared<TgBot::GenericReply>(), false, "quiz", false, index);
    commandStack_[userID] = BotCommand::gameReading;
    registerQuizCallback(userID, [this](TgBot::PollAnswer::Ptr answer)
                         { handleQuizReply(answer); });
    LOG_DEBUG("Finished quiz word reading\n");
    return *this;
  }
}