#include <iostream>
#include <string>
#include "SDL2-2.0.9\include\SDL.h"

SDL_Window *win;
SDL_Renderer *ren;

SDL_Texture *scrtexture;
Uint32 *pixels = new Uint32[640 * 400];

SDL_Surface *hei;
SDL_Surface *map;

int effu_w = 640;
int effu_h = 400;

float plx;
float ply;
float prx;
float pry;
int ybuffer[640] = { 0 };
Uint32 heibuffer[1024 * 1024] = { 0 };
Uint32 mapbuffer[1024 * 1024] = { 0 };

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
	if (y1 > effu_h || y2 > effu_h) return;
	float zz = 1.0 - z * 0.001;
	if (zz < 0) zz = 0;

	Uint8 *c = (Uint8*)&color;

	c[0] *= zz;
	c[1] *= zz;
	c[2] *= zz;

	Uint32 *cc = (Uint32*)c;

	int ys = y2;
	while (ys > y1) {
		ys--;
		pixels[ys * 640 + x] = *cc;
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

void DoHeightmap(float px, float py, float angle, float h, float horizon, float scale_h, float distance) {
	int sw = effu_w;
	int sh = effu_h;

	float sin_a = sin(angle);
	float cos_a = cos(angle);

	for (int i = 0; i < effu_w; i++) {
		ybuffer[i] = effu_h;
	}

	float dz = 0.001;
	float z = 1;

	while (z < distance) {
		plx = (-cos_a * z - sin_a * z) + px;
		ply = (sin_a*z - cos_a * z) + py;

		prx = (cos_a*z - sin_a * z) + px;
		pry = (-sin_a * z - cos_a * z) + py;

		float dx = (prx - plx) / sw;
		float dy = (pry - ply) / sw;

		for (int i = 0; i < sw; i++) {
			int c_plx = (int)plx & 1023;
			int c_ply = (int)ply & 1023;
			int offset = (c_ply << 10) + c_plx;

			int height_on_screen = (int)((h - heibuffer[offset]) / z * scale_h + horizon);
			VertLine(i, (int)height_on_screen, ybuffer[i], mapbuffer[offset], z);

			if (height_on_screen < ybuffer[i]) ybuffer[i] = height_on_screen;

			plx += dx;
			ply += dy;
		}

		z += dz;
		dz += 0.01;
	}
}

void RenderHeightmap() {
	SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ren);

	float angle = SDL_GetTicks()*0.0002;
	DoHeightmap(SDL_GetTicks()*0.08, SDL_GetTicks()*0.06, angle,180, 50+cos(SDL_GetTicks()*0.001)*25, 250, 2000);
}

int main(int argc, char * argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	win = SDL_CreateWindow("instanssi 2019 demo", 100, 100, 640, 400, SDL_WINDOW_SHOWN);

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

	scrtexture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 640, 400);

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

	memset(pixels, 0, 640 * 400 * sizeof(Uint32));

	SDL_Event e;
	bool quit = false;

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

		SDL_UpdateTexture(scrtexture, NULL, pixels, 640 * sizeof(Uint32));

		// draw
		RenderHeightmap();

		SDL_UpdateTexture(scrtexture, NULL, pixels, 640 * sizeof(Uint32));
		SDL_RenderCopy(ren, scrtexture, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	DoQuit();
	SDL_Quit();

	return 0;
}

