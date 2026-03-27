/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "RemoteServer.h"
#include "Game.h"
#include "Language.h"
#include "Logger.h"
#include "Options.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/BattleUnit.h"
#include "../Battlescape/BattlescapeState.h"
#include "httplib.h"
#include <sstream>
#include <chrono>
#include <algorithm>

namespace OpenXcom
{

RemoteServer::RemoteServer(Game* game, int port)
	: _game(game), _port(port)
{
	registerDefaultHandlers();
}

RemoteServer::~RemoteServer()
{
	if (_started)
		stop();
}

void RemoteServer::start()
{
	if (_started) return;

	auto* server = new httplib::Server();
	_httpServer = server;

	// Catch-all GET handler: queue commands for the game thread.
	server->Get(".*", [this](const httplib::Request& req, httplib::Response& res) {
		// Reject if shutting down
		if (_shuttingDown.load())
		{
			res.status = 503;
			res.set_content(R"({"error":"Server shutting down"})", "application/json");
			return;
		}

		// Rate limit: check queue size
		{
			std::lock_guard<std::mutex> lock(_queueMutex);
			if (static_cast<int>(_commandQueue.size()) >= MAX_QUEUED_COMMANDS)
			{
				res.status = 429;
				res.set_content(R"({"error":"Too many requests"})", "application/json");
				return;
			}
		}

		// Build command
		auto cmd = std::make_shared<RemoteCommand>();
		cmd->endpoint = req.path;
		for (const auto& p : req.params)
		{
			cmd->params[p.first] = p.second;
		}

		// Get future before pushing (promise is inside the shared object)
		auto future = cmd->result.get_future();

		// Enqueue
		{
			std::lock_guard<std::mutex> lock(_queueMutex);
			_commandQueue.push(cmd);
		}

		// Wake the game loop
		SDL_Event event;
		event.type = REMOTE_CMD_EVENT;
		SDL_PushEvent(&event);

		// Wait for game thread to process (5 second timeout)
		auto status = future.wait_for(std::chrono::seconds(5));
		if (status == std::future_status::timeout)
		{
			res.status = 503;
			res.set_content(R"({"error":"Game busy, try again"})", "application/json");
			return;
		}

		res.set_content(future.get(), "application/json");
	});

	_started = true;
	_thread = SDL_CreateThread(serverThreadFunc, this);
	Log(LOG_INFO) << "Remote server starting on 127.0.0.1:" << _port;
}

int RemoteServer::serverThreadFunc(void* data)
{
	auto* self = static_cast<RemoteServer*>(data);
	auto* server = static_cast<httplib::Server*>(self->_httpServer);
	server->listen("127.0.0.1", self->_port);
	return 0;
}

void RemoteServer::stop()
{
	if (!_started) return;

	Log(LOG_INFO) << "Remote server stopping...";

	// 1. Reject new requests
	_shuttingDown.store(true);

	// 2. Stop the HTTP server (unblocks listen(), waits for in-flight requests)
	auto* server = static_cast<httplib::Server*>(_httpServer);
	server->stop();

	// 3. Wait for the listener thread to exit
	if (_thread)
	{
		SDL_WaitThread(_thread, nullptr);
		_thread = nullptr;
	}

	// 4. Drain any remaining queued commands
	{
		std::lock_guard<std::mutex> lock(_queueMutex);
		while (!_commandQueue.empty())
		{
			auto cmd = _commandQueue.front();
			_commandQueue.pop();
			try
			{
				cmd->result.set_value(R"({"error":"Server shutting down"})");
			}
			catch (const std::future_error&)
			{
				// Promise already satisfied (e.g. HTTP side timed out)
			}
		}
	}

	// 5. Clean up
	delete server;
	_httpServer = nullptr;
	_started = false;

	Log(LOG_INFO) << "Remote server stopped.";
}

void RemoteServer::registerHandler(const std::string& path, HandlerFunc handler)
{
	_handlers[path] = std::move(handler);
}

void RemoteServer::processCommands()
{
	// Swap queue under lock to minimize lock hold time
	std::queue<std::shared_ptr<RemoteCommand>> local;
	{
		std::lock_guard<std::mutex> lock(_queueMutex);
		std::swap(_commandQueue, local);
	}

	while (!local.empty())
	{
		auto cmd = local.front();
		local.pop();

		std::string result;
		auto it = _handlers.find(cmd->endpoint);
		if (it != _handlers.end())
		{
			try
			{
				result = it->second(_game, cmd->params);
			}
			catch (const std::exception& e)
			{
				result = std::string(R"({"error":"Handler exception: )") + e.what() + R"("})";
			}
			catch (...)
			{
				result = R"({"error":"Unknown handler exception"})";
			}
		}
		else
		{
			result = R"({"error":"Unknown endpoint","path":")" + cmd->endpoint + R"("})";
		}

		try
		{
			cmd->result.set_value(result);
		}
		catch (const std::future_error&)
		{
			// Promise already satisfied (HTTP side timed out)
		}
	}
}

void RemoteServer::registerDefaultHandlers()
{
	// GET /heal?name=SOLDIER_NAME&amount=30
	registerHandler("/heal", [](Game* game, const std::map<std::string, std::string>& params) -> std::string {
		// Validate params
		auto nameIt = params.find("name");
		auto amountIt = params.find("amount");
		if (nameIt == params.end() || amountIt == params.end())
			return R"({"error":"Missing required parameter(s): 'name' and 'amount'"})";

		int amount;
		try
		{
			amount = std::stoi(amountIt->second);
		}
		catch (...)
		{
			return R"({"error":"'amount' must be an integer"})";
		}

		// Check battle exists
		SavedGame* save = game->getSavedGame();
		if (!save || !save->getSavedBattle())
			return R"({"error":"No battle in progress"})";

		// Find unit by name (player faction, alive)
		const std::string& targetName = nameIt->second;
		BattleUnit* found = nullptr;
		int matchCount = 0;
		for (auto* unit : *save->getSavedBattle()->getUnits())
		{
			if (unit->getOriginalFaction() != FACTION_PLAYER) continue;
			if (unit->isOut()) continue;
			if (unit->getName(game->getLanguage()) == targetName)
			{
				if (!found) found = unit;
				matchCount++;
			}
		}

		if (!found)
			return R"({"error":"Unit not found","name":")" + targetName + R"("})";

		// Apply heal
		int oldHp = found->getHealth();
		int maxHp = found->getBaseStats()->health;
		int newHp = std::min(oldHp + amount, maxHp);
		found->setHealth(newHp);

		// Refresh battlescape UI
		BattlescapeState* bs = save->getSavedBattle()->getBattleState();
		if (bs)
			bs->updateSoldierInfo(false);

		// Build response
		std::ostringstream json;
		json << R"({"status":"ok")";
		json << R"(,"unit":")" << targetName << "\"";
		json << R"(,"health":)" << found->getHealth();
		json << R"(,"maxHealth":)" << maxHp;
		json << R"(,"healed":)" << (found->getHealth() - oldHp);
		if (matchCount > 1)
		{
			json << R"(,"warning":"Multiple units with this name, healed first match")";
			json << R"(,"matches":)" << matchCount;
		}
		json << "}";
		return json.str();
	});

	// GET /status — basic game state info
	registerHandler("/status", [](Game* game, const std::map<std::string, std::string>&) -> std::string {
		std::ostringstream json;
		json << R"({"status":"ok")";

		SavedGame* save = game->getSavedGame();
		if (save)
		{
			json << R"(,"hasSave":true)";
			if (save->getSavedBattle())
			{
				json << R"(,"inBattle":true)";
				int playerUnits = 0;
				int aliveUnits = 0;
				for (auto* unit : *save->getSavedBattle()->getUnits())
				{
					if (unit->getOriginalFaction() != FACTION_PLAYER) continue;
					playerUnits++;
					if (!unit->isOut()) aliveUnits++;
				}
				json << R"(,"playerUnits":)" << playerUnits;
				json << R"(,"aliveUnits":)" << aliveUnits;
			}
			else
			{
				json << R"(,"inBattle":false)";
			}
		}
		else
		{
			json << R"(,"hasSave":false,"inBattle":false)";
		}

		json << "}";
		return json.str();
	});
}

}
