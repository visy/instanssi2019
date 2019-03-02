﻿
#define _CRT_SECURE_NO_WARNINGS 1

#include <iostream>
#include <string>
#include "SDL2-2.0.9\include\SDL.h"
#include "SDL2-2.0.9\include\bass.h"

#pragma comment(lib, "ws2_32.lib")

#include "sync.h"

struct Vector4 {
	float x, y, z, w;
};
struct Vector3 {
	float x, y, z;
};
struct Vector2 {
	float x, y;
};

static const float bpm = 150.0f; /* beats per minute */
static const int rpb = 8; /* rows per beat */
static const double row_rate = (double(bpm) / 60) * rpb; /* rows per second */
float row = 0;

SDL_Window *win;
SDL_Renderer *ren;

SDL_Texture *scrtexture;
SDL_Texture *kuviotexture;

SDL_Texture *reversecross_texture;
SDL_Texture *house_texture;
SDL_Texture *font_texture;
SDL_Texture *paita_texture;

int effu_w = 320;
int effu_h = 400;

Uint32 *pixels = new Uint32[effu_w * effu_h];
Uint32 *pixelskuvio = new Uint32[160 * 100];

SDL_Surface *colormap_image;
SDL_Surface *heightmap_image;
SDL_Surface *reversecross_image;
SDL_Surface *font_image;
SDL_Surface *house_image;
SDL_Surface *paita_image;

SDL_Rect font_bb[15 * 16] = {};

unsigned char house_mask[320 * 100] = { 255 };

float plx;
float ply;
float prx;
float pry;
int ybuffer[1024] = { 0 };
Uint32 colormap_buffer[1024 * 1024] = { 0 };
Uint32 heightmap_buffer[1024 * 1024] = { 0 };

Uint32 time = 0;

int channu = -1;
#ifndef SYNC_PLAYER
bool should_save = false;	// whether to save tracks when done.
							// don't save unless we actually connect.
#endif

sync_device *rocket;
const struct sync_track *scene;
const struct sync_track *clear_r, *clear_g, *clear_b, *clear_a;
const struct sync_track *cam_rot, *cam_dist, *cam_x, *cam_y;
const struct sync_track *cam_h, *cam_hori, *cam_scaleh;

int sync_scene;

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
float sync_c_a;

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
	delete[] pixelskuvio;
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
		ys -= 2;
		pixels[ys * effu_w + x] = (*cc);
		pixels[(ys - 1) * effu_w + x] = (*cc);
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

