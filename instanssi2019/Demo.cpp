
#define _CRT_SECURE_NO_WARNINGS 1

#include <iostream>
#include <string>
#include "SDL2-2.0.9\include\SDL.h"
#include "SDL2-2.0.9\include\bass.h"

#include <sys/types.h>

#pragma comment(lib, "ws2_32.lib")

#include "sync.h"


//#define SYNC_PLAYER
#define VALOT

struct Vector4 {
	float x, y, z, w;
};
struct Vector3 {
	float x, y, z;
};
struct Vector2 {
	float x, y;
};

#define bcopy(s1, s2, n) memmove((s2), (s1), (n))

static const float bpm = 150.0f; /* beats per minute */
static const int rpb = 8; /* rows per beat */
static const double row_rate = (double(bpm) / 60) * rpb; /* rows per second */
float row = 0;

SDL_Window *win;
SDL_Renderer *ren;

SDL_Texture *scrtexture;
SDL_Texture *rtttexture;
SDL_Texture *kuviotexture;

SDL_Texture *reversecross_texture;
SDL_Texture *house_texture;
SDL_Texture *font_texture;
SDL_Texture *paita_texture;
SDL_Texture *texturerepeat_texture;
SDL_Texture *devil1_texture;
SDL_Texture *devil2_texture;

SDL_Texture* anim01;
SDL_Texture* anim02;
SDL_Texture* anim03;
SDL_Texture* anim04;
SDL_Texture* anim05;
SDL_Texture* anim06;
SDL_Texture* anim07;
SDL_Texture* anim08;
SDL_Texture* anim09;
SDL_Texture* anim10;
SDL_Texture* anim11;
SDL_Texture* anim12;
SDL_Texture* anim13;
SDL_Texture* anim14;
SDL_Texture* anim15;
SDL_Texture* anim16;
SDL_Texture* anim17;
SDL_Texture* anim18;
SDL_Texture* anim19;
SDL_Texture* anim20;
SDL_Texture* anim21;
SDL_Texture* anim22;
SDL_Texture* anim23;
SDL_Texture* anim24;
SDL_Texture* anim25;
SDL_Texture* anim26;
SDL_Texture* anim27;
SDL_Texture* anim28;
SDL_Texture* anim29;
SDL_Texture* anim30;
SDL_Texture* anim31;
SDL_Texture* anim32;
SDL_Texture* anim33;
SDL_Texture* anim34;
SDL_Texture* anim35;
SDL_Texture* anim36;
SDL_Texture* anim37;
SDL_Texture* anim38;
SDL_Texture* anim39;
SDL_Texture* anim40;
SDL_Texture* anim41;
SDL_Texture* anim42;
SDL_Texture* anim43;
SDL_Texture* anim44;
SDL_Texture* anim45;
SDL_Texture* anim46;
SDL_Texture* anim47;
SDL_Texture* anim48;
SDL_Texture* anim49;
SDL_Texture* anim50;
SDL_Texture* anim51;
SDL_Texture* anim52;
SDL_Texture* anim53;
SDL_Texture* anim54;
SDL_Texture* anim55;
SDL_Texture* anim56;
SDL_Texture* anim57;
SDL_Texture* anim58;
SDL_Texture* anim60;
SDL_Texture* anim59;


int effu_w = 320;
int effu_h = 200;

Uint32 *pixels = new Uint32[effu_w * (effu_h + 50)];
Uint32 *pixelskuvio = new Uint32[160 * 100];

SDL_Surface *colormap_image;
SDL_Surface *heightmap_image;
SDL_Surface *reversecross_image;
SDL_Surface *font_image;
SDL_Surface *house_image;
SDL_Surface *paita_image;
SDL_Surface *texturerepeat_image;
SDL_Surface *devil1_image;
SDL_Surface *devil2_image;

SDL_Rect font_bb[15 * 16] = {};

