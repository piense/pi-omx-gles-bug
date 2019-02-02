#pragma once

#include <string>
#include <atomic>

extern "C"
{
//External libs
#include "bcm_host.h"
}

#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "../compositor/imagecommon.h"

void ARGB_TO_RGBA(uint32_t width, uint32_t height, uint32_t stride, uint32_t *buf);
void QuadFromRect(short x, short y, short width, short height, GLshort *quad);

class pis_MediaImageGFX 
{
public:
	pis_MediaImageGFX();
	~pis_MediaImageGFX();

	int SetScreenSize(uint32_t Width, uint32_t Height);

	//Called when a slide is loading resources
	int Load(void);

	//Called when a slide is unloading resources
	int Unload();

	void GLDraw();

	//Used to identify media type in XML
	std::string GetType();

	float X, Y, MaxWidth, MaxHeight;
	uint32_t ScreenWidth, ScreenHeight;

	std::string Filename;
	pis_mediaSizing Scaling;
	pis_img ImgCache;
	
	std::atomic<bool> mLoaded;

private:

	GLuint mGLTex;

	GLshort mQuad[3*4];

	GLfloat mTexCoords[4 * 2]  = {
	0.f,  1.f,
	1.f,  1.f,
	0.f,  0.f,
	1.f,  0.f,
	};

};
