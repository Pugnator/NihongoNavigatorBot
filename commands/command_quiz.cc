#include <random>
#include <future>
#include <thread>
#include "utils.hpp"
#include "botcommander.hpp"
#include "metrics/profiler.hpp"
#include "log.hpp"

namespace Bot
{
  bool containsAny(const std::vector<std::string> &list1, const std::vector<std::string> &list2)
  {
    for (const auto &item : list1)
    {
      if (std::find(list2.begin(), list2.end(), item) != list2.end())
      {
        return true;
      }
    }
    return false;
  }

  int findMatchingIndex(const std::vector<std::string> &where, const std::string &what)
  {
    for (size_t i = 0; i < where.size(); ++i)
    {
      if (where[i] == what)
      {
        return i;
      }
    }
    return -1;
  }

  void BotCommander::commandQuizRandomAsync(int64_t userID)
  {
    auto future = std::async(std::launch::async, &BotCommander::commandQuizRandom, this, userID);
  }

  const BotCommander &BotCommander::commandQuizRandom(int64_t userID)
  {
    LOG_DEBUG("User {} wants to train random quiz\n", userID);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 5);
    int choice = dis(gen);
    LOG_DEBUG("Random choice: {}\n", choice);
    if (choice == 0)
    {
      std::thread([this, userID]()
                  { this->commandQuizWordMeaning(userID); })
          .detach();
    }
    else if (choice == 1)
    {
      std::thread([this, userID]()
                  { this->commandQuizKanaReading(userID); })
          .detach();
    }
    else if (choice == 2)
    {
      std::thread([this, userID]()
                  { this->commandQuizWordReading(userID); })
          .detach();
    }
    else if (choice == 3)
    {
      std::thread([this, userID]()
                  { this->commandQuizJapaneseNumerals(userID); })
          .detach();
    }
    else if (choice == 4)
    {
      std::thread([this, userID]()
                  { this->commandQuizNumeralCounters(userID); })
          .detach();
    }
    else if (choice == 5)
    {
      std::thread([this, userID]()
                  { this->commandQuizListening(userID); })
          .detach();
    }
    return *this;
  }
}
