#include <stdio.h>
#include <list>
#include <dirent.h>
#include <string>
#include <assert.h>

#include "compositor/piImageResizer.h"
#include "compositor/pijpegdecoder.h"
#include "PiSignageLogging.h"

extern "C"
{
#include "bcm_host.h"

}

using namespace std;

sImage * loadImage(const char *filename, uint16_t maxWidth, uint16_t maxHeight, pis_mediaSizing scaling);
void showImage(sImage *img);
void removeImage();
void initDispmanx();
void deinitDispmanx();
sImage * demoImage( uint16_t maxWidth, uint16_t maxHeight, pis_mediaSizing scaling);
uint32_t screenWidth, screenHeight;
DISPMANX_DISPLAY_HANDLE_T dispmanx_display;

typedef struct
{
    DISPMANX_DISPLAY_HANDLE_T   display;
    DISPMANX_MODEINFO_T         info;
    void                       *image;
    DISPMANX_UPDATE_HANDLE_T    update;
    DISPMANX_RESOURCE_HANDLE_T  resource;
    DISPMANX_ELEMENT_HANDLE_T   element;
    uint32_t                    vc_image_ptr;

} RECT_VARS_T;

static RECT_VARS_T  gRectVars;

int main()
{
    bcm_host_init();

    pis_loggingLevel = PIS_LOGLEVEL_ALL;

    list<string> files;
    sImage *currentImage;

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ("testImages")) != NULL) {
      /* print all the files and directories within directory */
	while ((ent = readdir (dir)) != NULL) {
	    if(string(ent->d_name).find("jpg") != -1){
		printf("Adding %s\n",ent->d_name);
		files.push_back(string("testImages/")+string(ent->d_name));
	    }
	}
	closedir (dir);
    } else {
	/* could not open directory */
	printf ("ERROR opening directory\n");
	return EXIT_FAILURE;
    }
    
    int perSlide = 5; //Time in seconds to leave the slide up

    initDispmanx();

    while(1)
    {
        for(string f : files)
        {
            printf("Loading: %s\n", f.c_str());
            printf("%d %d\n",gRectVars.info.width, gRectVars.info.height);
            currentImage = loadImage(f.c_str(), gRectVars.info.width, gRectVars.info.height, pis_SIZE_SCALE);
            //currentImage = demoImage( gRectVars.info.width, gRectVars.info.height, pis_SIZE_SCALE);
            if(currentImage == NULL){
                printf("ERROR loading image.");
                goto cleanup;
            }
            showImage(currentImage);
            usleep(perSlide * 1000000); //Seconds
            removeImage();
            delete [] currentImage->imageBuf;
        }
    }


    cleanup:

    deinitDispmanx();

    bcm_host_deinit();

    return 0;
}

sImage * demoImage( uint16_t maxWidth, uint16_t maxHeight, pis_mediaSizing scaling)
{
    sImage *ret = new sImage();
    ret->imageHeight = maxHeight;
    ret->imageWidth = maxWidth;
    ret->stride = maxWidth * 4;
    ret->imageBuf = new uint8_t[maxHeight*maxWidth*4];
    for(uint32_t i = 0;i<maxHeight*maxWidth*4;i++)
        ret->imageBuf[i] = 255;
    printf("Loaded!\n");
    return ret;
}

void initDispmanx()
{
    RECT_VARS_T    *vars;
    uint32_t        screen = 0;
    int ret;

    vars = &gRectVars;

    bcm_host_init();

    printf("Open display[%i]...\n", screen );
    vars->display = vc_dispmanx_display_open( screen );

    ret = vc_dispmanx_display_get_info( vars->display, &vars->info);
    assert(ret == 0);
    printf( "Display is %d x %d\n", vars->info.width, vars->info.height );
}

void deinitDispmanx()
{
    int ret = vc_dispmanx_resource_delete( gRectVars.resource );
    assert( ret == 0 );
    ret = vc_dispmanx_display_close( gRectVars.display );
    assert( ret == 0 );
}

void showImage(sImage *img)
{
    int             ret;
    VC_RECT_T       src_rect;
    VC_RECT_T       dst_rect;

    VC_DISPMANX_ALPHA_T alpha = { (DISPMANX_FLAGS_ALPHA_T)(DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS), 
                             255, /*alpha 0->255*/
                             0 };

    RECT_VARS_T    *vars = &gRectVars;

    uint32_t vc_image_ptr;

    vars->resource = vc_dispmanx_resource_create( VC_IMAGE_ARGB8888,
                                                  img->imageWidth,
                                                  img->imageHeight,
                                                  &vc_image_ptr);

    assert( vars->resource );
    vc_dispmanx_rect_set( &dst_rect, 0, 0, img->imageWidth, img->imageHeight);
    ret = vc_dispmanx_resource_write_data(  vars->resource,
                                            VC_IMAGE_ARGB8888,
                                            img->stride,
                                            img->imageBuf,
                                            &dst_rect );
    assert( ret == 0 );
    vars->update = vc_dispmanx_update_start( 10 );
    assert( vars->update );

    vc_dispmanx_rect_set( &src_rect, 0, 0, img->imageWidth << 16, img->imageHeight << 16 );

    vc_dispmanx_rect_set( &dst_rect, ( vars->info.width - img->imageWidth ) / 2,
                                     ( vars->info.height - img->imageHeight ) / 2,
                                     img->imageWidth,
                                     img->imageHeight );

    vars->element = vc_dispmanx_element_add(    vars->update,
                                                vars->display,
                                                2000,               // layer
                                                &dst_rect,
                                                vars->resource,
                                                &src_rect,
                                                DISPMANX_PROTECTION_NONE,
                                                &alpha,
                                                NULL,             // clamp
                                                DISPMANX_NO_ROTATE );

    ret = vc_dispmanx_update_submit_sync( vars->update );
    assert( ret == 0 );
}

void removeImage()
{
    printf("Remove image!\n");

    int             ret;
    
    gRectVars.update = vc_dispmanx_update_start( 10 );
    assert( gRectVars.update );

    vc_dispmanx_element_remove(gRectVars.update, gRectVars.element);

    ret = vc_dispmanx_update_submit_sync( gRectVars.update );

    vc_dispmanx_resource_delete(gRectVars.resource);

}

//Loads an image file and resizes to the appropriate dimensions
//Uses the Pis OpenMAX components. Decodes the file, converts it to ARGB, then resizes it.
//Must use RGB for resizing or there are a bunch of restrictions on odd dimensions
sImage * loadImage(const char *filename, uint16_t maxWidth, uint16_t maxHeight, pis_mediaSizing scaling)
{
	pis_logMessage(PIS_LOGLEVEL_FUNCTION_HEADER,"pis_MediaImageGFX::loadImage: %s\n",filename);

	sImage *ret1 = NULL;
	sImage *ret2 = NULL;
	sImage *ret3 = NULL;
	PiImageDecoder decoder;
	PiImageResizer resize;
	int err;
	bool extraLine = false;
	bool extraColumn = false;
	double endMs;
	double startMs = linuxTimeInMs();

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

    endMs = linuxTimeInMs();

    pis_logMessage(PIS_LOGLEVEL_INFO, "pis_MediaImageGFX: Resized image in %f seconds.\n",(endMs-startMs)/1000.0);
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

