#include <iostream>
#include <string>
#include "SDL2-2.0.9\include\SDL.h"
#include "SDL2-2.0.9\include\bass.h"

#pragma comment(lib, "ws2_32.lib")

#include "sync.h"

static const float bpm = 150.0f; /* beats per minute */
static const int rpb = 8; /* rows per beat */
static const double row_rate = (double(bpm) / 60) * rpb; /* rows per second */
float row = 0;

SDL_Window *win;
SDL_Renderer *ren;

SDL_Texture *scrtexture;

int effu_w = 320;
int effu_h = 400;

Uint32 *pixels = new Uint32[effu_w * effu_h];

SDL_Surface *hei;
SDL_Surface *map;

float plx;
float ply;
float prx;
float pry;
int ybuffer[1024] = { 0 };
Uint32 heibuffer[1024 * 1024] = { 0 };
Uint32 mapbuffer[1024 * 1024] = { 0 };

Uint32 time = 0;

int channu = -1;
#ifndef SYNC_PLAYER
bool should_save = false;	// whether to save tracks when done.
							// don't save unless we actually connect.
#endif

sync_device *rocket;
const struct sync_track *clear_r, *clear_g, *clear_b;
const struct sync_track *cam_rot, *cam_dist, *cam_x, *cam_y;
const struct sync_track *cam_h, *cam_hori, *cam_scaleh;

float sync_rot;
float sync_dist;
float sync_x;
float sync_y;
float sync_h;
float sync_hori;
float sync_scaleh;

float sync_c_r;
float sync_c_g;
float sync_c_b;

static double bass_get_row(HSTREAM h)
{
	QWORD pos = BASS_ChannelGetPosition(h, BASS_POS_BYTE);
	double time = BASS_ChannelBytes2Seconds(h, pos);
	return time * row_rate;
}

#ifndef SYNC_PLAYER

static void bass_pause(void *d, int flag)
{
	HSTREAM h = *((HSTREAM *)d);
	if (flag)
		BASS_ChannelPause(h);
	else
		BASS_ChannelPlay(h, false);
}

static void bass_set_row(void *d, int row)
{
	HSTREAM h = *((HSTREAM *)d);
	QWORD pos = BASS_ChannelSeconds2Bytes(h, row / row_rate);
	BASS_ChannelSetPosition(h, pos, BASS_POS_BYTE);
}

static int bass_is_playing(void *d)
{
	HSTREAM h = *((HSTREAM *)d);
	return BASS_ChannelIsActive(h) == BASS_ACTIVE_PLAYING;
}

static struct sync_cb bass_cb = {
	bass_pause,
	bass_set_row,
	bass_is_playing
};

#endif /* !defined(SYNC_PLAYER) */

void DoQuit() {
	delete[] pixels;
	SDL_DestroyTexture(scrtexture);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
}


SDL_Surface* LoadSurface(std::string file) {
	std::string imagePath = SDL_GetBasePath();
	imagePath += file;

	SDL_Surface *bmp = SDL_LoadBMP(imagePath.c_str());
	if (bmp == nullptr) {
		DoQuit();
		std::cout << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return NULL;
	}

	return bmp;
}

SDL_Texture* LoadTexture(std::string file) {
	SDL_Surface *bmp = LoadSurface(file);

	SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, bmp);
	SDL_FreeSurface(bmp);

	if (tex == nullptr) {
		DoQuit();
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(win);
		std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return NULL;
	}

	return tex;
}


void VertLine(int x, int y1, int y2, Uint32 color, float z) {
	if (x < 0 || x > effu_w) return;
	if (y1 < 0) y1 = 0;
	if (y1 > effu_h) y1 = effu_h;
	if (y2 > effu_h) y2 = effu_h;
	float zz = 1.0 - z * 0.001;
	if (zz < 0) zz = 0;

	Uint8 *c = (Uint8*)&color;

	Uint32 *cc = (Uint32*)c;

	int ys = y2;
	while (ys > y1) {
		ys-=2;
		pixels[ys * effu_w + x] = (*cc);
		pixels[(ys-1) * effu_w + x] = (*cc);
	}

}

Uint32 GetPixel(SDL_Surface *surface, int x, int y) {
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to retrieve */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch (bpp) {
	case 1:
		return *p;
		break;

	case 2:
		return *(Uint16 *)p;
		break;

	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			return p[0] << 16 | p[1] << 8 | p[2];
		else
			return p[0] | p[1] << 8 | p[2] << 16;
		break;

	case 4:
		return *(Uint32 *)p;
		break;

	default:
		return 0;       /* shouldn't happen, but avoids warnings */
	}
}

Uint8 luma(Uint8 r, Uint8 g, Uint8 b) {
	return (Uint8)(0.2126*r + 0.7152*g + 0.0722*b);
}


