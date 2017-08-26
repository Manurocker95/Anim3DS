/* This file is part of What's in the Box 3DS!

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

#include "sfil.h"
#include "sftd.h"
#include "sf2d.h"

#include "font_ttf.h"			//Don't worry if it seems to have error here
#include "Settings.h"

#include "sound.h"
#include "scene.h"
#include "SceneManager.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <iostream>
#include <3ds.h>
#include <Vector>
#include <cstddef>  

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
	sf2d_texture * m_selector;
	sf2d_texture * m_banner;
	sf2d_texture * m_button;	    // Button image
	sf2d_texture *m_bgTop,*m_bgBot;	// Background images
	
	sound * m_bgm, *m_sfx;			// Sounds
	
	sftd_font* font;				// Main Font
	
	std::string content = "";
	std::vector<std::string> arraychapter;
	int arrayselect;
	int arraycount;
	//int m_episodeSelected;
	//int m_maxEpisodeSelected;
	MENU_TYPE menu_status;
	touchPosition touch;
	Result ret = 0;

private:

	Result http_download(const char *url);
	void InitializeViewer();
};

#endif