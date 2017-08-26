#include "Game.h"

static SwkbdCallbackResult MyCallback(void* user, const char** ppMessage, const char* text, size_t textlen)
{
	if (strstr(text, "lenny"))
	{
		*ppMessage = "Nice try but I'm not letting you use that meme right now";
		return SWKBD_CALLBACK_CONTINUE;
	}

	if (strstr(text, "brick"))
	{
		*ppMessage = "~Time to visit Brick City~";
		return SWKBD_CALLBACK_CLOSE;
	}

	return SWKBD_CALLBACK_OK;
}

Result GameScreen::http_download(const char *url)
{
	Result ret = 0;
	httpcContext context;
	char *newurl = NULL;
	u8* framebuf_top;
	u32 statuscode = 0;
	u32 contentsize = 0, readsize = 0, size = 0;
	u8 *buf, *lastbuf;


	gfxFlushBuffers();

	do {
		ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);

		gfxFlushBuffers();

		// This disables SSL cert verification, so https:// will be usable
		ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);

		gfxFlushBuffers();

		// Enable Keep-Alive connections (on by default, pending ctrulib merge)
		// ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
		// printf("return from httpcSetKeepAlive: %"PRId32"\n",ret);
		// gfxFlushBuffers();

		// Set a User-Agent header so websites can identify your application
		ret = httpcAddRequestHeaderField(&context, "User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.81 Safari/537.36");

		gfxFlushBuffers();

		// Tell the server we can support Keep-Alive connections.
		// This will delay connection teardown momentarily (typically 5s)
		// in case there is another request made to the same server.
		ret = httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");

		gfxFlushBuffers();

		ret = httpcBeginRequest(&context);
		if (ret != 0) {
			httpcCloseContext(&context);
			if (newurl != NULL) free(newurl);
			return ret;
		}

		ret = httpcGetResponseStatusCode(&context, &statuscode);
		if (ret != 0) {
			httpcCloseContext(&context);
			if (newurl != NULL) free(newurl);
			return ret;
		}

		if ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308)) {
			if (newurl == NULL) newurl = (char*)malloc(0x1000); // One 4K page for new URL
			if (newurl == NULL) {
				httpcCloseContext(&context);
				return -1;
			}
			ret = httpcGetResponseHeader(&context, "Location", newurl, 0x1000);
			url = newurl; // Change pointer to the url that we just learned
			printf("redirecting to url: %s\n", url);
			httpcCloseContext(&context); // Close this context before we try the next
		}
	} while ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308));

	if (statuscode != 200) {

		httpcCloseContext(&context);
		if (newurl != NULL) free(newurl);
		return -2;
	}

	// This relies on an optional Content-Length header and may be 0
	ret = httpcGetDownloadSizeState(&context, NULL, &contentsize);
	if (ret != 0) {
		httpcCloseContext(&context);
		if (newurl != NULL) free(newurl);
		return ret;
	}


	gfxFlushBuffers();

	// Start with a single page buffer
	buf = (u8*)malloc(0x1000);
	if (buf == NULL) {
		httpcCloseContext(&context);
		if (newurl != NULL) free(newurl);
		return -1;
	}

	do {
		// This download loop resizes the buffer as data is read.
		ret = httpcDownloadData(&context, buf + size, 0x1000, &readsize);
		size += readsize;
		if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING) {
			lastbuf = buf; // Save the old pointer, in case realloc() fails.
			buf = (u8*)realloc(buf, size + 0x1000);
			if (buf == NULL) {
				httpcCloseContext(&context);
				free(lastbuf);
				if (newurl != NULL) free(newurl);
				return -1;
			}
		}
	} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

	if (ret != 0) {
		httpcCloseContext(&context);
		if (newurl != NULL) free(newurl);
		free(buf);
		return -1;
	}

	// Resize the buffer back down to our actual final size
	lastbuf = buf;
	buf = (u8*)realloc(buf, size);
	if (buf == NULL) { // realloc() failed.
		httpcCloseContext(&context);
		free(lastbuf);
		if (newurl != NULL) free(newurl);
		return -1;
	}

	//printf("%s", buf);
	content = reinterpret_cast<char*>(buf);


	gfxFlushBuffers();

	//if(size>(240*400*3*2))size = 240*400*3*2;

	//framebuf_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	//memcpy(framebuf_top, buf, size);

	//gfxFlushBuffers();
	//gfxSwapBuffers();

	//framebuf_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	//memcpy(framebuf_top, buf, size);

	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForVBlank();

	httpcCloseContext(&context);
	free(buf);
	if (newurl != NULL) free(newurl);

	return 0;
}