unsigned char house_mask[320 * 100] = { 0 };

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
	SDL_DestroyTexture(rtttexture);
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


int nSeed = 5323;

int prng() {
	nSeed = (8253729 * nSeed + 2396403);
	return nSeed % 32767;
}


void DoTextG(const char* text, int x, int y) {
	cursor_x = x;
	cursor_y = y;

	cursor_x = x;

	for (int i = 0; i < strlen(text); i++) {
		char letter = text[i];
		int font_y = ((int)letter - 33) / 16;
		int font_x = ((int)letter - 33) % 16;

		SDL_Rect srcrect = font_bb[(font_y * 16) + font_x];

		int wg = prng() % 16;
		int hg = prng() % 16;
		int ef = abs(255 * cos((i*0.01 + 1)*0.1 + time * 0.0001 + cos(i*0.01 + y * 0.01 + time * 0.0001) * 20));
		SDL_SetTextureColorMod(font_texture, (abs(64 - ef) + 1), (abs(92 - ef) + 1), (abs(64 - ef) + 1));

		SDL_Rect dstrect;
		dstrect.x = cursor_x;
		dstrect.y = cursor_y;
		dstrect.w = srcrect.w * 2 + wg;
		dstrect.h = srcrect.h * 2 + hg;
		SDL_RenderCopy(ren, font_texture, &srcrect, &dstrect);

		cursor_x += srcrect.w * 2;
	}

}

void DoFont() {
	SDL_SetTextureBlendMode(scrtexture, SDL_BLENDMODE_BLEND);

	int ti = (int)sync_c_a;

	if (ti == 1) DoText("______Quadtrip+Ivory+Jumalauta______", 100, 1080 / 2 - 32);
	if (ti == 2) DoText("PURGATORIUm____________", 100, 1080 / 2 - 32);

	if (ti == 16) DoText("____Not_living_and_not_yet_dead____", 100, 1080 / 2 - 32);
	if (ti == 17) DoText("______Welcome_to_hell_on_Earth_____", 100, 1080 / 2 - 32);
	if (ti == 18) DoText("_____We_are_stuck_here_together____", 100, 1080 / 2 - 32);


	if (ti == 32)DoText("______Leave_your_life_behind..._____", 100, 1080 - 100);
	if (ti == 33)DoText("______This_mortal_realm_of_man._____", 100, 1080 - 100);
	if (ti == 34)DoText("______It_is_time_for_judgement._____", 100, 1080 - 100);

	if (ti == 50)DoText("_______visy_and_branch_coded", 100, 1080 - 100);
	if (ti == 51)DoText("_paasikivi_kekkosen_linja_composed", 100, 1080 - 100);
	if (ti == 52)DoText("dysposin_spiikki_and_branch_made_gfx", 100, 1080 - 100);
	if (ti == 53)DoText("__greetings_to__mehu__paraguay__dkd", 100, 1080 - 100);
	if (ti == 54)DoText("_may_your_souls_burn_forever_bright", 100, 1080 - 100);

	//	if (ti == 1) DoText("performing_forbidden_rituals_outside", 100, 100);
}

char teksti[39] = "                                      ";

void DoFontOverlay() {
	SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

	for (int i = 0; i < 39; i++) {
		teksti[i] = 65 + (prng() % 32);
	}

	for (int y = 0; y < 30; y++) {
		DoTextG(teksti, 0, y * 35);
	}
}

void RenderHeightmap() {
	SDL_SetTextureBlendMode(scrtexture, SDL_BLENDMODE_NONE);

	SDL_SetRenderDrawColor(ren, sync_c_r, sync_c_g, sync_c_b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ren);

	DoHeightmap(sync_x, sync_y, sync_rot, sync_h, sync_hori, sync_scaleh, sync_dist, 0);
}

