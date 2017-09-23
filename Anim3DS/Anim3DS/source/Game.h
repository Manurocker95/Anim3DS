/* This file is part of Anim3DS!

Copyright (C) 2017 Manuel Rodríguez Matesanz
>    This program is free software: you can redistribute it and/or modify
>    it under the terms of the GNU General Public License as published by
>    the Free Software Foundation, either version 3 of the License, or
>    (at your option) any later version.
>
>    This program is distributed in the hope that it will be useful,
>    but WITHOUT ANY WARRANTY; without even the implied warranty of
>    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
>    GNU General Public License for more details.
>
>    You should have received a copy of the GNU General Public License
>    along with this program.  If not, see <http://www.gnu.org/licenses/>.
>    See LICENSE for information.
*/

#pragma once
#ifndef _GAME_SCREEN_H_
#define _GAME_SCREEN_H_

#include "Settings.h"

#include "sound.h"
#include "scene.h"
#include "SceneManager.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <iostream>
#include <3ds.h>
#include <Vector>
#include <cstddef>  
#include "pp2d/pp2d.h"

class GameScreen : public Scene
{

public:

	enum MENU_TYPE { MAIN, LAST_ANIMES, ANIME_SELECTED, ANIME_READY_TO_WATCH, SEARCHING };

public:

	GameScreen();				// Constructor
	~GameScreen();				// Destructor
	void Start();				// initialize
	void Draw();				// Draw
	void CheckInputs();			// CheckInput
	void Update();				// Update

private:

	float m_offset;						// Offset for 3D
	float m_listOffset;
	Result m_internetServiceInitialized;
	sound * m_bgm, *m_sfx;			// Sounds
	std::string content = "";
	std::string chapterSelected = "";
	std::vector<std::string> arraychapter;
	int arrayselect;
	int arraycount;
	int off;
	MENU_TYPE menu_status;
	touchPosition touch;
	Result ret = 0;
	bool m_haveInternet;
	bool m_initializedList;
private:

	Result http_download(const char *url);
	void InitializeViewer();
	void ToggleWifi();
	void InitAnimeList();
};

#endif