GameScreen::GameScreen() : Scene ()
{
	// we initialize data
	Start();
}

GameScreen::~GameScreen()
{
	delete m_bgTop;
	delete m_bgBot;
	delete m_button;
	delete m_bgm;
	delete m_sfx;
	delete font;
}

void GameScreen::Start()
{
	// We will use 2 channels for sounds: 1 = BGM, 2= Sound effects so they can be played at same time. You can set as channels as you want.
	// We clear the channels
	ndspChnWaveBufClear(1);
	ndspChnWaveBufClear(2);

	// We load our font called font.ttf in data folder. Set the data folder in MakeFile
	font = sftd_load_font_mem(font_ttf, font_ttf_size);

	// We initialize our game variables
	m_offset = 0;
	m_listOffset = 0;
	int m_episodeSelected = 0;
	int m_maxEpisodeSelected = 0;
	menu_status = GameScreen::MENU_TYPE::MAIN;

	// We load our images and place them into RAM so they can be painted
	m_bgTop = sfil_load_PNG_file(IMG_BG_GAME_TOP, SF2D_PLACE_RAM);
	m_bgBot = sfil_load_PNG_file(IMG_BG_GAME_BOT, SF2D_PLACE_RAM);
	m_button = sfil_load_PNG_file(IMG_BUTTON_SPRITE, SF2D_PLACE_RAM);
	m_banner = sfil_load_PNG_file(IMG_BANNER_SPRITE, SF2D_PLACE_RAM);

	// We load our sounds // PATH, CHANNEL, LOOP? -> // BGM plays with loop, but sfx just one time
	m_bgm = new sound(SND_BGM_GAME, 1, true);
//	m_sfx = new sound(SND_SFX_TAP, 2, false);

	// We play our bgm
	m_bgm->play();

	InitializeViewer();
}

void GameScreen::InitializeViewer()
{

	httpcInit(0); // Buffer size when POST/PUT.
	ret = http_download("http://animeflv.net/");
	int vval1 = content.find("ltimos episodios");
	int vval2 = content.find("ltimos animes", vval1);

	content = content.substr(vval1, vval2 - vval1);

	int val1 = 1;
	int val2;
	int val0 = 0;
	arrayselect = 0;
	arraycount = 0;

	while (val0 != -1)
	{
		val0 = content.find("/ver/", val1);
		if (val0 == -1) { break; }

		val1 = content.find("/ver/", val1);
		val2 = (content.find('"', val1));
		string gdrive = "http://animeflv.net" + content.substr(val1, val2 - val1);


		arraychapter.push_back(gdrive);
		cout << arraycount << ". " << gdrive.substr(gdrive.rfind("/") + 1) << endl;
		arraycount++;
		val1++;
	}

	// arraychapter[arrayselect].substr(arraychapter[arrayselect].rfind("/") + 1)
}

