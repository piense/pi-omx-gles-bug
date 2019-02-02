#include <string>

#include "PiMediaImageGFX.h"
#include "../compositor/piImageResizer.h"
#include "../compositor/pijpegdecoder.h"

using namespace std;

pis_MediaImageGFX::pis_MediaImageGFX(){
	pis_logMessage(PIS_LOGLEVEL_FUNCTION_HEADER,"pis_MediaImageGFX::pis_MediaImage\n");

	X = 0;
	Y = 0;
	MaxWidth = 1;
	MaxHeight = 1;
	ScreenWidth = 0;
	ScreenHeight = 0;
	Scaling = pis_SIZE_SCALE;

	ImgCache.img = NULL;

	mGLTex = 0;
	mLoaded = false;
}

pis_MediaImageGFX::~pis_MediaImageGFX()
{

}

//Loads an image file and resizes to the appropriate dimensions
//Uses the Pis OpenMAX components. Decodes the file, converts it to ARGB, then resizes it.
//Must use RGB for resizing or there are a bunch of restrictions on odd dimensions
sImage * loadImage(const char *filename, uint16_t maxWidth, uint16_t maxHeight, pis_mediaSizing scaling)
{

	sImage *ret1 = NULL;
	sImage *ret2 = NULL;
	sImage *ret3 = NULL;
	PiImageDecoder decoder;
	PiImageResizer resize;
	int err;
	bool extraLine = false;
	bool extraColumn = false;

	//TODO: Create a component with the decoding, color space, and resize tunneled together

	err = decoder.DecodeJpegImage(filename, &ret1);

	if(err != 0 || ret1 == NULL)
	{
		pis_logMessage(PIS_LOGLEVEL_ERROR, "loadImage: Error loading JPEG %s\n", filename);
		goto error;
	}

	pis_logMessage(PIS_LOGLEVEL_ALL,"pis_MediaImageGFX: Returned from decoder: %dx%d Size: %d Stride: %d Slice Height: %d\n",
			ret1->imageWidth,ret1->imageHeight,
			ret1->imageSize, ret1->stride, ret1->sliceHeight
			);

	//Decoder buffer logic makes this safe, sliceHeight and stride are always rounded up to an even number
	if(ret1->imageHeight%2 != 0) { ret1->imageHeight += 1; extraLine = true; }
	if(ret1->imageWidth%2 != 0) { ret1->imageWidth += 1; extraColumn = true; }

	//Resizer does odd things with YUV so
	//Convert to RGB888 first
	err =  resize.ResizeImage (
			(char *)ret1->imageBuf,
			ret1->imageWidth, ret1->imageHeight,
			ret1->imageSize,
			(OMX_COLOR_FORMATTYPE) ret1->colorSpace,
			ret1->stride,
			ret1->sliceHeight,
			ret1->imageWidth, ret1->imageHeight,
			pis_SIZE_STRETCH,
			OMX_COLOR_Format32bitARGB8888,
			&ret2
			);

	if(extraLine == true) ret2->imageHeight--;
	if(extraColumn == true) ret2->imageWidth--;

	if(err != 0 || ret2 == NULL)
	{
		pis_logMessage(PIS_LOGLEVEL_ERROR, "pis_MediaImageGFX::loadImage: Error converting color space\n");
		goto error;
	}

	//Finally get the image into the size we need it
	err =  resize.ResizeImage (
			(char *)ret2->imageBuf,
			ret2->imageWidth, ret2->imageHeight,
			ret2->imageSize,
			(OMX_COLOR_FORMATTYPE) ret2->colorSpace,
			ret2->stride,
			ret2->sliceHeight,
			maxWidth,maxHeight,
			scaling,
			OMX_COLOR_Format32bitARGB8888,
			&ret3
			);

	if(err != 0 || ret3 == NULL)
	{
		pis_logMessage(PIS_LOGLEVEL_ERROR, "pis_MediaImageGFX::loadImage: Error resizing image.\n");
		goto error;
	}

	delete [] ret2->imageBuf;
	delete ret2;

    delete [] ret1->imageBuf;

    delete ret1;


	return ret3;

	error:
	if(ret1 != NULL && ret1->imageBuf != NULL)
		delete [] ret1->imageBuf;
	if(ret1 != NULL)
		delete ret1;

	if(ret2 != NULL && ret2->imageBuf != NULL)
		delete [] ret2->imageBuf;
	if(ret2 != NULL)
		delete ret2;

	if(ret3 != NULL)
		delete ret3;

	return NULL;
}

