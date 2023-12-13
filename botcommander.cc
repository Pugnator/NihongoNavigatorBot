#include "botcommander.hpp"
#include <curl/curl.h>
#include "metrics/profiler.hpp"
#include "log.hpp"

#include <unordered_set>

inline constexpr const char *const SQL_OPTIONS =
    "PRAGMA main.page_size = 4096;"
    "PRAGMA main.cache_size=-4096;"
    "PRAGMA main.synchronous=OFF;"
    "PRAGMA foreign_keys = ON;"
    "PRAGMA main.journal_mode=WAL;"
    "PRAGMA main.temp_store=FILE;";

namespace Bot
{
  BotCommander::BotCommander(TgBot::Bot &bot)
      : bot_(bot)
  {
    try
    {
      db_ = std::make_unique<SQLite::Database>(
          "audio_cache.db3",
          SQLite::OPEN_READWRITE |
              SQLite::OPEN_CREATE |
              SQLite::OPEN_FULLMUTEX);
      db_->exec(SQL_OPTIONS);
      db_->exec("CREATE TABLE IF NOT EXISTS AudioCache(ID INTEGER PRIMARY KEY AUTOINCREMENT, AudioID INT UNIQUE, TatoebaID INT UNIQUE);");
    }
    catch (const SQLite::Exception &e)
    {
      LOG_EXCEPTION("SQlite exception", e);
    }

    userManager_ = std::make_unique<Bot::UserManager>(bot_);

    quizKeyboard_ = std::make_shared<TgBot::InlineKeyboardMarkup>();
    createInlineKeyboard({{"Kana reading", "Word reading"}, {"Word meaning", "Listening"}, {"Numerals", "Random test"}}, quizKeyboard_);

    continueKeyboard_ = std::make_shared<TgBot::InlineKeyboardMarkup>();
    createInlineKeyboard({{"One more"}, {"Stop"}}, continueKeyboard_);

    difficultyLevelKeyboard_ = std::make_shared<TgBot::InlineKeyboardMarkup>();
    createInlineKeyboard({{"I'm Too Young to Die"}, {"Hurt Me Plenty"}, {"Ultra Violence"}, {"Unthinkable"}}, difficultyLevelKeyboard_);

    numeralKeyboard_ = std::make_shared<TgBot::InlineKeyboardMarkup>();
    createInlineKeyboard({{
                              "一",
                              "二",
                              "三",
                              "四",
                              "五",
                          },
                          {"六", "七", "八", "九", "十"},
                          {"百", "千", "万", "零", "="}},
                         numeralKeyboard_);

    search_ = std::make_shared<Search::DictSearch>();

    bot_.getEvents().onPollAnswer([this](TgBot::PollAnswer::Ptr answer)
                                  { 
                              processQuizReplies(answer);
                              LOG_DEBUG("Poll answer received: {}\n", answer->user->id);                              
                              this->userReplyStack_.erase(answer->user->id);
                              this->bot_.getApi().sendMessage(answer->user->id, "Continue?", false, 0, continueKeyboard_); });
  }

  const BotCommander &BotCommander::wordOfDay(int64_t userId)
  {
    return *this;
  }

  void BotCommander::createOneColumnKeyboard(const std::vector<std::string> &buttonStrings, TgBot::ReplyKeyboardMarkup::Ptr &kb)
  {
    for (size_t i = 0; i < buttonStrings.size(); ++i)
    {
      std::vector<TgBot::KeyboardButton::Ptr> row;
      TgBot::KeyboardButton::Ptr button = std::make_shared<TgBot::KeyboardButton>();
      button->text = buttonStrings[i];
      row.push_back(button);
      kb->keyboard.push_back(row);
    }
  }

  void BotCommander::createKeyboard(const std::vector<std::vector<std::string>> &buttonLayout, TgBot::ReplyKeyboardMarkup::Ptr &kb)
  {
    try
    {
      for (size_t i = 0; i < buttonLayout.size(); ++i)
      {
        std::vector<TgBot::KeyboardButton::Ptr> row;
        for (size_t j = 0; j < buttonLayout[i].size(); ++j)
        {
          TgBot::KeyboardButton::Ptr button = std::make_shared<TgBot::KeyboardButton>();
          button->text = buttonLayout[i][j];
          row.push_back(button);
        }
        kb->keyboard.push_back(row);
      }
    }
    catch (const TgBot::TgException &e)
    {
      LOG_EXCEPTION("Exception while creating a reply keyboard", e);
    }
  }

  void BotCommander::createInlineKeyboard(const std::vector<std::vector<std::string>> &buttonLayout, TgBot::InlineKeyboardMarkup::Ptr &kb)
  {
    try
    {
      for (size_t i = 0; i < buttonLayout.size(); ++i)
      {
        std::vector<TgBot::InlineKeyboardButton::Ptr> row;
        for (size_t j = 0; j < buttonLayout[i].size(); ++j)
        {
          TgBot::InlineKeyboardButton::Ptr button = std::make_shared<TgBot::InlineKeyboardButton>();
          button->text = buttonLayout[i][j];
          button->callbackData = buttonLayout[i][j];
          row.push_back(button);
        }
        kb->inlineKeyboard.push_back(row);
      }
    }
    catch (const TgBot::TgException &e)
    {
      LOG_EXCEPTION("Exception while creating an inline keyboard", e);
    }
  }

