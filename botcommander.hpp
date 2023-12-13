#pragma once
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <tgbot/tgbot.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include "usermanager.hpp"
#include "waka.hpp"

namespace Bot
{
  bool containsAny(const std::vector<std::string> &list1, const std::vector<std::string> &list2);
  int findMatchingIndex(const std::vector<std::string> &where, const std::string &what);

  enum class BotCommand
  {
    none,
    searchSingleWord,
    searchExample,
    explainInput,
    gameKanaReading,
    gameAudition,
    gameMeaning,
    gameReading,
    gameNumerals,
    stop,
    outputPause,
  };

  enum class UserReply
  {
    none,
    waiting,
    goon,
    stop
  };

  enum class DifficultyLevel
  {
    easy,
    medium,
    hard,
    ultra,
  };

  using CommandStack = std::unordered_map<int64_t /*userId*/, BotCommand>;
  using UserReplyStack = std::unordered_map<int64_t /*userId*/, UserReply>;

  class BotCommander
  {
  public:
    using Ptr = std::unique_ptr<BotCommander>;
    using ReplyCallback = std::function<void(TgBot::PollAnswer::Ptr answer)>;

    BotCommander(TgBot::Bot &bot);
    ~BotCommander() = default;
    BotCommander(const BotCommander &) = delete;
    BotCommander &operator=(const BotCommander &) = delete;

    void parseCommand(const TgBot::Message::Ptr &message);
    void parseCallback(const TgBot::CallbackQuery::Ptr &query);
    void parseUserInput(const TgBot::Message::Ptr &message);
    void parseUserInputAsync(const TgBot::Message::Ptr &message);
    void parseInlineQuery(const TgBot::InlineQuery::Ptr &query);
    void parseChosenInlineResult(const TgBot::ChosenInlineResult::Ptr &result);
    void parseEditedMessage(const TgBot::Message::Ptr &message);
    const BotCommander &start(int64_t userId);
    const BotCommander &help(int64_t userId);
    const BotCommander &settings(int64_t userId);
    const BotCommander &wordOfDay(int64_t userId);

    void handleQuizReply(TgBot::PollAnswer::Ptr answer);
    void processQuizReplies(TgBot::PollAnswer::Ptr answer);

  private:
    std::string getStringToken(const std::string &str, unsigned index);
    const BotCommander &commandSearchWord(const TgBot::Message::Ptr &query);
    const BotCommander &commandSearchExample(const TgBot::Message::Ptr &query);

    const BotCommander &commandWordAllInfo(const TgBot::Message::Ptr &query);

    const BotCommander &commandQuizKanaReading(int64_t userID);
    const BotCommander &commandQuizKanaReading(TgBot::Message::Ptr message);
    const BotCommander &commandQuizWordReading(int64_t userID);
    const BotCommander &commandQuizWordMeaning(int64_t userID);
    const BotCommander &commandQuizListening(int64_t userID);
    const BotCommander &commandQuizNumeralsRandomAsync(int64_t userID);
    const BotCommander &commandQuizJapaneseNumerals(int64_t userID);
    const BotCommander &commandQuizNumeralCounters(int64_t userID);
    const BotCommander &commandQuizNumeralsCallback(int64_t userID, const std::string &data);
    const BotCommander &commandQuizRandom(int64_t userID);
    void commandQuizRandomAsync(int64_t userID);

    void registerQuizCallback(int64_t userID, ReplyCallback callback);
    void unregisterQuizCallback(int64_t userID);

    void createOneColumnKeyboard(const std::vector<std::string> &buttonStrings, TgBot::ReplyKeyboardMarkup::Ptr &kb);
    void createKeyboard(const std::vector<std::vector<std::string>> &buttonLayout, TgBot::ReplyKeyboardMarkup::Ptr &kb);
    void createInlineKeyboard(const std::vector<std::vector<std::string>> &buttonLayout, TgBot::InlineKeyboardMarkup::Ptr &kb);
    static size_t downloadCallback(void *ptr, size_t size, size_t nmemb, void *stream);
    UserReply waitForUserReply(int64_t userID);

  private:
    std::string escapeMarkdownV2(const std::string &input);
    std::string downloadURL(const std::string &url, const std::string &targetFilename, bool force);
    bool storeAudioCache(const std::string &audioID, uint32_t tatoebaID);
    std::string getAudioCache(uint32_t tatoebaID);

    TgBot::Bot &bot_;
    std::unique_ptr<SQLite::Database> db_;
    DifficultyLevel difficultyLevel_ = DifficultyLevel::easy;
    Bot::UserManager::Ptr userManager_;

    TgBot::InlineKeyboardMarkup::Ptr quizKeyboard_;
    TgBot::InlineKeyboardMarkup::Ptr continueKeyboard_;
    TgBot::InlineKeyboardMarkup::Ptr difficultyLevelKeyboard_;
    TgBot::InlineKeyboardMarkup::Ptr numeralKeyboard_;

    CommandStack commandStack_;
    UserReplyStack userReplyStack_;

    Search::DictSearch::Ptr search_;
    std::string quizRandomKana_;

    std::map<int64_t, ReplyCallback> replyCallbacks_;
    std::mutex replyCallbacksMutex;
  };
}