void GameScreen::Draw()
{

	int off = 0;

	sf2d_start_frame(GFX_TOP, GFX_LEFT);
	sf2d_draw_texture(m_bgTop, 0, 0);

	switch (menu_status)
	{
	case MENU_TYPE::MAIN:

		sf2d_draw_texture(m_banner, 75, 60);
		sftd_draw_text(font, 160, 220, C_SECOND_BLUE, 15, "Manurocker95 (C) 2017");

		break;
	case MENU_TYPE::LAST_ANIMES:
		
		//sftd_draw_text(font, 20, 20, C_SECOND_BLUE, 20, arraychapter[arrayselect].substr(arraychapter[arrayselect].rfind("/") + 1).c_str());
		
		for (int i = 0; i < arraychapter.size(); i++)
		{
			if (i == arrayselect)
			{
				sftd_draw_text(font, 50, i * 14 + off+ m_listOffset, C_SECOND_BLUE, 18, "->");
			}
			sftd_draw_text(font, 65, i * 14 + off + m_listOffset, C_SECOND_BLUE, 18, std::to_string(i).c_str());
			sftd_draw_text(font, 80, i * 14 + off+ m_listOffset, C_SECOND_BLUE, 18, arraychapter[i].substr(arraychapter[i].rfind("/") + 1).c_str());
		}	
		
		break;
	case MENU_TYPE::ANIME_SELECTED:

		sftd_draw_text(font, 20, 20, C_SECOND_BLUE, 20, arraychapter[arrayselect].substr(arraychapter[arrayselect].rfind("/") + 1).c_str());

		break;
	case MENU_TYPE::ANIME_READY_TO_WATCH:
		sftd_draw_text(font, 20, 20, C_SECOND_BLUE, 20, "Pulsa A para ver el episodio");
		break;

	case MENU_TYPE::SEARCHING:
		//sftd_draw_text(font, 20, 20, C_SECOND_BLUE, 20, "Buscar anime:");
		break;
	}

	sf2d_end_frame();

	// If we have activated 3D in Settings
	if (STEREOSCOPIC_3D_ACTIVATED)
	{
		// We check the offset by the slider
		m_offset = CONFIG_3D_SLIDERSTATE * MULTIPLIER_3D;
		sf2d_start_frame(GFX_TOP, GFX_RIGHT);
		sf2d_draw_texture(m_bgTop, 0, 0);
		//sftd_draw_text(font, 15 - m_offset, 5, C_BLUE, TAPTEXTSIZE, "HELLO WORLD!!");
		sf2d_end_frame();
	}

	// Bottom screen (We just show an image)
	sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
	sf2d_draw_texture(m_bgBot, 0, 0);
	sftd_draw_text(font, 80, 5, C_SECOND_BLUE, 20, "Selecciona una opción:");
	
	// VER ULTIMOS ANIMES
	sf2d_draw_texture(m_button, 18, 36);
	sftd_draw_text(font, 60, 40, C_SECOND_BLUE, 30, "VER ÚLTIMOS ANIMES");

	// BUSCAR ANIME
	sf2d_draw_texture(m_button, 18, 102);
	sftd_draw_text(font, 95, 106, C_SECOND_BLUE, 30, "BUSCAR ANIME");

	// SALIR
	sf2d_draw_texture(m_button, 18, 168);
	sftd_draw_text(font, 135, 172, C_SECOND_BLUE, 30, "SALIR");
	sf2d_end_frame();
}

void GameScreen::Update()
{

}

