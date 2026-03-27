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
#include "InfoboxOKState.h"
#include "../Engine/Game.h"
#include "../Interface/TextButton.h"
#include "../Interface/Frame.h"
#include "../Interface/Text.h"
#include "../Interface/Cursor.h"
#include "../Engine/Options.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"

namespace OpenXcom
{

/**
 * Initializes all the elements.
 * @param game Pointer to the core game.
 * @param msg Message string.
 */
InfoboxOKState::InfoboxOKState(const std::string &msg, bool largeReport, const std::string &msgRight)
{
	_screen = false;
	_txtRight = nullptr;

	// Create objects
	if (largeReport)
	{
		_frame = new Frame(310, 170, 5, 10);
		_btnOk = new TextButton(120, 18, 100, 158);
		// Left column: stats
		_txtTitle = new Text(148, 145, 10, 13);
		// Right column: traces
		if (!msgRight.empty())
		{
			_txtRight = new Text(148, 145, 162, 13);
		}
	}
	else
	{
		_frame = new Frame(261, 89, 30, 48);
		_btnOk = new TextButton(120, 18, 100, 112);
		_txtTitle = new Text(255, 61, 33, 51);
	}

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	add(_frame, "infoBoxOK", "battlescape");
	add(_btnOk, "infoBoxOKButton", "battlescape");
	add(_txtTitle, "infoBoxOK", "battlescape");
	if (_txtRight)
	{
		add(_txtRight, "infoBoxOK", "battlescape");
	}

	centerAllSurfaces();

	// Set up objects
	_frame->setThickness(3);
	_frame->setHighContrast(true);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&InfoboxOKState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&InfoboxOKState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&InfoboxOKState::btnOkClick, Options::keyCancel);
	_btnOk->setHighContrast(true);

	if (largeReport)
	{
		_txtTitle->setSmall();
		_txtTitle->setAlign(ALIGN_LEFT);
		_txtTitle->setVerticalAlign(ALIGN_TOP);
		if (_txtRight)
		{
			_txtRight->setSmall();
			_txtRight->setAlign(ALIGN_LEFT);
			_txtRight->setVerticalAlign(ALIGN_TOP);
			_txtRight->setHighContrast(true);
			_txtRight->setWordWrap(true);
			_txtRight->setText(msgRight);
		}
	}
	else
	{
		_txtTitle->setBig();
		_txtTitle->setAlign(ALIGN_CENTER);
		_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	}
	_txtTitle->setHighContrast(true);
	_txtTitle->setWordWrap(true);
	_txtTitle->setText(msg);

	_game->getCursor()->setVisible(true);
}

/**
 *
 */
InfoboxOKState::~InfoboxOKState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void InfoboxOKState::btnOkClick(Action *)
{
	_game->popState();
}

}
