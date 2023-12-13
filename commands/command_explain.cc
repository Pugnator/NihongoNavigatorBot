#include "botcommander.hpp"
#include "log.hpp"
#include <algorithm>
#include <numeric>
namespace Bot
{
  const BotCommander &BotCommander::commandWordAllInfo(const TgBot::Message::Ptr &query)
  {
    int64_t userID = query->from->id;
    if (query->from->isBot)
    {
      userID = query->chat->id;
      return *this;
    }

    std::string input;
    if (query->replyToMessage && !query->replyToMessage->text.empty())
    {
      input = getStringToken(query->replyToMessage->text, 1);
    }
    else
    {
      input = getStringToken(query->text, 2);
    }

    if (input.empty())
    {
      bot_.getApi().sendMessage(userID, "No input provided.");
      LOG_DEBUG("No input provided for info_word\n");
      return *this;
    }
    LOG_DEBUG("User {} wants info regarding the word {}\n", userID, input);
    Search::SearchRequest::Ptr searchRequestUnique = std::make_unique<Search::SearchRequest>(input);
    searchRequestUnique->enableSearchInWriting().enableSearchInGlossary().enableSearchInReading();
    auto possibleIDs = search_->jmdict->search(std::move(searchRequestUnique));
    if (possibleIDs.empty())
    {
      bot_.getApi().sendMessage(userID, "No results found.");
      LOG_DEBUG("No results found for {}\n", input);
      return *this;
    }
    bot_.getApi().sendMessage(userID, std::format("Found {} results.", possibleIDs.size()));
    uint32_t counter = 0;
    for (const auto &id : possibleIDs)
    {
      if (++counter == 5)
      {
        userReplyStack_[userID] = UserReply::waiting;
        if (waitForUserReply(userID) == UserReply::stop)
        {
          return *this;
        }
        counter = 0;
      }

      try
      {
        auto glosses = search_->jmdict->gloss_by_id(id);
        if (!glosses.empty())
        {

          std::string combinedGloss = std::accumulate(
              glosses.begin() + 1,
              glosses.end(),
              glosses[0],
              [](const std::string &lhs, const std::string &rhs)
              {
                return lhs + ", " + rhs;
              });
          bot_.getApi().sendMessage(userID, combinedGloss);
        }

        auto reads = search_->jmdict->reading_by_id(id);
        if (!reads.empty())
        {
          std::string combinedReads = std::accumulate(
              reads.begin() + 1,
              reads.end(),
              reads[0],
              [](const std::string &lhs, const std::string &rhs)
              {
                return lhs + ", " + rhs;
              });
          bot_.getApi().sendMessage(userID, combinedReads);
        }

        auto writing = search_->jmdict->kanji_by_id(id);
        if (!writing.empty())
        {
          std::string combinedWriting = std::accumulate(
              writing.begin() + 1,
              writing.end(),
              writing[0],
              [](const std::string &lhs, const std::string &rhs)
              {
                return lhs + ", " + rhs;
              });
          bot_.getApi().sendMessage(userID, combinedWriting);
        }
      }
      catch (const std::exception &e)
      {
        LOG_EXCEPTION("Exception while searching", e);
      }
    }
    LOG_DEBUG("Search for {} finished\n", query->text);
    return *this;
  }
}