#pragma once

#include <stdint.h>
#include "../PiSignageLogging.h"

enum pis_mediaSizing
{
	pis_SIZE_CROP,
	pis_SIZE_SCALE,
	pis_SIZE_STRETCH
};

struct sImage {
    uint8_t *imageBuf;
    uint32_t imageWidth;
    uint32_t imageHeight;
    uint8_t colorSpace;
    uint32_t imageSize; //in bytes

    uint32_t stride;
    uint32_t sliceHeight;
};

//ARGB 32 bit image buffer
struct pis_img
{
	uint32_t *img;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
};

#ifndef ALIGN_UP
#define ALIGN_UP(x,y)  ((x + (y)-1) & ~((y)-1))
#endif
