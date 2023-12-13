#include <csignal>

#include <tgbot/tgbot.h>
#include "log.hpp"
#include "botcommander.hpp"
#include "metrics/profiler.hpp"

std::atomic<bool> sigintReceived(false);

void handleSignal(int s)
{
  LOG_INFO("Got SIGINT, exiting\n");
  sigintReceived = true;
  Profiler::getInstance().dumpTextReport("metrics.txt");
}

int main()
{
#if defined(WAKABOT_TOKEN)
  LOG_DEBUG("WAKABOT_TOKEN is set: {}\n", WAKABOT_TOKEN);
  TgBot::Bot bot(WAKABOT_TOKEN);
#else
  LOG_DEBUG("WAKABOT_TOKEN is not set, trying to get it from environment\n");
  const char *token = getenv("WAKABOT_TOKEN");
  if (!token)
  {
    LOG_DEBUG("WAKABOT_TOKEN is not set, exiting\n");
    return 1;
  }
  TgBot::Bot bot(token);
#endif

  Log::get().configure(TraceType::file).set_level(TraceSeverity::debug);
  LOG_INFO("Bot {} started\n", bot.getApi().getMe()->username);

  Bot::BotCommander::Ptr commander = std::make_unique<Bot::BotCommander>(bot);

  // Set commands
  std::vector<TgBot::BotCommand::Ptr> commands;
  TgBot::BotCommand::Ptr cmdArray = std::make_shared<TgBot::BotCommand>();
  cmdArray->command = "search";
  cmdArray->description = "Search for a word in the dictionary.";
  commands.push_back(cmdArray);
  cmdArray = std::make_shared<TgBot::BotCommand>();
  cmdArray->command = "example";
  cmdArray->description = "Search for an usage example in the dictionary.";
  commands.push_back(cmdArray);
  cmdArray = std::make_shared<TgBot::BotCommand>();
  cmdArray->command = "quiz";
  cmdArray->description = "Start a quiz. You can choose between diffeeent types of questions.";
  commands.push_back(cmdArray);
  cmdArray = std::make_shared<TgBot::BotCommand>();
  cmdArray->command = "info_word";
  cmdArray->description = "Explains a word, kanji or example in the dictionary.";
  commands.push_back(cmdArray);
  bot.getApi().setMyCommands(commands);

  // Set event handlers
  bot.getEvents().onUnknownCommand([&commander](TgBot::Message::Ptr message)
                                   { commander->parseCommand(message); });

  bot.getEvents().onNonCommandMessage([&commander](TgBot::Message::Ptr message)
                                      { commander->parseUserInputAsync(message); });

  bot.getEvents().onCallbackQuery([&commander](TgBot::CallbackQuery::Ptr query)
                                  { commander->parseCallback(query); });

  bot.getEvents().onInlineQuery([&commander](TgBot::InlineQuery::Ptr query)
                                { commander->parseInlineQuery(query); });

  bot.getEvents().onChosenInlineResult([&commander](TgBot::ChosenInlineResult::Ptr result)
                                       { commander->parseChosenInlineResult(result); });

  bot.getEvents().onEditedMessage([&commander](TgBot::Message::Ptr message)
                                  { commander->parseEditedMessage(message); });

  signal(SIGINT, handleSignal);

  bot.getApi().deleteWebhook();
  TgBot::TgLongPoll longPoll(bot);
  int lastUpdateId = 0;
  try
  {
    while (!sigintReceived)
    {
      longPoll.start();
    }
  }
  catch (TgBot::TgException &e)
  {
    LOG_EXCEPTION("Runtime error", e);
  }
}