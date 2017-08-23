#pragma once
#ifndef _SPLASH_SCREEN_H_
#define _SPLASH_SCREEN_H_

#include <sftd.h>
#include <sf2d.h>
#include <sfil.h>

#include "Settings.h"

#include "sound.h"
#include "scene.h"
#include "SceneManager.h"


class SplashScreen: public Scene
{

public:

	enum SPLASH_STATE { OPENING, STAY, ENDING };

	SplashScreen();						// Constructor
	~SplashScreen();
	
	void Start() override;				 
	void Draw() override;				
	void CheckInputs() override;		
	void Update() override;				
	void GoToGame();
private:

	int m_splashOpacity, m_scTimer;
	bool m_sfxSplash, m_goToGame;
	sound * m_SFX;
	sf2d_texture * m_bgTop, *m_bgBot;
	SPLASH_STATE m_splashOpeningState;
};

#endif