void DoHeightmap(float px, float py, float angle, float h, float horizon, float scale_h, float distance, int iter) {
	int sw = effu_w;
	int sh = effu_h;

	float sin_a = sin(angle);
	float cos_a = cos(angle);

	for (int i = 0; i < effu_w; i++) {
		ybuffer[i] = effu_h;
	}

	float dz = 0.001;
	float z = 1;
	plx = 0;
	ply = 0;
	int o = cos(time*0.0001)*150;
	while (z < distance) {
		plx = (-cos_a * z - sin_a * z) + px;
		ply = (sin_a*z - cos_a * z) + py;

		prx = (cos_a*z - sin_a * z) + px;
		pry = (-sin_a * z - cos_a * z) + py;

		float dx = (prx - plx) / sw;
		float dy = (pry - ply) / sw;

		for (int i = 0; i < sw; i+=1) {
			int c_plx = (int)plx & 1023;
			int c_ply = (int)ply & 1023;
			int offset = (c_ply << 10) + c_plx;


			int height_on_screen = (int)(((h  * cos(sin(z*0.01))) - heibuffer[offset]) / z * scale_h + horizon);
			height_on_screen *= 2;
			VertLine(i, (int)height_on_screen+effu_h, ybuffer[i], mapbuffer[offset], z);


			plx += dx;
			ply += dy;
		}

		z += dz;
		dz += 0.01;
	}


}

void RenderHeightmap() {
	SDL_SetRenderDrawColor(ren, sync_c_r, sync_c_g, sync_c_b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ren);

	DoHeightmap(sync_x, sync_y, sync_rot, sync_h, sync_hori, sync_scaleh, sync_dist, 0);
}

int main(int argc, char * argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	win = SDL_CreateWindow("instanssi 2019 demo", 100, 100, 640, 480, SDL_WINDOW_SHOWN);

	if (win == nullptr) {
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ren == nullptr) {
		SDL_DestroyWindow(win);
		std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	scrtexture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, effu_w, effu_h);

	hei = LoadSurface("d1.bmp");
	map = LoadSurface("c1.bmp");

	if (hei == NULL || map == NULL) return 1;

	for (int y = 0; y < 1024; y++) {
		for (int x = 0; x < 1024; x++) {
			Uint32 heightpixel = GetPixel(hei, x, y);
			Uint8 *colors = (Uint8*)&heightpixel;

			Uint8 heightvalue = luma(colors[2], colors[1], colors[0]);
			heibuffer[y * 1024 + x] = heightvalue;

			mapbuffer[y * 1024 + x] = GetPixel(map, x, y);
		}
	}

	SDL_Event e;
	bool quit = false;

	rocket = sync_create_device("sync");

#ifndef SYNC_PLAYER
	sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
#endif

	clear_r = sync_get_track(rocket, "clear.r");
	clear_g = sync_get_track(rocket, "clear.g");
	clear_b = sync_get_track(rocket, "clear.b");
	cam_rot = sync_get_track(rocket, "cam.rot"),
	cam_dist = sync_get_track(rocket, "cam.dist");
	cam_x = sync_get_track(rocket, "cam.x");
	cam_y = sync_get_track(rocket, "cam.y");
	cam_h = sync_get_track(rocket, "cam.h");
	cam_hori = sync_get_track(rocket, "cam.hori");
	cam_scaleh = sync_get_track(rocket, "cam.scaleh");

	HSTREAM stream;

	/* init BASS */
	BASS_Init(-1, 44100, 0, 0, 0);
	stream = BASS_StreamCreateFile(false, "music.ogg", 0, 0, BASS_STREAM_PRESCAN);

	/* let's roll! */
	BASS_Start();
	BASS_ChannelPlay(stream, false);

	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
			if (e.type == SDL_KEYDOWN) {
				switch (e.key.keysym.sym) {
				case SDLK_ESCAPE:
					quit = true;
					break;
				default:
					break;
				}
			}
		}

		row = bass_get_row(stream);
#ifndef SYNC_PLAYER
		if (sync_update(rocket, (int)floor(row), &bass_cb, (void *)&stream)) {
			if (sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT) == 0)
				should_save = true;
		}
#endif

		sync_c_r = float(sync_get_val(clear_r, row));
		sync_c_g = float(sync_get_val(clear_g, row));
		sync_c_b = float(sync_get_val(clear_b, row));

		sync_x = float(sync_get_val(cam_x, row));
		sync_y = float(sync_get_val(cam_y, row));
		sync_h = float(sync_get_val(cam_h, row));
		sync_hori = float(sync_get_val(cam_hori, row));
		sync_scaleh = float(sync_get_val(cam_scaleh, row));
		sync_rot = float(sync_get_val(cam_rot, row));
		sync_dist = float(sync_get_val(cam_dist, row));

		time = row*row_rate*4;

		memset(pixels, 0, effu_w * effu_h * sizeof(Uint32));

		// draw
		RenderHeightmap();

		SDL_UpdateTexture(scrtexture, NULL, pixels, effu_w * sizeof(Uint32));

		SDL_RenderCopy(ren, scrtexture, NULL, NULL);
		SDL_RenderPresent(ren);

		BASS_Update(0); /* decrease the chance of missing vsync */
	}

#ifndef SYNC_PLAYER
	if (should_save)		//don't clobber if user just ran it then hit Esc
		sync_save_tracks(rocket);
#endif
	sync_destroy_device(rocket);

	BASS_StreamFree(stream);
	BASS_Free();

	DoQuit();
	SDL_Quit();

	return 0;
}

