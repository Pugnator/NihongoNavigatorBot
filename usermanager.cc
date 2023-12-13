#include "usermanager.hpp"
#include "log.hpp"

inline constexpr const char *const SQL_OPTIONS =
    "PRAGMA main.page_size = 4096;"
    "PRAGMA main.cache_size=-4096;"
    "PRAGMA main.synchronous=OFF;"
    "PRAGMA foreign_keys = ON;"
    "PRAGMA main.journal_mode=WAL;"
    "PRAGMA main.temp_store=MEMORY;";
namespace Bot
{
  UserManager::UserManager(TgBot::Bot &bot, const std::string &dbFilePath)
      : bot_(bot)
  {
    try
    {
      db = std::make_unique<SQLite::Database>(
          dbFilePath,
          SQLite::OPEN_READWRITE |
              SQLite::OPEN_CREATE |
              SQLite::OPEN_FULLMUTEX);
      db->exec(SQL_OPTIONS);
      db->exec("CREATE TABLE IF NOT EXISTS User (ID INTEGER PRIMARY KEY AUTOINCREMENT, UserID INTEGER, Difficulty INTEGER DEFAULT 0, JLPT INTEGER DEFAULT 0)");
      db->exec("CREATE TABLE IF NOT EXISTS Quiz (ID INTEGER PRIMARY KEY AUTOINCREMENT, KanaReadingCorrect INTEGER DEFAULT 0, KanaReadingTotal INTEGER DEFAULT 0, WordReadingCorrect INTEGER DEFAULT 0, WordReadingTotal INTEGER DEFAULT 0, WordMeaningCorrect INTEGER DEFAULT 0, WordMeaningTotal INTEGER DEFAULT 0, RandomCorrect INTEGER DEFAULT 0, RandomTotal INTEGER DEFAULT 0, UserID INTEGER, FOREIGN KEY(UserID) REFERENCES User(ID) ON DELETE CASCADE)");
      db->exec("CREATE TABLE IF NOT EXISTS Settings (UserID INTEGER, Difficulty INTEGER DEFAULT 0, JLPT INTEGER DEFAULT 0)");
    }
    catch (const SQLite::Exception &e)
    {
      LOG_EXCEPTION("SQlite exception", e);
    }
  }

  void UserManager::createUserEntry(int64_t username)
  {
    try
    {
      SQLite::Statement query(*db, "INSERT INTO User (ID) VALUES (?)");
      query.bind(1, username);
      query.exec();
    }
    catch (const SQLite::Exception &e)
    {
      LOG_EXCEPTION("SQlite exception", e);
    }
  }

  bool UserManager::userExists(int64_t username)
  {
    try
    {
      SQLite::Statement query(*db, "SELECT ID FROM User WHERE ID = ?");
      query.bind(1, username);
      return query.executeStep();
    }
    catch (const SQLite::Exception &e)
    {
      LOG_EXCEPTION("SQlite exception", e);
    }
    return false;
  }
}