  UserReply BotCommander::waitForUserReply(int64_t userID)
  {
    bot_.getApi().sendMessage(userID, "Show more?", false, 0, continueKeyboard_);
    userReplyStack_[userID] = UserReply::waiting;
    const auto start_time = std::chrono::steady_clock::now();
    const auto timeout_duration = std::chrono::milliseconds(10000);

    while (userReplyStack_[userID] != UserReply::goon && userReplyStack_[userID] != UserReply::stop)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (std::chrono::steady_clock::now() - start_time > timeout_duration)
      {
        LOG_DEBUG("Timeout while waiting for user reply\n");
        userReplyStack_.erase(userID);
        bot_.getApi().sendMessage(userID, "Timeout. Stopping.", false, 0, std::make_shared<TgBot::GenericReply>(), "Markdown");
        return UserReply::stop;
      }
    }

    auto it = userReplyStack_.find(userID);
    if (it == userReplyStack_.end())
    {
      LOG_DEBUG("User ID not found in userReplyStack\n");
      return UserReply::stop;
    }

    UserReply reply = it->second;
    userReplyStack_.erase(it);
    return reply;
  }

  void BotCommander::registerQuizCallback(int64_t userID, ReplyCallback callback)
  {
    std::lock_guard<std::mutex> lock(replyCallbacksMutex);
    LOG_DEBUG("Registering reply callback for user {}\n", userID);
    replyCallbacks_[userID] = callback;
  }

  void BotCommander::unregisterQuizCallback(int64_t userID)
  {
    std::lock_guard<std::mutex> lock(replyCallbacksMutex);
    LOG_DEBUG("Unregistering reply callback for user {}\n", userID);
    replyCallbacks_.erase(userID);
  }

  void BotCommander::processQuizReplies(TgBot::PollAnswer::Ptr answer)
  {
    auto it = replyCallbacks_.find(answer->user->id);
    if (it != replyCallbacks_.end())
    {
      it->second(answer);
      unregisterQuizCallback(answer->user->id);
    }
  }

  std::string BotCommander::getStringToken(const std::string &input, unsigned index)
  {
    std::istringstream iss(input);
    std::string token;

    for (unsigned i = 1; i <= index; ++i)
    {
      iss >> token;
      if (!iss)
      {
        return std::string();
      }
    }

    return token;
  }

  bool BotCommander::storeAudioCache(const std::string &audioID, uint32_t tatoebaID)
  {
    try
    {
      SQLite::Statement stmt(*db_, "INSERT OR IGNORE INTO AudioCache (AudioID, TatoebaID) VALUES (?, ?);");
      LOG_DEBUG("Storing audio cache: {} {}\n", audioID, tatoebaID);
      stmt.bind(1, audioID);
      stmt.bind(2, tatoebaID);
      stmt.exec();
    }
    catch (const std::exception &e)
    {
      LOG_EXCEPTION("Audio Cache exception", e);
      return false;
    }
    return true;
  }

  std::string BotCommander::getAudioCache(uint32_t tatoebaID)
  {
    try
    {
      SQLite::Statement stmt(*db_, "SELECT AudioID FROM AudioCache WHERE TatoebaID = ?");
      stmt.bind(1, tatoebaID);
      if (stmt.executeStep())
      {
        return stmt.getColumn("AudioID").getText();
      }
    }
    catch (const std::exception &e)
    {
      LOG_EXCEPTION("Audio Cache exception", e);
    }
    return std::string();
  }

  size_t BotCommander::downloadCallback(void *ptr, size_t size, size_t nmemb, void *stream)
  {
    if (!stream)
    {
      return size * nmemb;
    }
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
  }

  std::string BotCommander::downloadURL(const std::string &url, const std::string &targetFilename, bool force)
  {
    RECORD_CALL();
    if (targetFilename.empty())
    {
      LOG_DEBUG("Target file for downloading is empty.\n");
      return std::string();
    }

    if (!force && std::filesystem::exists(targetFilename))
    {
      LOG_DEBUG("Target file {} already exists and no force flag was specified\n", targetFilename);
      return targetFilename;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, &downloadCallback);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, true);
    char errbuf[CURL_ERROR_SIZE] = {0};
    curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0...");

    LOG_DEBUG("Downloading {} to {}\n", url, targetFilename);
    errno = 0;
    FILE *targetFile = fopen(targetFilename.c_str(), "wb");
    if (!targetFile)
    {
      LOG_DEBUG("Error occurred while opening file.\n");
      return std::string();
    }
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, targetFile);
    const int max_attempts = 5;
    CURLcode retcode = CURLE_FAILED_INIT;

    for (int i = 1; i <= max_attempts; i++)
    {
      try
      {
        retcode = curl_easy_perform(curl_handle);
        long response_code = 0;
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
        LOG_INFO("HTTP Response Code: {}\n", response_code);
        if (retcode == CURLE_OK)
        {
          break;
        }
        LOG_INFO("CURL error: {0}\nRetrying...\n", (uint32_t)retcode);
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
      }
      catch (...)
      {
        LOG_INFO("Exception occurred while downloading file.\n");
        break;
      }
    }

    if (CURLE_OK != retcode)
    {
      LOG_INFO("CURL error: {0}\n", errbuf);
    }
    fclose(targetFile);

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    return CURLE_OK == retcode ? targetFilename : std::string();
  }

  std::string BotCommander::escapeMarkdownV2(const std::string &input)
  {
    std::unordered_set<char> mdSpecialChars = {
        '_', '*', '[', ']', '(', ')', '~', '`', '>', '#', '+', '-', '=', '|', '{', '}', '.', '!'};

    std::string result;
    for (char c : input)
    {
      if (mdSpecialChars.count(c))
      {
        result += '\\'; // Add escape character
      }
      result += c;
    }

    return result;
  }
}