int pis_MediaImageGFX::SetScreenSize(
			uint32_t w, uint32_t h)
{
	pis_logMessage(PIS_LOGLEVEL_FUNCTION_HEADER,"pis_MediaImageGFX::SetGraphicsHandles\n");

	ScreenWidth = w;
	ScreenHeight = h;
	return 0;
}

//Called when a slide is loading resources
int pis_MediaImageGFX::Load( )
{
	pis_logMessage(PIS_LOGLEVEL_FUNCTION_HEADER,"pis_MediaImageGFX::Load\n");

	if(mGLTex == 0){
		sImage *img = loadImage(Filename.c_str(),
				MaxWidth * ScreenWidth,
				MaxHeight * ScreenHeight,
				Scaling);

		if(img == NULL || img->imageBuf == NULL)
		{
			pis_logMessage(PIS_LOGLEVEL_ERROR,"pis_MediaImageGFX::Load Error loading image\n");
			//TODO SOmething
			return -1;
		}

		ImgCache.img = (uint32_t *) img->imageBuf;
		ImgCache.width =  img->imageWidth;
		ImgCache.height =  img->imageHeight;
		ImgCache.stride = img->stride;

		delete img;

		glGenTextures(1, &mGLTex);

		ARGB_TO_RGBA(ImgCache.width, ImgCache.height, ImgCache.stride, ImgCache.img);

		// setup texture
		glBindTexture(GL_TEXTURE_2D, mGLTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ImgCache.width, ImgCache.height, 0,
					GL_RGBA, GL_UNSIGNED_BYTE, ImgCache.img);

		QuadFromRect
		( (int32_t)(X*ScreenWidth - ImgCache.width/2),
					(int32_t)(ScreenHeight - Y*ScreenHeight - ImgCache.height/2),
					(int32_t)ImgCache.width,(int32_t)ImgCache.height,mQuad);

		delete [] ImgCache.img;
		ImgCache.img = NULL;
		mLoaded = true;
	}else{
		pis_logMessage(PIS_LOGLEVEL_ERROR, "pis_MediaImageGFX::Load ERROR mGLTex already loaded\n");
	}


	return 0;
}

//Called when a slide is unloading resources
int pis_MediaImageGFX::Unload()
{
	pis_logMessage(PIS_LOGLEVEL_FUNCTION_HEADER,"pis_MediaImageGFX::Unload\n");

	if(mGLTex != 0){
		glDeleteTextures(1, &mGLTex);
		mLoaded = false;
	}

	mGLTex = 0;

	ImgCache.img = NULL;
	ImgCache.width =  0;
	ImgCache.height =  0;
	ImgCache.stride = 0;

	return 0;

}


void pis_MediaImageGFX::GLDraw()
{
	if(mLoaded)
	{
		// setup first texture
		glBindTexture(GL_TEXTURE_2D, mGLTex);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_NEAREST);

		// setup overall texture environment
		glTexCoordPointer(2, GL_FLOAT, 0, mTexCoords);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glVertexPointer( 3, GL_SHORT, 0, mQuad );

		// Draw first (front) face:
		// Bind texture surface to current vertices
		glBindTexture(GL_TEXTURE_2D, mGLTex);

		// draw first 4 vertices
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4);
	}
}


//Utility function since everything seems to like ARGB but GLES uses RGBA

typedef uint32_t Color32;


//Moves bytes around for OMX (32-bit words) ARGB to GL RGBA (bytes)
inline Color32 Color32Reverse(Color32 x)
{

    return                            
        ((x & 0xFF00FF00) ) | 
        ((x & 0x00FF0000) >>  16) | 
        ((x & 0x000000FF) <<  16);  
}

// Shuffles the bytes and tight packs the buffer
void ARGB_TO_RGBA(uint32_t width, uint32_t height, uint32_t stride, uint32_t *buf)
{
       
    uint32_t dstAt = 0;
    uint32_t srcAt = 0;
    uint32_t newStride = width;
    uint32_t smallStride = stride / 4;

    for(int32_t y=0;y<height;y++)
    {
        for(int32_t x = 0;x<width;x++){
            buf[dstAt+x] = Color32Reverse(buf[srcAt+x]);
        }
        dstAt += newStride;
        srcAt += smallStride;
    }

}

void QuadFromRect(short x, short y, short width, short height, GLshort *quad)
{
	quad[0] = x;
	quad[1] = y;
	quad[2] = 0; ///

	quad[3] = x+width; 
	quad[4] = y;
	quad[5] = 0; //

	quad[6] = x; 
	quad[7] = y+height;
	quad[8] = 0; //

	quad[9] = x+width; 
	quad[10] = y+height;
	quad[11] = 0; ///
}