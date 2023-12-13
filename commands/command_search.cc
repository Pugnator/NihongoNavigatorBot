#include "../botcommander.hpp"
#include "log.hpp"

namespace Bot
{
  const BotCommander &BotCommander::commandSearchWord(const TgBot::Message::Ptr &query)
  {
    LOG_DEBUG("User {} wants to search for a word\n", query->chat->id);
    int64_t userID = query->chat->id;
    commandStack_[userID] = BotCommand::searchSingleWord;
    const std::string searchActionStr = "typing";
    bot_.getApi().sendChatAction(userID, searchActionStr);
    std::string input = "";
    if (!query->text.empty() && query->text[0] == '/')
      input = getStringToken(query->text, 2);
    else
      input = getStringToken(query->text, 1);

    if (input.empty())
    {
      bot_.getApi().sendMessage(userID, "Enter a word to search for.");
      LOG_DEBUG("No input provided for search_word\n");
      return *this;
    }

    Search::SearchRequest::Ptr searchRequestUnique = std::make_unique<Search::SearchRequest>(input);
    searchRequestUnique->enableSearchInGlossary();
    std::vector<uint32_t> possibleIDs;
    // First try to search in glossary
    auto ids = search_->jmdict->search(std::move(searchRequestUnique));
    possibleIDs.insert(possibleIDs.end(), ids.begin(), ids.end());

    // May be it's in Japanese?
    searchRequestUnique = std::make_unique<Search::SearchRequest>(input);
    searchRequestUnique->enableSearchInWriting().enableSearchInReading();
    ids = search_->jmdict->search(std::move(searchRequestUnique));
    possibleIDs.insert(possibleIDs.end(), ids.begin(), ids.end());

    // Maybe it's a romaji?
    std::string romaji = KanaProc::toRomaji(input);
    if (romaji != input)
    {
      searchRequestUnique = std::make_unique<Search::SearchRequest>(romaji);
      searchRequestUnique->enableSearchInWriting().enableSearchInReading();
      ids = search_->jmdict->search(std::move(searchRequestUnique));

      possibleIDs.insert(possibleIDs.end(), ids.begin(), ids.end());
    }
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
        auto writing = search_->jmdict->kanji_by_id(id);
        auto reads = search_->jmdict->reading_by_id(id);
        auto glosses = search_->jmdict->gloss_by_id(id);

        std::string result = std::format("*{}* _{}_ {} `{}` ...",
                                         !writing.empty() ? writing.front() : reads.front(),
                                         !writing.empty() ? reads.front() : "",
                                         KanaProc::toRomaji(reads.front()),
                                         glosses.front());

        bot_.getApi().sendMessage(query->chat->id, result, false, 0, std::make_shared<TgBot::GenericReply>(), "Markdown");
      }
      catch (const std::exception &e)
      {
        LOG_EXCEPTION("Exception while searching", e);
      }
    }
    LOG_DEBUG("Search for {} finished\n", input);
    return *this;
  }

  const BotCommander &BotCommander::commandSearchExample(const TgBot::Message::Ptr &query)
  {
    int64_t userID = query->chat->id;
    commandStack_[query->chat->id] = BotCommand::searchExample;
    std::string input = "";
    if (!query->text.empty() && query->text[0] == '/')
      input = getStringToken(query->text, 2);
    else
      input = getStringToken(query->text, 1);
    LOG_DEBUG("User {} wants to search for usage examples\n", userID);
    const std::string searchActionStr = "typing";
    bot_.getApi().sendChatAction(userID, searchActionStr);
    Search::SearchRequest::Ptr searchRequestUnique = std::make_unique<Search::SearchRequest>();
    searchRequestUnique->enableSearchInExamples();
    searchRequestUnique->setSearchQuery(input);
    auto possibleIDs = search_->example->search(std::move(searchRequestUnique));
    if (possibleIDs.empty())
    {
      bot_.getApi().sendMessage(userID, "Enter a word to search for in usage examples.");
      LOG_DEBUG("No input provided for search_example\n");
      return *this;
    }

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

      std::string result;
      auto example = search_->example->tatoeba_example(id);
      auto translation = search_->example->tatoeba_translation_eng(id);
      result += example;
      result += "\r\n";
      if (!translation.empty())
        result += translation.front();

      bot_.getApi().sendMessage(userID, result);
    }
    LOG_DEBUG("Search for {} finished\n", input);
    return *this;
  }
}