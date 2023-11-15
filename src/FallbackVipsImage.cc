
/*  DSO module loader for external image types

    Copyright (C) 2000-2012 Ruven Pillay.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
*/


#ifdef HAVE_VIPS

#include <algorithm>

#include "FallbackVipsImage.h"
#include "Logger.h"


// Reference our logging object
extern Logger logfile;


FallbackVipsImage::~FallbackVipsImage() {
    //no op
}

//https://stackoverflow.com/questions/650162/why-cant-the-switch-statement-be-applied-to-strings
constexpr unsigned int hash_switch(const char *s, int off = 0) {
    return !s[off] ? 5381 : (hash_switch(s, off+1)*33) ^ s[off];
}

void FallbackVipsImage::openImage() {

    using namespace vips;

    const string& imagePath = getFileName(0, 0); //args unused

    //todo consider implement testImageType() to support this class and hand-pick formats
    int ext = '.';
    const char* extension = NULL;
    const char* suffix = strrchr(imagePath.c_str(), ext);
    switch (hash_switch(suffix)) {
        //IIPSrv tests by the binary data /header, not suffix - potentially problematic
        case hash_switch(".png"):
        case hash_switch(".jpg"):
        case hash_switch(".jpeg"):
        case hash_switch(".webp"):
            break; //allow these formats
        default:
            throw string( "Unsupported image type: " + string(suffix));
    }

    //let vips load whatever it can (vips_image_new_from_file) and return if success

    imageObject = make_unique<VImage>(VImage::new_from_file(imagePath.c_str(),
                                        VImage::option ()->set ("access", VIPS_ACCESS_SEQUENTIAL)));

    const int width = imageObject->width();
    const int height = imageObject->height();

    //50MB raw image at most
    if (VIPS_IMAGE_SIZEOF_IMAGE(imageObject->get_image()) > 50*10e6) {
        throw "Image too big to serve! Use tiling.";
    }

    const int tile = width < height ? width:height;
    tile_width = tile;
    tile_height = tile;
    tile_widths.emplace_back(tile);
    tile_heights.emplace_back(tile);
    image_widths.emplace_back(width);
    image_heights.emplace_back(height);
    numResolutions = 1;

    colourspace = NONE;

    channels = imageObject->bands();
    int bandFormat = imageObject->format();
// use this instead...    if (vips_band_format_is8bit(bandFormat))
    switch(bandFormat) {
        case VIPS_FORMAT_UCHAR:
            bpc = sizeof(unsigned char)*8;
            sampleType = FIXEDPOINT;
            break;
        case VIPS_FORMAT_CHAR:
            bpc = sizeof(char)*8;
            sampleType = FIXEDPOINT;
            break;
        case VIPS_FORMAT_USHORT:
            bpc = sizeof(unsigned short)*8;
            sampleType = FIXEDPOINT;
            break;
        case VIPS_FORMAT_SHORT:
            bpc = sizeof(short)*8;
            sampleType = FIXEDPOINT;
            break;
        case VIPS_FORMAT_UINT:
            bpc = sizeof(unsigned int)*8;
            sampleType = FIXEDPOINT;
            break;
        case VIPS_FORMAT_INT:
            bpc = sizeof(int)*8;
            sampleType = FIXEDPOINT;
            break;
        case VIPS_FORMAT_FLOAT:
            bpc = sizeof(float)*8;
            sampleType = FLOATINGPOINT;
            break;
        case VIPS_FORMAT_DOUBLE:
            bpc = sizeof(double)*8;
            sampleType = FLOATINGPOINT;
            break;
        default:
            //todo: print also the enum type
            throw string( "Unsupported image data type!");
    }
}

void FallbackVipsImage::closeImage() {
    //noop handled by VImage destructor
}


RawTile FallbackVipsImage::getTile( int seq, int angle, unsigned int resolution, int layer, unsigned int tile ) {
    //todo better crop and then convert
    switch (imageObject->interpretation()) {
        case VIPS_INTERPRETATION_sRGB: colourspace = sRGB; break;
        case VIPS_INTERPRETATION_B_W: colourspace = GREYSCALE; break; //BINARY ignored...
        case VIPS_INTERPRETATION_LAB: colourspace = CIELAB; break;
        default:
            logfile << "Unsupported colour space: conversion to sRGB" << endl;
            imageObject = make_unique<VImage>(imageObject->colourspace(VIPS_INTERPRETATION_sRGB));
            colourspace = sRGB;
            break;
    }

    const string& imagePath = getImagePath();
    const int width = imageObject->width();
    int currentTileWidth = tile_width;
    int currentTileHeight = tile_width;
    const int height = imageObject->height();
    VImage* image = NULL;
    //odd images treated as the second part of the image data

    if (width != height) {
        if (tile == 0) {
            VImage crop = imageObject->crop(0, 0, tile_width, tile_width);
            image = &crop;
        } else {
            //odd image
            if (width > height) {
                currentTileWidth = width - tile_width;
                VImage crop = imageObject->crop(tile_width, 0, currentTileWidth, tile_width);
                image = &crop;
            } else {
                currentTileHeight = height - tile_width;
                VImage crop = imageObject->crop(0, tile_width, tile_width, currentTileHeight);
                image = &crop;
            }
        }
    } else {
        image = imageObject.get();
    }

    const void* data = image != NULL ? image->data() : NULL;
    if (data == NULL) {
        throw string( "Unable to read image " + imagePath );
    }
    const size_t length = VIPS_IMAGE_SIZEOF_IMAGE(image->get_image());

    //set BPC 8 force treat as bytes, sizeof uses bytes too
    RawTile rawtile( tile, resolution, seq, angle, currentTileWidth, currentTileHeight, channels, 8);
    rawtile.filename = imagePath;
    rawtile.allocate(length);
    memset(rawtile.data, 0, length);
    //todo two pass copy too much
    memcpy(rawtile.data, data, length);
    return rawtile;
}


#endif
