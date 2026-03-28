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
#include "HotkeyHelpState.h"
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
 * Initializes the hotkey help popup with two columns of shortcuts.
 */
HotkeyHelpState::HotkeyHelpState()
{
	_screen = false;

	// Create objects - near full screen
	_frame = new Frame(310, 185, 5, 3);
	_btnOk = new TextButton(80, 16, 120, 170);
	_txtLeft = new Text(148, 160, 10, 6);
	_txtRight = new Text(148, 160, 162, 6);

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	add(_frame, "infoBoxOK", "battlescape");
	add(_btnOk, "infoBoxOKButton", "battlescape");
	add(_txtLeft, "infoBoxOK", "battlescape");
	add(_txtRight, "infoBoxOK", "battlescape");

	centerAllSurfaces();

	// Set up objects
	_frame->setThickness(3);
	_frame->setHighContrast(true);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&HotkeyHelpState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&HotkeyHelpState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&HotkeyHelpState::btnOkClick, Options::keyCancel);
	_btnOk->setHighContrast(true);

	// Left column - Tactical Overlays
	std::ostringstream left;
	left << "=== TACTICAL OVERLAYS ===" << "\n";
	left << "\n";
	left << "Shift+1  Colored LOS" << "\n";
	left << "Ctrl+1    LOS to all enemies" << "\n";
	left << "C+S+1    Blocked LOS" << "\n";
	left << "Alt+1     Shot simulation" << "\n";
	left << "\n";
	left << "Shift+2  Corridor of fire" << "\n";
	left << "Shift+3  Exposure display" << "\n";
	left << "Shift+4  Crossfire (hover)" << "\n";
	left << "\n";
	left << "O         Overwatch lanes" << "\n";
	left << "Shift+O  Enemy overwatch" << "\n";
	left << "C+S+O    All enemy overwatch" << "\n";
	left << "\n";
	left << "Pause    Unit FOV cone" << "\n";
	left << "Ctrl+End Debug vision";

	_txtLeft->setSmall();
	_txtLeft->setAlign(ALIGN_LEFT);
	_txtLeft->setVerticalAlign(ALIGN_TOP);
	_txtLeft->setHighContrast(true);
	_txtLeft->setWordWrap(true);
	_txtLeft->setText(left.str());

	// Right column - Info & Controls
	std::ostringstream right;
	right << "=== INFO & STATUS ===" << "\n";
	right << "\n";
	right << "Ctrl+F    Fatal wounds" << "\n";
	right << "Ctrl+H    Hit log (this turn)" << "\n";
	right << "C+A+H    Turn diary (all)" << "\n";
	right << "Ctrl+E    No experience list" << "\n";
	right << "C+S+E     Experience overview" << "\n";
	right << "Ctrl+M    Melee damage preview" << "\n";
	right << "\n";
	right << "=== VIEW & CONTROLS ===" << "\n";
	right << "\n";
	right << "Ctrl+C   Single map layer" << "\n";
	right << "Ctrl+B   Reopen briefing" << "\n";
	right << "Ctrl+S   Quick mode (speed)" << "\n";
	right << "Ctrl+X   Mute unit sounds" << "\n";
	right << "Alt+C     Custom marker" << "\n";
	right << "\n";
	right << "Shift+?  This help";

	_txtRight->setSmall();
	_txtRight->setAlign(ALIGN_LEFT);
	_txtRight->setVerticalAlign(ALIGN_TOP);
	_txtRight->setHighContrast(true);
	_txtRight->setWordWrap(true);
	_txtRight->setText(right.str());

	_game->getCursor()->setVisible(true);
}

/**
 *
 */
HotkeyHelpState::~HotkeyHelpState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void HotkeyHelpState::btnOkClick(Action *)
{
	_game->popState();
}

}
