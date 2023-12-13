#include <random>
#include <future>
#include <thread>
#include "utils.hpp"
#include "botcommander.hpp"
#include "metrics/profiler.hpp"
#include "log.hpp"

namespace Bot
{
  const BotCommander &BotCommander::commandQuizListening(int64_t userID)
  {
    LOG_DEBUG("User {} wants to train a listening\n", userID);

    Training::JlptTraining::Ptr training;
    try
    {
      training = std::make_unique<Training::JlptTraining>();
    }
    catch (const std::runtime_error &e)
    {
      bot_.getApi().sendMessage(userID, std::format("Error: {}", e.what()));
      LOG_DEBUG("Error: {}\n", e.what());
      return *this;
    }

    uint32_t exampleID = training->getRandomAudioExampleIDForLevel();
    if (!exampleID)
    {
      LOG_DEBUG("Couldn't get random audio example\n");
      return *this;
    }
    LOG_DEBUG("Example ID selected: {}\n", exampleID);

    uint32_t audioID = search_->example->audio_for_example(exampleID);
    const std::string performer = search_->example->author_for_audio_example(audioID);
    const std::string license = search_->example->license_for_audio_example(audioID);
    const std::string exampleText = search_->example->tatoeba_example(exampleID);
    auto engTranslations = search_->example->tatoeba_translation_eng(exampleID);
    // select random translation    
    std::string engTranslation;
    if (!engTranslations.empty())
    {
      engTranslation = engTranslations[std::rand() % engTranslations.size()];
    }
    auto rusTranslations = search_->example->tatoeba_translation_rus(exampleID);
    std::string rusTranslation;
    if (!rusTranslations.empty())
    {
      rusTranslation = rusTranslations[std::rand() % rusTranslations.size()];
    }

    const std::string cacheID = getAudioCache(exampleID);
    const std::string licenseText = std::format("This work by {} is licensed under {}.", performer.empty() ? "Anonymous" : performer, license.empty() ? "CC BY 4.0" : license);
    bot_.getApi().sendMessage(userID, std::format("Japanese: ||{}||", escapeMarkdownV2(exampleText)), false, 0, std::make_shared<TgBot::GenericReply>(), "MarkdownV2");
    if(!engTranslation.empty())
      bot_.getApi().sendMessage(userID, std::format("English: ||{}||", escapeMarkdownV2(engTranslation)), false, 0, std::make_shared<TgBot::GenericReply>(), "MarkdownV2");
    if (!rusTranslation.empty())
      bot_.getApi().sendMessage(userID, std::format("Russian: ||{}||", escapeMarkdownV2(rusTranslation)), false, 0, std::make_shared<TgBot::GenericReply>(), "MarkdownV2");

    if (!cacheID.empty())
    {
      LOG_DEBUG("Found audio in cache: {}\n", cacheID);
      auto message = bot_.getApi().sendAudio(userID, cacheID, licenseText, 0, performer, "Tatoeba");
      if (message)
      {
        LOG_DEBUG("Sent audio message with ID {}\n", message->audio->fileId);
      }
      else
      {
        LOG_DEBUG("Failed to send audio message\n");
      }
      bot_.getApi().sendMessage(message->chat->id, "Continue?", false, 0, continueKeyboard_);
      commandStack_[message->chat->id] = BotCommand::gameAudition;
      return *this;
    }

    std::string audioURL = search_->example->url_for_audio_example(audioID);
    LOG_DEBUG("Audio URL: {}\n", audioURL);

    std::string audioFilename = downloadURL(audioURL, getTempFileName(), true);
    if (audioFilename.empty())
    {
      LOG_DEBUG("Failed to download audio file\n");
      return *this;
    }
    LOG_DEBUG("Audio file: {}\n", audioFilename);
    const std::string audioMimeType = "audio/mpeg";
    auto message = bot_.getApi().sendAudio(userID, TgBot::InputFile::fromFile(audioFilename, audioMimeType), licenseText, 0, performer, "Tatoeba");
    if (message)
    {
      storeAudioCache(message->audio->fileId, audioID);
      LOG_DEBUG("Sent audio message with ID {}\n", message->audio->fileId);
    }
    else
    {
      LOG_DEBUG("Failed to send audio message\n");
    }
    std::remove(audioFilename.c_str());
    bot_.getApi().sendMessage(message->chat->id, "Continue?", false, 0, continueKeyboard_);
    commandStack_[message->chat->id] = BotCommand::gameAudition;
    return *this;
  }
}