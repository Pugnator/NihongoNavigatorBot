#include <random>
#include <future>
#include <thread>
#include "utils.hpp"
#include "botcommander.hpp"
#include "metrics/profiler.hpp"
#include "log.hpp"

namespace
{
  uint64_t initialNumber = 0;
  std::string correctNumeralString;
  TgBot::Message::Ptr inputMessage;
  TgBot::Message::Ptr outputMessage;
}
namespace Bot
{

  const BotCommander &BotCommander::commandQuizJapaneseNumerals(int64_t userID)
  {
    LOG_DEBUG("User {} is training japanese numerals\n", userID);

    Training::JlptTraining::Ptr training = std::make_unique<Training::JlptTraining>(5);
    initialNumber = training->getRandomNumber();
    correctNumeralString = training->getNumberString(initialNumber);
    LOG_DEBUG("Initial number: {} = [{}]\n", initialNumber, correctNumeralString);

    inputMessage = bot_.getApi().sendMessage(userID, std::format("Spell {}", initialNumber), false, 0, numeralKeyboard_);
    outputMessage = bot_.getApi().sendMessage(userID, "Reply:");
    return *this;
  }

  const BotCommander &BotCommander::commandQuizNumeralCounters(int64_t userID)
  {
    Training::JlptTraining::Ptr training = std::make_unique<Training::JlptTraining>(5);
    uint32_t id = training->getRandomCounterSuffix();
    std::vector<std::string> kanjies;
    unsigned maxCount = 20;
    while (maxCount--)
    {
      kanjies = search_->jmdict->kanji_by_id(id);
      if (!kanjies.empty())
        break;
      id = training->getRandomCounterSuffix();
    }

    if (kanjies.empty())
    {
      LOG_DEBUG("No kanji for counter {}\n", id);
      bot_.getApi().sendMessage(userID, "No kanji for counter, it's probably a bug.");
      return *this;
    }

    std::string counterKanji = kanjies.front();
    std::vector<std::string> counterMeanings = training->getCounterDescription(id);
    if (counterMeanings.empty())
    {
      LOG_DEBUG("No meanings for counter {}\n", counterKanji);
      bot_.getApi().sendMessage(userID, "No meanings for counter, it's probably a bug.");
      return *this;
    }
    std::string correctTranslation = counterMeanings.front();

    unsigned optionsCount = 0;
    std::vector<uint32_t> ids;
    while (optionsCount < 3)
    {
      uint32_t idOptional = training->getRandomCounterSuffix();
      auto gloss = training->getCounterDescription(idOptional);
      if (gloss.empty())
        continue;

      ids.push_back(idOptional);
      optionsCount++;
    }
    ids.push_back(id);
    std::shuffle(ids.begin(), ids.end(), std::mt19937(std::random_device()()));
    uint32_t index = std::distance(ids.begin(), std::find(ids.begin(), ids.end(), id));
    std::vector<std::string> translations;
    for (auto id : ids)
    {
      auto meanings = training->getCounterDescription(id);
      if (meanings.empty())
      {
        LOG_DEBUG("No meanings for counter {}\n", id);
        continue;
      }
      translations.push_back(meanings.front());
    }

    std::string correctCounter = training->getNumberString(1) + counterKanji;
    bot_.getApi().sendPoll(userID, correctCounter, translations, false, 0, std::make_shared<TgBot::GenericReply>(), false, "quiz", false, index);
    return *this;
  }

  const BotCommander &BotCommander::commandQuizNumeralsCallback(int64_t userID, const std::string &data)
  {
    static std::string userReplyText;
    LOG_DEBUG("User {} is inputting numerals: {}\n", userID, data);

    if (data == "=")
    {
      userReplyText = "";
      bot_.getApi().deleteMessage(userID, inputMessage->messageId);
      if (userReplyText == correctNumeralString)
      {
        bot_.getApi().sendMessage(userID, "Correct!");
      }
      else
      {
        bot_.getApi().sendMessage(userID, std::format("Wrong! Correct answer is {} = {}", initialNumber, correctNumeralString));
      }
      inputMessage.reset();
      outputMessage.reset();
      bot_.getApi().sendMessage(userID, "Continue?", false, 0, continueKeyboard_);
      return *this;
    }
    else
    {
      userReplyText += data;
      while (true)
      {
        std::string userInputformatted = std::format("Reply:{}", userReplyText);
        auto result = bot_.getApi().editMessageText(userInputformatted, userID, outputMessage->messageId);
        if (result != nullptr)
        {
          LOG_DEBUG("Edited message\n");
          break;
        }
        else
        {
          LOG_DEBUG("Failed to edit message\n");
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }
    }
    return *this;
  }

  const BotCommander &BotCommander::commandQuizNumeralsRandomAsync(int64_t userID)
  {
    LOG_DEBUG("User {} wants to train numerals quiz\n", userID);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);
    int choice = dis(gen);
    LOG_DEBUG("Random choice: {}\n", choice);
    commandStack_[userID] = BotCommand::gameNumerals;
    if (choice == 0)
    {
      std::thread([this, userID]()
                  { this->commandQuizJapaneseNumerals(userID); })
          .detach();
    }
    else if (choice == 1)
    {
      std::thread([this, userID]()
                  { this->commandQuizNumeralCounters(userID); })
          .detach();
    }
    return *this;
  }
}