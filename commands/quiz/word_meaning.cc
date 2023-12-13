#include <random>
#include <future>
#include <thread>
#include "utils.hpp"
#include "botcommander.hpp"
#include "metrics/profiler.hpp"
#include "log.hpp"

namespace Bot
{
  const BotCommander &BotCommander::commandQuizWordMeaning(int64_t userID)
  {
    LOG_DEBUG("User {} wants to train word meaning\n", userID);
    Training::JlptTraining::Ptr training;
    try
    {
      training = std::make_unique<Training::JlptTraining>(5);
    }
    catch (const std::runtime_error &e)
    {
      bot_.getApi().sendMessage(userID, std::format("Error: {}", e.what()));
      LOG_DEBUG("Error: {}\n", e.what());
      return *this;
    }
    std::string exampleWord = training->getRandomWord();
    if (exampleWord.empty())
    {
      LOG_DEBUG("Couldn't get random word\n");
      return *this;
    }
    auto translations = training->prepareQuizTranslationsForWord(exampleWord, 4);
    if (translations.empty())
    {
      LOG_DEBUG("Couldn't get translations for word {}\n", exampleWord);
      return *this;
    }
    auto wordTranslations = training->getWordTranslations(exampleWord);
    if (wordTranslations.empty())
    {
      LOG_DEBUG("Couldn't get word translations for word {}\n", exampleWord);
      return *this;
    }
    int index = -1;
    for (const auto &t : wordTranslations)
    {
      index = findMatchingIndex(translations, t);
      if (index != -1)
        break;
    }

    LOG_DEBUG("Word: {}, Matching index: {} [{}]\n", exampleWord, index, translations[index]);
    bot_.getApi().sendMessage(userID, "What does this mean?", false, 0, std::make_shared<TgBot::GenericReply>(), "Markdown");
    bot_.getApi().sendPoll(userID, exampleWord, translations, false, 0, std::make_shared<TgBot::GenericReply>(), false, "quiz", false, index);
    commandStack_[userID] = BotCommand::gameMeaning;
    LOG_DEBUG("Finished quiz word meaning\n");
    return *this;
  }
}