void GameScreen::CheckInputs()
{
	static SwkbdState swkbd;
	static char mybuf[60];
	static SwkbdStatusData swkbdStatus;
	static SwkbdLearningData swkbdLearning;
	SwkbdButton button = SWKBD_BUTTON_NONE;
	bool didit = false;

	hidScanInput();

	u32 pressed = hidKeysDown();

	if ((pressed & KEY_RIGHT) || (pressed & KEY_DOWN))
	{
		if (menu_status == MENU_TYPE::LAST_ANIMES)
		{
			if (arrayselect < arraychapter.size() - 1)
			{
				arrayselect++;
			}
			else
			{
				arrayselect = 0;
			}
		}
	}

	if ((pressed & KEY_LEFT) || (pressed & KEY_UP))
	{
		if (menu_status == MENU_TYPE::LAST_ANIMES)
		{
			if (arrayselect > 0)
			{
				arrayselect--;
			}
			else
			{
				arrayselect = arraychapter.size() - 1;
			}
		}

	}

	if ((pressed & KEY_B))
	{
		if (menu_status == MENU_TYPE::MAIN)
		{
			SceneManager::instance()->SaveDataAndExit();
		}
		else if (menu_status == MENU_TYPE::LAST_ANIMES)
		{
			menu_status = MENU_TYPE::MAIN;
		}
		else if (menu_status == MENU_TYPE::ANIME_SELECTED)
		{
			menu_status = MENU_TYPE::LAST_ANIMES;
		}
		else if (menu_status == MENU_TYPE::ANIME_READY_TO_WATCH)
		{
			menu_status = MENU_TYPE::ANIME_SELECTED;
		}
	}

	// We Exit pressing Select
	if ((pressed & KEY_SELECT))
	{
		SceneManager::instance()->SaveDataAndExit();
	}

	if ((pressed & KEY_A))
	{
		if (menu_status == MENU_TYPE::MAIN)
		{
			menu_status = MENU_TYPE::LAST_ANIMES;
		}
		else if (menu_status == MENU_TYPE::LAST_ANIMES)
		{
			menu_status = MENU_TYPE::ANIME_SELECTED;
		}
		else if (menu_status == MENU_TYPE::ANIME_SELECTED)
		{
			menu_status = MENU_TYPE::ANIME_READY_TO_WATCH;
			ret = http_download(arraychapter[arrayselect].c_str());
			int val1 = content.find("server=hyperion");
			int val2 = content.find('"', val1);

			string gdrive = content.substr(val1, val2 - val1);
			gdrive = "http://s3.animeflv.com/check.php?" + gdrive;
			content = "";
			ret = http_download(gdrive.c_str());
			val1 = content.find(":360");
			val1 = content.find("http", val1);
			val2 = content.find('"', val1);

			gdrive = content.substr(val1, val2 - val1);
			cout << gdrive << endl;
			cout << "VIDEO EXTRAIDO: PRESIONA START PARA CONTINUAR." << endl;
			APT_PrepareToStartSystemApplet(APPID_WEB);
			APT_StartSystemApplet(APPID_WEB, gdrive.c_str(), 1024, 0);
		}
		else if (menu_status == MENU_TYPE::ANIME_READY_TO_WATCH)
		{
			SceneManager::instance()->SaveDataAndExit();
		}
	}

	if (pressed & KEY_TOUCH)
	{
		hidTouchRead(&touch);
		if ((touch.px > 18 && touch.px < 300) && (touch.py > 36 && touch.py < 90))
		{
			if (menu_status == MENU_TYPE::MAIN)
			{
				menu_status = MENU_TYPE::LAST_ANIMES;
			}
			
		}

		if ((touch.px > 18 && touch.px < 300) && (touch.py > 102 && touch.py < 156))
		{
			didit = true;
			swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, -1);
			swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
			swkbdSetFilterCallback(&swkbd, MyCallback, NULL);
			button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
			if (mybuf == "")
				didit = false;

			if (didit)
			{
				ret = http_download(mybuf);
				int val1 = content.find("server=hyperion");
				int val2 = content.find('"', val1);

				string gdrive = content.substr(val1, val2 - val1);
				gdrive = "http://s3.animeflv.com/check.php?" + gdrive;

				content = "";
				ret = http_download(gdrive.c_str());
				val1 = content.find(":360");
				val1 = content.find("http", val1);
				val2 = content.find('"', val1);
				gdrive = content.substr(val1, val2 - val1);
				cout << gdrive << endl;
				APT_PrepareToStartSystemApplet(APPID_WEB);
				APT_StartSystemApplet(APPID_WEB, gdrive.c_str(), 1024, 0);
				didit = false;

				menu_status = MENU_TYPE::ANIME_READY_TO_WATCH;
			}
		}

		if ((touch.px > 18 && touch.px < 300) && (touch.py > 172 && touch.py < 226)) //+54
		{
			SceneManager::instance()->SaveDataAndExit();
		}
	}
}

