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
#include "../Engine/State.h"

namespace OpenXcom
{

class TextButton;
class Frame;
class Text;

/**
 * Displays a help popup listing all custom keyboard shortcuts.
 */
class HotkeyHelpState : public State
{
private:
	TextButton *_btnOk;
	Frame *_frame;
	Text *_txtLeft;
	Text *_txtRight;
public:
	/// Creates the HotkeyHelpState.
	HotkeyHelpState();
	/// Cleans up the HotkeyHelpState.
	~HotkeyHelpState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
};

}
