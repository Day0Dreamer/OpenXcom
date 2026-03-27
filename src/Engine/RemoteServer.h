#pragma once
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
#include <string>
#include <map>
#include <functional>
#include <queue>
#include <mutex>
#include <future>
#include <memory>
#include <atomic>
#include <SDL.h>

namespace OpenXcom
{

class Game;

/// SDL event type used to wake the game loop when a remote command is queued.
static constexpr Uint8 REMOTE_CMD_EVENT = SDL_USEREVENT;

/// Maximum number of commands allowed in the queue before rejecting new ones.
static constexpr int MAX_QUEUED_COMMANDS = 16;

/// A command queued from the HTTP thread for execution on the game thread.
struct RemoteCommand
{
	std::string endpoint;
	std::map<std::string, std::string> params;
	std::promise<std::string> result;
};

/**
 * Lightweight HTTP server that accepts GET requests on localhost
 * and dispatches them as commands to the game thread.
 */
class RemoteServer
{
public:
	using HandlerFunc = std::function<std::string(Game*, const std::map<std::string, std::string>&)>;

	RemoteServer(Game* game, int port);
	~RemoteServer();

	/// Starts the HTTP listener thread.
	void start();
	/// Stops the HTTP listener and drains the command queue.
	void stop();
	/// Registers a handler for the given path (e.g. "/heal").
	void registerHandler(const std::string& path, HandlerFunc handler);
	/// Processes all queued commands on the game thread.
	void processCommands();

private:
	Game* _game;
	int _port;
	std::atomic<bool> _shuttingDown{false};
	bool _started = false;

	std::map<std::string, HandlerFunc> _handlers;
	std::queue<std::shared_ptr<RemoteCommand>> _commandQueue;
	std::mutex _queueMutex;

	SDL_Thread* _thread = nullptr;
	void* _httpServer = nullptr; // httplib::Server*, opaque to avoid header leak

	static int serverThreadFunc(void* data);
	void registerDefaultHandlers();
};

}
