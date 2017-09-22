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

	//programacion es mode 0
	//buscar es mode 1

	gfxFlushBuffers();

	do {
		ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);

		// This disables SSL cert verification, so https:// will be usable
		ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);


		// Set a User-Agent header so websites can identify your application
		ret = httpcAddRequestHeaderField(&context, "User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.81 Safari/537.36");

		gfxFlushBuffers();

		// Tell the server we can support Keep-Alive connections.
		// This will delay connection teardown momentarily (typically 5s)
		// in case there is another request made to the same server.
		ret = httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");

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
						  //printf("redirecting to url: %s\n",url);
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
	content = reinterpret_cast<char*>(buf);

	//gspWaitForVBlank();

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
	delete m_bgm;
	delete m_sfx;
	pp2d_free_texture(TEXTURE_SPRITESHEET2);
}

void GameScreen::Start()
{

	// We initialize our game variables
	m_offset = 0;
	m_listOffset = 0;

	menu_status = GameScreen::MENU_TYPE::MAIN;

	// We load our images and place them into RAM so they can be painted
	pp2d_load_texture_png(TEXTURE_SPRITESHEET2, IMG_SPRITES);

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

	while (val0 != -1) {
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

	// Top Screen
	pp2d_begin_draw(GFX_TOP);
	pp2d_draw_texture_part(TEXTURE_SPRITESHEET2, 0, 0, TOP_WIDTH, 0, TOP_WIDTH, HEIGHT);


	switch (menu_status)
	{
	case MENU_TYPE::MAIN:

		// BANNER - 
		pp2d_draw_texture_part(TEXTURE_SPRITESHEET2, 75, 60, 0, 536, 256, 128);
		pp2d_draw_text(140, 220, 0.4f, 0.4f, C_SECOND_BLUE, "Manurocker95 (C) 2017");

		break;
	case MENU_TYPE::LAST_ANIMES:
		
		//TODO//sftd_draw_text(font, 20, 20, C_SECOND_BLUE, 20, arraychapter[arrayselect].substr(arraychapter[arrayselect].rfind("/") + 1).c_str());
		
		for (int i = 0; i < arraychapter.size(); i++)
		{
			if (i == arrayselect)
			{
				pp2d_draw_text(50, i * 14 + off + m_listOffset, 0.4f, 0.4f, C_SECOND_BLUE, "->");
			}
			pp2d_draw_text(65, i * 14 + off + m_listOffset, 0.4f, 0.4f, C_SECOND_BLUE, std::to_string(i).c_str());
			pp2d_draw_text(85, i * 14 + off + m_listOffset, 0.4f, 0.4f, C_SECOND_BLUE, arraychapter[i].substr(arraychapter[i].rfind("/") + 1).c_str());
		}	
		
		break;
	case MENU_TYPE::ANIME_SELECTED:

		pp2d_draw_text(20, 35, 0.4f, 0.4f, C_SECOND_BLUE, arraychapter[arrayselect].substr(arraychapter[arrayselect].rfind("/") + 1).c_str());

		break;
	case MENU_TYPE::ANIME_READY_TO_WATCH:
		pp2d_draw_text(20, 35, 0.4f, 0.4f, C_SECOND_BLUE, "Pulsa A para ver el episodio");
		break;

	case MENU_TYPE::SEARCHING:
		pp2d_draw_text(20, 35, 0.4f, 0.4f, C_SECOND_BLUE, "Buscar anime");
		break;
	}

	// Bottom screen
	pp2d_draw_on(GFX_BOTTOM);
	pp2d_draw_texture_part(TEXTURE_SPRITESHEET2, 0, 0, TOP_WIDTH, HEIGHT, BOTTOM_WIDTH, HEIGHT);
	pp2d_draw_text(80, 10, 0.5f, 0.5f, C_SECOND_BLUE, "Selecciona una opcion:");


	// VER ULTIMOS ANIMES
	pp2d_draw_texture_part(TEXTURE_SPRITESHEET2, 18, 36, 0, 480, 283, 56);
	pp2d_draw_text(60, 55, 0.7f, 0.7f, C_SECOND_BLUE, "VER ULTIMOS ANIMES");

	// BUSCAR ANIME
	pp2d_draw_texture_part(TEXTURE_SPRITESHEET2, 18, 102, 0, 480, 283, 56);
	pp2d_draw_text(95, 121, 0.7f, 0.7f, C_SECOND_BLUE, "BUSCAR ANIME");

	// SALIR
	pp2d_draw_texture_part(TEXTURE_SPRITESHEET2, 18, 168, 0, 480, 283, 56);
	pp2d_draw_text(135, 187, 0.7f, 0.7f, C_SECOND_BLUE, "SALIR");
	pp2d_end_draw();

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
			int val1 = content.find("ok.ru/videoembed/");
			int val2 = content.find('"', val1);
			string gdrive = content.substr(val1, val2 - val1);
			gdrive = "https://" + gdrive;
			content = "";
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

