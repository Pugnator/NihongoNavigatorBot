#pragma once
#include <tgbot/tgbot.h>
#include <SQLiteCpp/SQLiteCpp.h>

namespace Bot
{
  class UserManager
  {
  public:
    using Ptr = std::unique_ptr<UserManager>;

    UserManager(TgBot::Bot& bot, const std::string& dbFilePath = "bot_stats.db3");
    ~UserManager() = default;
    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;    

    void createUserEntry(int64_t username);
    bool userExists(int64_t username);

  private:
    std::unique_ptr<SQLite::Database> db;
    TgBot::Bot& bot_;
  };
}