Uint32 get_pixel(SDL_Surface *surface, int x, int y)
{
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
void set_pixel(SDL_Renderer *rend, int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	SDL_SetRenderDrawColor(rend, r, g, b, a);
	SDL_RenderDrawPoint(rend, x, y);
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
	int o = cos(time*0.0001) * 150;
	while (z < distance) {
		plx = (-cos_a * z - sin_a * z) + px;
		ply = (sin_a*z - cos_a * z) + py;

		prx = (cos_a*z - sin_a * z) + px;
		pry = (-sin_a * z - cos_a * z) + py;

		float dx = (prx - plx) / sw;
		float dy = (pry - ply) / sw;

		for (int i = 0; i < sw; i += 1) {
			int c_plx = (int)plx & 1023;
			int c_ply = (int)ply & 1023;
			int offset = (c_ply << 10) + c_plx;


			int height_on_screen = (int)(((h  * cos(sin(z*0.01))) - colormap_buffer[offset]) / z * scale_h + horizon);
			height_on_screen *= 2;
			VertLine(i, (int)height_on_screen + effu_h, ybuffer[i], heightmap_buffer[offset], z);


			plx += dx;
			ply += dy;
		}

		z += dz;
		dz += 0.01;
	}
}

int cursor_x = 0;
int cursor_y = 0;

void DoText(const char* text, int x, int y) {
	cursor_x = x;
	cursor_y = y;
	int off = -4;
	SDL_SetTextureColorMod(font_texture, 0, 0, 0);

	for (int i = 0; i < strlen(text); i++) {
		char letter = text[i];
		int font_y = ((int)letter - 33) / 16;
		int font_x = ((int)letter - 33) % 16;

		SDL_Rect srcrect = font_bb[(font_y * 16) + font_x];

		SDL_Rect dstrect;
		dstrect.x = cursor_x;
		dstrect.y = cursor_y;
		dstrect.w = srcrect.w * 2;
		dstrect.h = srcrect.h * 2;
		SDL_RenderCopy(ren, font_texture, &srcrect, &dstrect);

		cursor_x += srcrect.w * 2;
	}

	cursor_x = x;
	SDL_SetTextureColorMod(font_texture, 255, 255, 255);

	for (int i = 0; i < strlen(text); i++) {
		char letter = text[i];
		int font_y = ((int)letter - 33) / 16;
		int font_x = ((int)letter - 33) % 16;

		SDL_Rect srcrect = font_bb[(font_y * 16) + font_x];

		SDL_Rect dstrect;
		dstrect.x = cursor_x + off;
		dstrect.y = cursor_y + off;
		dstrect.w = srcrect.w * 2;
		dstrect.h = srcrect.h * 2;
		SDL_RenderCopy(ren, font_texture, &srcrect, &dstrect);

		cursor_x += srcrect.w * 2;
	}

}

void DoFont() {
	SDL_SetTextureBlendMode(scrtexture, SDL_BLENDMODE_BLEND);

	int ti = (int)sync_c_a;

	if (ti == 1) DoText("______Quadtrip+Ivory+Jumalauta______", 100, 1080 / 2 - 32);
	if (ti == 2) DoText("PURGATORIUm____________", 100, 1080 / 2 - 32);


	//	if (ti == 1) DoText("performing_forbidden_rituals_outside", 100, 100);
}

void RenderHeightmap() {
	SDL_SetTextureBlendMode(scrtexture, SDL_BLENDMODE_NONE);

	SDL_SetRenderDrawColor(ren, sync_c_r, sync_c_g, sync_c_b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ren);

	DoHeightmap(sync_x, sync_y, sync_rot, sync_h, sync_hori, sync_scaleh, sync_dist, 0);
}

SDL_Surface *sshot;
void RenderKefrensCross() {

	SDL_Rect dstrect;

	SDL_SetRenderTarget(ren, scrtexture);
	SDL_SetTextureBlendMode(scrtexture, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(ren, sync_c_r, sync_c_g, sync_c_b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ren);
	dstrect.w = 128;
	dstrect.h = 128;
	for (int i = 0; i < 100; i += 2) {
		dstrect.x = 320 / 2 - 64 + 100 * sin(sync_dist + 2.0f *  3.141592f * i / 100) * cos(sync_rot + 2.0f *  3.141592f * i / 128);
		dstrect.y = 200 / 2 - 64 + i - 80 + sin(sync_dist*0.321f + i * 0.05f) * 30;
		SDL_RenderCopy(ren, reversecross_texture, NULL, &dstrect);
	}
	int w = 1920;
	int h = 1080;
	if (sshot == NULL)
		sshot = SDL_CreateRGBSurface(0, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

	SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);

	for (int x = 16; x < 320 - 16; x++) {
		auto selected = GetPixel(sshot, x, 0);
		for (int y = 0; y < 200; y++) {
			auto pixel = GetPixel(sshot, x, y);
			Uint8 r, g, b, a;
			SDL_GetRGBA(pixel, sshot->format, &r, &g, &b, &a);
			if (pixel & 0x00ffffff > 0) {
				selected = pixel;
			}
			else {
				Uint8 r, g, b;
				SDL_GetRGB(selected, sshot->format, &r, &g, &b);
				set_pixel(ren, x, y, selected >> 16 & 255, selected >> 8 & 255, selected & 255, 255);
			}
			if (selected & 0x00ffffff == 0) { y += 10;  continue; }
			Uint32 rr = selected >> 16 & 255;
			Uint32 gg = selected >> 8 & 255;
			Uint32 bb = selected & 255;
			Uint32 aa = selected >> 24 & 255;
			if (rr > 0) rr--;
			if (gg > 0) gg--;
			if (bb > 0) bb--;
			selected = rr << 16;
			selected |= gg << 8;
			selected |= bb;
			selected |= aa << 24;
		}

	}
	set_pixel(ren, 100, 100, 32, 123, 200, 32);
	SDL_SetRenderTarget(ren, NULL);
	SDL_RenderPresent(ren);
	//SDL_FreeSurface(sshot);
}

float dot(Vector3 A, Vector3 B)
{
	float product = 0;
	product = product + A.x * B.x;
	product = product + A.y * B.y;
	product = product + A.z * B.z;
	return product;
}
float step(float a, float b) {
	if (b > a) return 1.0f;
	return 0.0f;
}
Vector3 normalize(Vector3 v) {
	float length = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
	v.x /= length;
	v.y /= length;
	v.z /= length;
	return v;
}
float sphere(Vector3 ray, Vector3 dir, Vector3 center, float radius)
{
	Vector3 rc; // = ray - center;
	rc.x = ray.x - center.x;
	rc.y = ray.y - center.y;
	rc.z = ray.z - center.z;
	float c = dot(rc, rc) - (radius*radius);
	float b = dot(dir, rc);
	float d = b * b - c;
	float t = -b - sqrt(abs(d));
	float st = 1.0f - step(0.0, min(t, d));
	return  t + st;
}
Vector4 mainImage(Vector2 fragCoord, Vector2 iResolution)
{
	Vector4 C; // = vec4(1.0);
	C = { 0.0f,0.0f,0.0f,0.0f };
	Vector2 uv; // = fragCoord.xy / iResolution.xy - vec2(0.5);
	uv.x = fragCoord.x / iResolution.x - 0.5f;
	uv.y = fragCoord.y / iResolution.y - 0.5f;
	float aspect = iResolution.x / iResolution.y;
	Vector3 direction; // = normalize(vec3(.5 * uv * vec2(aspect, 1.0), 0.7));
	direction.x = 0.5 * uv.x * aspect;
	direction.y = 0.5 * uv.y;
	direction.z = 0.7;
	direction = normalize(direction);
	float dist = sphere(Vector3{ 0.0f, 0.0f, -3.0f }, direction, Vector3{ 0.0f,0.0f,0.0f }, sync_h);
	//C.rgb *= direction * dist*dist / 4.;
	float d = dist * dist / 4.0f;
	C.x = direction.x * d;
	C.y = direction.y * d;
	C.z = direction.z * d;
	//C = texture(iChannel0, C.rg);
	return C;
}
int flick = 0;
void SphereEffect() {

	SDL_Rect dstrect;

	SDL_SetRenderTarget(ren, scrtexture);
	SDL_SetTextureBlendMode(scrtexture, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(ren, sync_c_r, sync_c_g, sync_c_b, SDL_ALPHA_OPAQUE);
	float ax = sin(sync_x) * 256.0f;
	float ay = cos(sync_y) * 256.0f;
	flick++;
	for (int y = 0; y < 200; y++) {
		for (int x = 0; x < 320; x ++) {
			Vector4 uv = mainImage(Vector2{ (float)x + sin(sync_rot)*100.0f,(float)y + cos(sync_rot*0.8f)*100.0f }, Vector2{ 320.0f,200.0f });
			/*
			auto pixel = GetPixel(sshot, x, y);
			Uint8 r, g, b, a;
			SDL_GetRGBA(pixel, sshot->format, &r, &g, &b, &a);
			*/
			Uint8 r, g, b, a;
			r = (Uint8)(uv.x * 256.0f + ax) & 127;
			g = (Uint8)(uv.y * 256.0f + ay) & 127;
			b = (Uint8)(uv.z * 256.0f) & 255;
			int d = uv.z * 128.0f;
			int d2 = uv.z * 133.0f;
			int d3 = uv.z * 144.0f;
			auto pixel = GetPixel(reversecross_image, r, g);

			r = pixel >> 16 & 255;
			g = pixel >> 8 & 255;
			b = pixel & 255;

			r = (Uint8)max((int)r - d3, 0);
			g = (Uint8)max((int)g - d2, 0);
			b = (Uint8)max((int)b - d, 0);
			a = pixel >> 24 & 255;

			set_pixel(ren, x, y, r, g, b, 255);
		}
	}
	SDL_SetRenderTarget(ren, NULL);
	SDL_RenderPresent(ren);
	//SDL_FreeSurface(sshot);
}


void FireEffect() {

	SDL_Rect dstrect;

	SDL_SetRenderTarget(ren, scrtexture);
	SDL_SetTextureBlendMode(scrtexture, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(ren, sync_c_r, sync_c_g, sync_c_b, SDL_ALPHA_OPAQUE);
	int w = 1920;
	int h = 1080;
	if (sshot == NULL)
		sshot = SDL_CreateRGBSurface(0, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

		for (int x = 0; x < 320; x++) {

			Uint8 r, g, b, a;
			r = (Uint8)rand();
			g = (Uint8)rand() >> 2;
			b = (Uint8)rand() >> 3;
			if (rand() & 3 == 0) {
				r = 255;
				g = 255;
				b = 255;
			}
			set_pixel(ren, x, 199, r, g, b, 255);
		}
	
	SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);

	flick++;
	for (int y = 0; y < 199; y++) {
		for (int x = flick & 1; x < 319; x+=2) {
			/*
			auto pixel = GetPixel(sshot, x, y);
			Uint8 r, g, b, a;
			SDL_GetRGBA(pixel, sshot->format, &r, &g, &b, &a);
			*/
			Uint8 r, g, b, a;
			auto pixel = GetPixel(sshot, x + 1,min(y+ (rand() & 1 + 1),199) );

			r = pixel >> 16 & 255;
			g = pixel >> 8 & 255;
			b = pixel & 255;

			r = (Uint8)max((int)r - 2, 0);
			g = (Uint8)max((int)g - 4, 0);
			b = (Uint8)max((int)b - 8, 0);
			a = pixel >> 24 & 255;

			set_pixel(ren, x, y, r, g, b, 255);
		}
	}
	SDL_SetRenderTarget(ren, NULL);
	SDL_RenderPresent(ren);
	//SDL_FreeSurface(sshot);
}

void RenderHouse() {
	SDL_SetTextureBlendMode(scrtexture, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(ren, sync_c_r, sync_c_g, sync_c_b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ren);

	for (int y = 0; y < 100; y++) {
		for (int x = 1; x < 320; x++) {
			int xs = x * 1 + ((effu_w - 2) / 32);
			int ys = y * 3;
			pixels[ys * effu_w + xs] = house_mask[y * 320 + x] * 0x00111111;
			pixels[(ys + 1) * effu_w + xs] = house_mask[y * 320 + x] * 0x00111111;
		}
	}

	SDL_UpdateTexture(scrtexture, NULL, pixels, effu_w * sizeof(Uint32));
	SDL_RenderCopy(ren, scrtexture, NULL, NULL);

	SDL_SetTextureBlendMode(scrtexture, SDL_BLENDMODE_BLEND);

	SDL_SetTextureAlphaMod(house_texture, 200);
	SDL_Rect dstrect;

	dstrect.x = effu_w / 8;
	dstrect.y = 1;
	dstrect.w = effu_w * 6;
	dstrect.h = effu_h * 2;
	SDL_RenderCopy(ren, house_texture, NULL, &dstrect);
}

int gw = 160;
int gh = 100;

int g2w = 320;
int g2h = 400;

int nSeed = 5323;

int prng() {
	nSeed = (8253729 * nSeed + 2396403);
	return nSeed % 32767;
}

int paitacount = 0;
int r_paita = 1337;

void RenderShirt() {
	SDL_SetRenderTarget(ren, scrtexture);
	SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ren);

	r_paita = (int)sync_h;

	for (int y = 0; y < gh; y++) {
		for (int x = 0; x < gw; x += 1) {
			float pe = 0.05;
			int col = (int)((0xFF * cos(r_paita + y * pe + tan(x*100.01))*sin((x*abs(cos(r_paita + 300)) - 40)*pe)*tan(x*0.01 + cos(x*r_paita*0.01 + y * 0.1)*r_paita*0.00009*cos(tan(x*0.2 + r_paita)))));
			int dd = abs(gh / 2 - y);
			if (dd > 20) col -= (dd - 20) * 0x50;
			if (col < 0xFF) pixelskuvio[y*gw + x] = 0x00000000;
			else pixelskuvio[y*gw + x] = 0xFFFFFFFF;

			//if (col > 0xFF) g.pixels[(y*gw)+(x+1)] = 0xAAAAAAAA;

		}
	}
	for (int y = 0; y < gh; y++) {
		for (int x = 0; x < gw / 2; x++) {
			pixelskuvio[y*gw + x] = pixelskuvio[(y*gw) + (gw - 1 - x)];
		}
	}

	SDL_UpdateTexture(kuviotexture, NULL, pixelskuvio, 160 * sizeof(Uint32));
	//SDL_SetTextureAlphaMod(paita_texture, 255);
	SDL_Rect dstrect;

	dstrect.x = effu_w * 1.62;
	dstrect.y = effu_h * 0.9;
	dstrect.w = 160 * 4;
	dstrect.h = 100 * 2.7;
	SDL_RenderCopy(ren, kuviotexture, NULL, &dstrect);
	SDL_SetTextureBlendMode(paita_texture, SDL_BLENDMODE_BLEND);

	SDL_SetTextureAlphaMod(paita_texture, 255);
	SDL_RenderCopy(ren, paita_texture, NULL, NULL);

	dstrect.x = 0;
	dstrect.y = 0;
	dstrect.w = 1920;
	dstrect.h = 1080;
	SDL_SetRenderTarget(ren, scrtexture);

	SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_MOD);
	SDL_SetRenderDrawColor(ren, sync_c_r, sync_c_g, sync_c_b, 0);

	SDL_RenderFillRect(ren, &dstrect);

	SDL_SetRenderTarget(ren, NULL);
	SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
}


int main(int argc, char * argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	win = SDL_CreateWindow("instanssi 2019 demo", 100, 100, 1920, 1080, SDL_WINDOW_SHOWN);

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
	kuviotexture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 160, 100);

	colormap_image = LoadSurface("d1.bmp");
	heightmap_image = LoadSurface("c1.bmp");
	reversecross_image = LoadSurface("badtaste.bmp");
	font_image = LoadSurface("font.bmp");
	house_image = LoadSurface("house.bmp");
	paita_image = LoadSurface("paita.bmp");

	SDL_SetColorKey(font_image, SDL_TRUE, SDL_MapRGB(font_image->format, 0x0, 0x0, 0x0));
	SDL_SetColorKey(paita_image, SDL_TRUE, SDL_MapRGB(font_image->format, 0xff, 0xff, 0x0));
	SDL_SetColorKey(reversecross_image, SDL_TRUE, SDL_MapRGB(reversecross_image->format, 0xff, 0xff, 0xff));
	reversecross_texture = SDL_CreateTextureFromSurface(ren, reversecross_image);
	font_texture = SDL_CreateTextureFromSurface(ren, font_image);
	paita_texture = SDL_CreateTextureFromSurface(ren, paita_image);

	// house mask
	for (int y2 = 0; y2 < 2; y2++) {
		for (int x2 = 0; x2 < 1; x2++) {
			for (int y = 0; y < 100; y++) {
				for (int x = 0; x < 320; x++) {
					Uint32 pixel = GetPixel(house_image, (x / 2) + x2, y + y2);
					if (pixel == 0x00000000) {
						house_mask[y * 320 + x] -= 32;
					}
				}
			}
		}
	}

	SDL_SetColorKey(house_image, SDL_TRUE, SDL_MapRGB(font_image->format, 0xff, 0xff, 0xff));

	house_texture = SDL_CreateTextureFromSurface(ren, house_image);

	if (colormap_image == NULL || heightmap_image == NULL) return 1;

	// font bounding boxes
	for (int y = 0; y < 15; y++) {
		for (int x = 0; x < 16; x++) {
			SDL_Rect rect;
			rect.x = x * 61 - (x > 0 ? 1 : 0);
			rect.y = y * 50;
			rect.w = 25 - (y < 5 ? 0 : 4);
			rect.h = 32;

			font_bb[(y * 16) + x] = rect;
		}
	}

	// height map values inserted to array
	for (int y = 0; y < 1024; y++) {
		for (int x = 0; x < 1024; x++) {
			Uint32 heightpixel = GetPixel(colormap_image, x, y);
			Uint8 *colors = (Uint8*)&heightpixel;

			Uint8 heightvalue = luma(colors[2], colors[1], colors[0]);
			colormap_buffer[y * 1024 + x] = heightvalue;

			heightmap_buffer[y * 1024 + x] = GetPixel(heightmap_image, x, y);
		}
	}

	SDL_Event e;
	bool quit = false;

	rocket = sync_create_device("sync");

#ifndef SYNC_PLAYER
	sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
#endif

	scene = sync_get_track(rocket, "scene");
	clear_r = sync_get_track(rocket, "clear.r");
	clear_g = sync_get_track(rocket, "clear.g");
	clear_b = sync_get_track(rocket, "clear.b");
	clear_a = sync_get_track(rocket, "clear.a");

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
		sync_c_a = float(sync_get_val(clear_a, row));

		sync_x = float(sync_get_val(cam_x, row));
		sync_y = float(sync_get_val(cam_y, row));
		sync_h = float(sync_get_val(cam_h, row));
		sync_hori = float(sync_get_val(cam_hori, row));
		sync_scaleh = float(sync_get_val(cam_scaleh, row));
		sync_rot = float(sync_get_val(cam_rot, row));
		sync_dist = float(sync_get_val(cam_dist, row));

		time = row * row_rate * 4;

		memset(pixels, 0, effu_w * effu_h * sizeof(Uint32));
		memset(pixelskuvio, 0, 160 * 100 * sizeof(Uint32));

		int sync_scene = (int)sync_get_val(scene, row);
		// draw scene
		switch (sync_scene) {
		case 0:
			RenderHeightmap();
			SDL_UpdateTexture(scrtexture, NULL, pixels, effu_w * sizeof(Uint32));
			SDL_RenderCopy(ren, scrtexture, NULL, NULL);
			break;
		case 1:
			RenderKefrensCross();
			SDL_RenderCopy(ren, scrtexture, NULL, NULL);
			break;
		case 2:
			RenderHouse();
			break;
		case 3:
			RenderShirt();
			//			SDL_RenderCopy(ren, scrtexture, NULL, NULL);
			break;
		case 4:
			SphereEffect();
			SDL_RenderCopy(ren, scrtexture, NULL, NULL);
			break;
		case 5:
			FireEffect();
			SDL_RenderCopy(ren, scrtexture, NULL, NULL);
			break;
		}

		DoFont();

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

