#include <random>
#include <future>
#include <thread>
#include "utils.hpp"
#include "botcommander.hpp"
#include "metrics/profiler.hpp"
#include "log.hpp"

namespace Bot
{
  const BotCommander &BotCommander::commandQuizKanaReading(TgBot::Message::Ptr message)
  {
    RECORD_CALL();
    std::string correctAnswer = KanaProc::toRomaji(quizRandomKana_);
    std::string userAnswerRomaji = KanaProc::toRomaji(KanaProc::fromRomaji(message->text));
    LOG_DEBUG("User thinks that {} reads as {}\n", quizRandomKana_, message->text);
    LOG_DEBUG("{} ==> {}\n", correctAnswer, userAnswerRomaji);

    if (correctAnswer == userAnswerRomaji)
    {
      bot_.getApi().sendMessage(message->chat->id, "*Correct!*", false, 0, std::make_shared<TgBot::GenericReply>(), "Markdown");
    }
    else
    {
      bot_.getApi().sendMessage(message->chat->id, std::format("*Wrong!* It reads as *{}*", correctAnswer), false, 0, std::make_shared<TgBot::GenericReply>(), "Markdown");
    }
    bot_.getApi().sendMessage(message->chat->id, "Continue?", false, 0, continueKeyboard_);
    commandStack_[message->chat->id] = BotCommand::gameKanaReading;
    return *this;
  }

  const BotCommander &BotCommander::commandQuizKanaReading(int64_t userID)
  {
    LOG_DEBUG("User {} wants to train kana reading\n", userID);
    bot_.getApi().sendDice(userID, false, 0, std::make_shared<TgBot::GenericReply>(), "🎲", "Markdown");
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

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> intDist(3, 8);
    std::uniform_real_distribution<float> floatDist(0.0, 1.0);
    int randomInt = intDist(gen);
    float randomFloat = floatDist(gen);
    quizRandomKana_ = training->getRandomKanaWord(randomInt, randomFloat);
    std::string question = std::format("Can you read it? *{}*. _Repeat it in romaji_", quizRandomKana_);
    bot_.getApi().sendMessage(userID, question, false, 0, std::make_shared<TgBot::GenericReply>(), "Markdown");
    LOG_DEBUG("Finished quiz kana reading\n");
    commandStack_[userID] = BotCommand::gameKanaReading;

    return *this;
  }
}