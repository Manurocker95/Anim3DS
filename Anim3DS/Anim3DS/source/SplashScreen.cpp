#include "SplashScreen.h"

SplashScreen::SplashScreen() : Scene()
{
	Start();
}

SplashScreen::~SplashScreen()
{
	m_SFX->stop();
	delete m_bgTop;
	delete m_bgBot;
}

void SplashScreen::Start()
{
	m_scTimer = 0;
	m_splashOpacity = 0;
	m_sfxSplash = false;
	m_goToGame = false;
	m_bgTop = sfil_load_PNG_file(IMG_SPLASHSCREEN, SF2D_PLACE_RAM);			
	m_bgBot = sfil_load_PNG_file(IMG_SPLASHSCREENBOTTOM, SF2D_PLACE_RAM);
	m_SFX = new sound(SND_SFX_SPLASH, 2, false);
}

void SplashScreen::Draw()
{

	switch (m_splashOpeningState)
	{
	case OPENING:
		// TopScreen
		sf2d_start_frame(GFX_TOP, GFX_LEFT);
		sf2d_draw_texture_blend(m_bgTop, 0, 0, RGBA8(255, 255, 255, m_splashOpacity));
		sf2d_end_frame();
		// BottomScreen
		sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
		sf2d_draw_texture_blend(m_bgBot, 0, 0, RGBA8(255, 255, 255, m_splashOpacity));
		sf2d_end_frame();
		break;
	case STAY:
		// TopScreen
		sf2d_start_frame(GFX_TOP, GFX_LEFT);
		sf2d_draw_texture_blend(m_bgTop, 0, 0, RGBA8(255, 255, 255, m_splashOpacity));
		sf2d_end_frame();
		// BottomScreen
		sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
		sf2d_draw_texture_blend(m_bgBot, 0, 0, RGBA8(255, 255, 255, m_splashOpacity));
		sf2d_end_frame();
		break;
	case ENDING:
		// TopScreen
		sf2d_start_frame(GFX_TOP, GFX_LEFT);
		sf2d_draw_texture_blend(m_bgTop, 0, 0, RGBA8(255, 255, 255, m_splashOpacity));
		sf2d_end_frame();
		// BottomScreen
		sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
		sf2d_draw_texture_blend(m_bgBot, 0, 0, RGBA8(255, 255, 255, m_splashOpacity));
		sf2d_end_frame();
		break;
	}
}

void SplashScreen::Update()
{
	switch (m_splashOpeningState)
	{
	case OPENING:

		m_splashOpacity += 3;

		if (m_splashOpacity >= 255)
		{
			m_splashOpacity = 255;
			m_splashOpeningState = STAY;

			if (!m_sfxSplash)
			{
				m_sfxSplash = true;
				m_SFX->play();
			}
		}

		break;
	case STAY:

		m_scTimer += 5;

		if (m_scTimer >= 300)
		{
			m_splashOpeningState = ENDING;
		}

		break;
	case ENDING:

		m_splashOpacity -= 3;

		if (m_splashOpacity <= 0)
		{
			m_goToGame = true;
		}
		break;
	}

	if (m_goToGame)
	{
		GoToGame();
	}
}

void SplashScreen::GoToGame()
{
	delete (m_SFX);
	sf2d_free_texture(m_bgTop);
	sf2d_free_texture(m_bgBot);
	SceneManager::instance()->setActualScene(SceneManager::GAME);
}

void SplashScreen::CheckInputs()
{
	hidScanInput();

	// Pressing select we exit
	if ((hidKeysDown() & KEY_SELECT))
	{
		SceneManager::instance()->SaveDataAndExit();
	}

	// We can skip Intro if we press A or touch the bottom screen
	if ((hidKeysDown() & KEY_A) || (hidKeysDown() & KEY_TOUCH))
	{
		m_goToGame = true;
	}
}