SDL_Surface *sshot;
void RenderKefrensCross() {


	SDL_SetRenderTarget(ren, NULL);
	//	SDL_SetTextureBlendMode(rtttexture, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(ren, sync_c_r, sync_c_g, sync_c_b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ren);
	SDL_Rect dstrect;
	SDL_SetRenderTarget(ren, rtttexture);
	SDL_RenderClear(ren);
	dstrect.w = 128;
	dstrect.h = 128;
	for (int i = 0; i < 98; i += 2) {
		dstrect.x = 320 / 2 - 64 + 100 * sin(sync_dist + 2.0f *  3.141592f * i / 200) * cos(sync_rot + 2.0f *  3.141592f * i / 128);
		dstrect.y = 200 / 2 - 64 + i - 90 + sin(sync_dist*0.321f + i * 0.05f) * 20;
		SDL_SetTextureColorMod(reversecross_texture, 255+i/2-50, 255+i-100, 255+i-100);
		SDL_RenderCopy(ren, reversecross_texture, NULL, &dstrect);
	}

	int w = 1920;
	int h = 1080;
	if (sshot == NULL)
		sshot = SDL_CreateRGBSurface(0, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

	SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);

	for (int x = 0; x < 320 - 16; x++) {
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
	SDL_SetRenderTarget(ren, NULL);
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
	uv.x *= 2.0;
	uv.y *= 2.0;
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

	SDL_SetRenderTarget(ren, rtttexture);
	SDL_SetTextureBlendMode(rtttexture, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(ren, sync_c_r, sync_c_g, sync_c_b, SDL_ALPHA_OPAQUE);
	float ax = sin(sync_x) * 256.0f;
	float ay = cos(sync_y) * 256.0f;
	flick++;
	for (int y = 0; y < 200; y += 1) {
		for (int x = 0; x < 320; x++) {
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
			int d3 = uv.z * 114.0f;
			auto pixel = GetPixel(texturerepeat_image, r, g);

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
	//SDL_FreeSurface(sshot);
}

void RenderHouse();

void FireEffect() {
	SDL_SetRenderTarget(ren, rtttexture);
	SDL_SetTextureBlendMode(rtttexture, SDL_BLENDMODE_BLEND);
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
		for (int x = flick & 1; x < 319; x += 2) {
			/*
			auto pixel = GetPixel(sshot, x, y);
			Uint8 r, g, b, a;
			SDL_GetRGBA(pixel, sshot->format, &r, &g, &b, &a);
			*/
			Uint8 r, g, b, a;
			auto pixel = GetPixel(sshot, x + 1, min(y + (rand() & 1 + 1), 199));

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

	SDL_Rect dstrect1;
	dstrect1.w = 320;
	dstrect1.h = 200;
	dstrect1.x = 0;
	dstrect1.y = 0;

	SDL_Rect dstrect2;
	dstrect2.w = 320;
	dstrect2.h = 200;
	dstrect2.x = 0;
	dstrect2.y = 0;

	if (sync_hori <= 1.0f) {
		if (sync_hori < 0.0f) {
			Uint8 r0, g0, b0, a0;
			for (int y = 0; y < 100; y+=2) {
				for (int x = 0; x < 160; x++) {
					auto pixel0 = GetPixel(house_image, x, y);
					r0 = (pixel0 >> 16) & 255;
					g0 = (pixel0 >> 8) & 255;
					b0 = (pixel0 & 255);
					if (r0 + g0 + b0 < 3) {
						set_pixel(ren, x+50, y+50, y,y,y,255);
					}
				}
			}
		} else if (sync_hori == 0.0f) {
			SDL_RenderCopy(ren, devil1_texture, NULL, &dstrect1);
		}
		else if (sync_hori == 1.0f) {
			SDL_RenderCopy(ren, devil2_texture, NULL, &dstrect2);
		}
		else {
			Uint8 r0, g0, b0, a0;
			Uint8 r1, g1, b1, a1;
			for (int y = 0; y < 200; y++) {
				for (int x = 0; x < 320; x++) {
					auto pixel0 = GetPixel(devil1_image, x, y);
					r0 = (pixel0 >> 16) & 255;
					g0 = (pixel0 >> 8) & 255;
					b0 = (pixel0 & 255);
					auto pixel1 = GetPixel(devil2_image, x, y);
					r1 = (pixel1 >> 16) & 255;
					g1 = (pixel1 >> 8) & 255;
					b1 = (pixel1 & 255);

					float r = min(max(sync_hori + (rand() % 10) / 100.0f,0.0f),1.0f);
					// outoa

					r0 *= (1.0f - r);
					g0 *= (1.0f - r);
					b0 *= (1.0f - r);
					r1 *= r;
					g1 *= r;
					b1 *= r;

					r0 = (int)(r0 + r1) & 255;
					g0 = (int)(g0 + g1) & 255;
					b0 = (int)(b0 + b1) & 255;
					a0 = 255;
					if (r0 + g0 + b0 > 3) {
						set_pixel(ren, x, y, r0, g0, b0, 255);
					}
				}
			}
		}
	}
	SDL_SetRenderTarget(ren, NULL);
	//SDL_FreeSurface(sshot);
}

void RenderHouse() {
	SDL_SetRenderTarget(ren, NULL);
	SDL_SetTextureBlendMode(scrtexture, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(ren, sync_c_r, sync_c_g, sync_c_b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ren);

	for (int y = 0; y < 100; y++) {
		for (int x = 1; x < 320; x++) {
			int xs = x + ((effu_w - 2) / 32) - 10;
			int ys = y * 1.3;
			pixels[(ys * effu_w) + xs] = house_mask[(y * 320) + x - 10] * 0x00111111;
			pixels[((ys + 1) * effu_w) + xs] = house_mask[(y * 320) + x - 10] * 0x00111111;
		}
	}

	for (int y = 100; y < 130; y++) {
		for (int x = 1; x < 320; x++) {
			int xs = x + ((effu_w - 2) / 32) - 10;
			int ys = y * 1.3;
			xs += cos(time*0.001 + y * 0.5)*(6 - abs(cos(y*0.1) * 3));
			pixels[ys * effu_w + xs] = house_mask[(((230 - y) - 30) * 320) + x - 10] * 0x00040033;
		}
	}

	SDL_UpdateTexture(scrtexture, NULL, pixels, effu_w * sizeof(Uint32));
	SDL_RenderCopy(ren, scrtexture, NULL, NULL);

	SDL_SetTextureBlendMode(scrtexture, SDL_BLENDMODE_BLEND);

	SDL_SetTextureAlphaMod(house_texture, 180);
	SDL_Rect dstrect;

	dstrect.x = effu_w / 7;
	dstrect.y = 1;
	dstrect.w = effu_w * 6;
	dstrect.h = effu_h * 3.5;
	SDL_RenderCopy(ren, house_texture, NULL, &dstrect);
}

int startrow = -1;
int currentrow = 0;
void DoAnimPlay() {

	if (startrow == -1) startrow = row;

	SDL_SetRenderTarget(ren, rtttexture);
	SDL_SetTextureBlendMode(rtttexture, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(ren, sync_c_r, sync_c_g, sync_c_b, SDL_ALPHA_OPAQUE);

	currentrow = row - startrow;
	if (currentrow == 60) { startrow = row; currentrow = 0; }

	if (currentrow == 0) SDL_RenderCopy(ren, anim01, NULL, NULL);
	if (currentrow == 1) SDL_RenderCopy(ren, anim02, NULL, NULL);
	if (currentrow == 2) SDL_RenderCopy(ren, anim03, NULL, NULL);
	if (currentrow == 3) SDL_RenderCopy(ren, anim04, NULL, NULL);
	if (currentrow == 4) SDL_RenderCopy(ren, anim05, NULL, NULL);
	if (currentrow == 5) SDL_RenderCopy(ren, anim06, NULL, NULL);
	if (currentrow == 6) SDL_RenderCopy(ren, anim07, NULL, NULL);
	if (currentrow == 7) SDL_RenderCopy(ren, anim08, NULL, NULL);
	if (currentrow == 8) SDL_RenderCopy(ren, anim09, NULL, NULL);
	if (currentrow == 9) SDL_RenderCopy(ren, anim10, NULL, NULL);
	if (currentrow == 10) SDL_RenderCopy(ren, anim11, NULL, NULL);
	if (currentrow == 11) SDL_RenderCopy(ren, anim12, NULL, NULL);
	if (currentrow == 12) SDL_RenderCopy(ren, anim13, NULL, NULL);
	if (currentrow == 13) SDL_RenderCopy(ren, anim14, NULL, NULL);
	if (currentrow == 14) SDL_RenderCopy(ren, anim15, NULL, NULL);
	if (currentrow == 15) SDL_RenderCopy(ren, anim16, NULL, NULL);
	if (currentrow == 16) SDL_RenderCopy(ren, anim17, NULL, NULL);
	if (currentrow == 17) SDL_RenderCopy(ren, anim18, NULL, NULL);
	if (currentrow == 18) SDL_RenderCopy(ren, anim19, NULL, NULL);
	if (currentrow == 19) SDL_RenderCopy(ren, anim20, NULL, NULL);
	if (currentrow == 20) SDL_RenderCopy(ren, anim21, NULL, NULL);
	if (currentrow == 21) SDL_RenderCopy(ren, anim22, NULL, NULL);
	if (currentrow == 22) SDL_RenderCopy(ren, anim23, NULL, NULL);
	if (currentrow == 23) SDL_RenderCopy(ren, anim24, NULL, NULL);
	if (currentrow == 24) SDL_RenderCopy(ren, anim25, NULL, NULL);
	if (currentrow == 25) SDL_RenderCopy(ren, anim26, NULL, NULL);
	if (currentrow == 26) SDL_RenderCopy(ren, anim27, NULL, NULL);
	if (currentrow == 27) SDL_RenderCopy(ren, anim28, NULL, NULL);
	if (currentrow == 28) SDL_RenderCopy(ren, anim29, NULL, NULL);
	if (currentrow == 29) SDL_RenderCopy(ren, anim30, NULL, NULL);
	if (currentrow == 30) SDL_RenderCopy(ren, anim31, NULL, NULL);
	if (currentrow == 31) SDL_RenderCopy(ren, anim32, NULL, NULL);
	if (currentrow == 32) SDL_RenderCopy(ren, anim33, NULL, NULL);
	if (currentrow == 33) SDL_RenderCopy(ren, anim34, NULL, NULL);
	if (currentrow == 34) SDL_RenderCopy(ren, anim35, NULL, NULL);
	if (currentrow == 35) SDL_RenderCopy(ren, anim36, NULL, NULL);
	if (currentrow == 36) SDL_RenderCopy(ren, anim37, NULL, NULL);
	if (currentrow == 37) SDL_RenderCopy(ren, anim38, NULL, NULL);
	if (currentrow == 38) SDL_RenderCopy(ren, anim39, NULL, NULL);
	if (currentrow == 39) SDL_RenderCopy(ren, anim40, NULL, NULL);
	if (currentrow == 40) SDL_RenderCopy(ren, anim41, NULL, NULL);
	if (currentrow == 41) SDL_RenderCopy(ren, anim42, NULL, NULL);
	if (currentrow == 42) SDL_RenderCopy(ren, anim43, NULL, NULL);
	if (currentrow == 43) SDL_RenderCopy(ren, anim44, NULL, NULL);
	if (currentrow == 44) SDL_RenderCopy(ren, anim45, NULL, NULL);
	if (currentrow == 45) SDL_RenderCopy(ren, anim46, NULL, NULL);
	if (currentrow == 46) SDL_RenderCopy(ren, anim47, NULL, NULL);
	if (currentrow == 47) SDL_RenderCopy(ren, anim48, NULL, NULL);
	if (currentrow == 48) SDL_RenderCopy(ren, anim49, NULL, NULL);
	if (currentrow == 49) SDL_RenderCopy(ren, anim50, NULL, NULL);
	if (currentrow == 50) SDL_RenderCopy(ren, anim51, NULL, NULL);
	if (currentrow == 51) SDL_RenderCopy(ren, anim52, NULL, NULL);
	if (currentrow == 52) SDL_RenderCopy(ren, anim53, NULL, NULL);
	if (currentrow == 53) SDL_RenderCopy(ren, anim54, NULL, NULL);
	if (currentrow == 54) SDL_RenderCopy(ren, anim55, NULL, NULL);
	if (currentrow == 55) SDL_RenderCopy(ren, anim56, NULL, NULL);
	if (currentrow == 56) SDL_RenderCopy(ren, anim57, NULL, NULL);
	if (currentrow == 57) SDL_RenderCopy(ren, anim58, NULL, NULL);
	if (currentrow == 58) SDL_RenderCopy(ren, anim59, NULL, NULL);
	if (currentrow == 59) SDL_RenderCopy(ren, anim60, NULL, NULL);
	SDL_SetRenderTarget(ren, NULL);

}

int gw = 160;
int gh = 100;

int g2w = 320;
int g2h = 200;

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
	dstrect.y = effu_h * 1.8;
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

int litecounter = 0;

int main(int argc, char * argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	win = SDL_CreateWindow("instanssi 2019 demo", 8, 8, 1920, 1080, SDL_WINDOW_SHOWN);
	SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);

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
	rtttexture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, effu_w, effu_h);

	colormap_image = LoadSurface("d1.bmp");
	heightmap_image = LoadSurface("c1.bmp");
	reversecross_image = LoadSurface("badtaste.bmp");
	font_image = LoadSurface("font.bmp");
	house_image = LoadSurface("house.bmp");
	paita_image = LoadSurface("paita.bmp");
	texturerepeat_image = LoadSurface("textureloop.bmp");
	devil1_image = LoadSurface("smear1.bmp");
	devil2_image = LoadSurface("smear2.bmp");

	anim01 = LoadTexture("0001.bmp");
	anim02 = LoadTexture("0002.bmp");
	anim03 = LoadTexture("0003.bmp");
	anim04 = LoadTexture("0004.bmp");
	anim05 = LoadTexture("0005.bmp");
	anim06 = LoadTexture("0006.bmp");
	anim07 = LoadTexture("0007.bmp");
	anim08 = LoadTexture("0008.bmp");
	anim09 = LoadTexture("0009.bmp");
	anim10 = LoadTexture("0010.bmp");
	anim11 = LoadTexture("0011.bmp");
	anim12 = LoadTexture("0012.bmp");
	anim13 = LoadTexture("0013.bmp");
	anim14 = LoadTexture("0014.bmp");
	anim15 = LoadTexture("0015.bmp");
	anim16 = LoadTexture("0016.bmp");
	anim17 = LoadTexture("0017.bmp");
	anim18 = LoadTexture("0018.bmp");
	anim19 = LoadTexture("0019.bmp");
	anim20 = LoadTexture("0020.bmp");
	anim21 = LoadTexture("0021.bmp");
	anim22 = LoadTexture("0022.bmp");
	anim23 = LoadTexture("0023.bmp");
	anim24 = LoadTexture("0024.bmp");
	anim25 = LoadTexture("0025.bmp");
	anim26 = LoadTexture("0026.bmp");
	anim27 = LoadTexture("0027.bmp");
	anim28 = LoadTexture("0028.bmp");
	anim29 = LoadTexture("0029.bmp");
	anim30 = LoadTexture("0030.bmp");
	anim31 = LoadTexture("0031.bmp");
	anim32 = LoadTexture("0032.bmp");
	anim33 = LoadTexture("0033.bmp");
	anim34 = LoadTexture("0034.bmp");
	anim35 = LoadTexture("0035.bmp");
	anim36 = LoadTexture("0036.bmp");
	anim37 = LoadTexture("0037.bmp");
	anim38 = LoadTexture("0038.bmp");
	anim39 = LoadTexture("0039.bmp");
	anim40 = LoadTexture("0040.bmp");
	anim41 = LoadTexture("0041.bmp");
	anim42 = LoadTexture("0042.bmp");
	anim43 = LoadTexture("0043.bmp");
	anim44 = LoadTexture("0044.bmp");
	anim45 = LoadTexture("0045.bmp");
	anim46 = LoadTexture("0046.bmp");
	anim47 = LoadTexture("0047.bmp");
	anim48 = LoadTexture("0048.bmp");
	anim49 = LoadTexture("0049.bmp");
	anim50 = LoadTexture("0050.bmp");
	anim51 = LoadTexture("0051.bmp");
	anim52 = LoadTexture("0052.bmp");
	anim53 = LoadTexture("0053.bmp");
	anim54 = LoadTexture("0054.bmp");
	anim55 = LoadTexture("0055.bmp");
	anim56 = LoadTexture("0056.bmp");
	anim57 = LoadTexture("0057.bmp");
	anim58 = LoadTexture("0058.bmp");
	anim59 = LoadTexture("0059.bmp");
	anim60 = LoadTexture("0060.bmp");


	SDL_SetColorKey(font_image, SDL_TRUE, SDL_MapRGB(font_image->format, 0x0, 0x0, 0x0));
	SDL_SetColorKey(paita_image, SDL_TRUE, SDL_MapRGB(font_image->format, 0xff, 0xff, 0x0));
	SDL_SetColorKey(reversecross_image, SDL_TRUE, SDL_MapRGB(reversecross_image->format, 0xff, 0xff, 0xff));
	SDL_SetColorKey(devil1_image, SDL_TRUE, SDL_MapRGB(reversecross_image->format, 0x00, 0x00, 0x00));
	SDL_SetColorKey(devil2_image, SDL_TRUE, SDL_MapRGB(reversecross_image->format, 0x00, 0x00, 0x00));

	reversecross_texture = SDL_CreateTextureFromSurface(ren, reversecross_image);
	font_texture = SDL_CreateTextureFromSurface(ren, font_image);
	paita_texture = SDL_CreateTextureFromSurface(ren, paita_image);
	texturerepeat_texture = SDL_CreateTextureFromSurface(ren, texturerepeat_image);
	devil1_texture = SDL_CreateTextureFromSurface(ren, devil1_image);
	devil2_texture = SDL_CreateTextureFromSurface(ren, devil2_image);

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

	int sock, n;
	unsigned int length;
	struct sockaddr_in server;
	struct hostent *hp;
	//lightnum  //color
	char buffer[159] = { 0x1, 0x1,0x00,0x00,  0xff,0x00,0x00,
							0x1,1, 0, 0,0,0,
							0x1,2, 0, 0,0,0,
							0x1,3, 0, 0,0,0,
							0x1,4, 0, 0,0,0,
							0x1,5, 0, 0,0,0,
							0x1,6, 0, 0,0,0,
							0x1,7, 0, 0,0,0,
							0x1,8, 0, 0,0,0,
							0x1,9, 0, 0,0,0,
							0x1,10, 0, 0,0,0,
							0x1,11, 0, 0,0,0,
							0x1,12, 0, 0,0,0,
							0x1,13, 0, 0,0,0,
							0x1,14, 0, 0,0,0,
							0x1,15, 0, 0,0,0,
							0x1,16, 0, 0,0,0,
							0x1,17, 0, 0,0,0,
							0x1,18, 0, 0,0,0,
							0x1,19, 0, 0,0,0,
							0x1,20, 0, 0,0,0,
							0x1,21, 0, 0,0,0,
							0x1,22, 0, 0,0,0,
							0x1,23, 0, 0,0,0,
							0x1,24, 0, 0,0,0
	};
	hp = gethostbyname("valot.party");
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	int removeme;
	server.sin_family = AF_INET;
	bcopy((char *)hp->h_addr,
		(char *)&server.sin_addr,
		hp->h_length);
	server.sin_port = htons(atoi("9909"));

	length = sizeof(struct sockaddr_in);
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
			SDL_RenderCopy(ren, rtttexture, NULL, NULL);
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
			SDL_RenderCopy(ren, rtttexture, NULL, NULL);
			break;
		case 5:
			FireEffect();
			SDL_RenderCopy(ren, rtttexture, NULL, NULL);
			break;
		case 6:
			DoFontOverlay();
			break;
		case 7:
			DoAnimPlay();
			SDL_RenderCopy(ren, rtttexture, NULL, NULL);
			break;
		}
		DoFont();

		SDL_RenderPresent(ren);

		BASS_Update(0); /* decrease the chance of missing vsync */

		if (sync_scene == 0) {
			for (int i = 0; i < 151; i += 6) {
				buffer[4 + i] = 200 + sin(time*0.005 + i * 0.1) * 50;
				buffer[5 + i] = 128 + sin(time*0.005 + i * 0.1) * 64;
				buffer[6 + i] = 128 + sin(time*0.005 + i * 0.1) * 128;
			}
#ifdef VALOT
			n = sendto(sock, buffer, sizeof(buffer) - 8, 0, (const struct sockaddr *)&server, length);
#endif
		}

		if (sync_scene == 1) {
			for (int i = 0; i < 151 - 6; i += 6) {
				buffer[4 + i] = 128 + sin(time*0.005 + i * 1.1) * 127;
				buffer[5 + i] = 128 + sin(time*0.005 + i * 1.1) * 127;
				buffer[6 + i] = 0;
			}
#ifdef VALOT
			n = sendto(sock, buffer, sizeof(buffer)-8, 0, (const struct sockaddr *)&server, length);
#endif
			}


		if (sync_scene == 3) {
			for (int i = 0; i < 151; i += 6) {
				buffer[4 + i] = 64;
				buffer[5 + i] = 16;
				buffer[6 + i] = 16;
			}
#ifdef VALOT
			n = sendto(sock, buffer, sizeof(buffer) - 8, 0, (const struct sockaddr *)&server, length);
#endif
			}

		if (sync_scene == 4) {
			for (int i = 0; i < 151; i += 6) {
				buffer[4 + i] = 128 + sin(time*0.01 + i * 0.3) * 128;
				buffer[5 + i] = 128 + sin(time*0.01 + i * 0.3) * 128;
				buffer[6 + i] = 128 + sin(time*0.01 + i * 0.3) * 128;
			}
#ifdef VALOT
			n = sendto(sock, buffer, sizeof(buffer) - 8, 0, (const struct sockaddr *)&server, length);
#endif
			}

		if (sync_scene == 5) {
			for (int i = 0; i < 151 - 6; i += 6) {
				buffer[4 + i] = 128 + sin(time*0.1 + i * 0.3) * 128;
				buffer[5 + i] = 0;
				buffer[6 + i] = 0;
			}
#ifdef VALOT
			n = sendto(sock, buffer, sizeof(buffer) - 8, 0, (const struct sockaddr *)&server, length);
#endif
			}

		if (sync_scene == 6) {
			for (int i = 0; i < 151 - 6; i += 6) {
				buffer[4 + i] = 128 + sin(i*100. + time * 0.1) * 127;
				buffer[5 + i] = 255;
				buffer[6 + i] = 255;
			}
#ifdef VALOT
			n = sendto(sock, buffer, sizeof(buffer) - 8, 0, (const struct sockaddr *)&server, length);
#endif
			}


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