
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

#include "FallbackVipsImage.h"
#include "Logger.h"

using namespace std;

// Reference our logging object
extern Logger logfile;


FallbackVipsImage::~FallbackVipsImage() {
    //no op
}


void FallbackVipsImage::openImage() {

    using namespace vips;
    if (VIPS_INIT("")) {
        vips_error_exit(NULL);
    }

    const string& imagePath = getImagePath();

//todo consider implement testImageType() to support this class and hand-pick formats

//    switch (suffix) {
//        //IIPSrv tests by the binary data /header, not suffix - potentially problematic
//        case "png":
//            vips_pngload(imagePath, &imageHandle); break;
//        case "jpg":
//        case: "jpeg":
//            vips_jpegload(imagePath, &imageHandle); break;
//        case: "webp":
//            vips_webpload(imagePath, &imageHandle); break;
//        default:
//            throw string( "Unsupported image type: " + suffix);
//    }

    //let vips load whatever it can (vips_image_new_from_file) and return if success

    imageObject = VImage::new_from_file(imagePath.c_str(),
                                        VImage::option ()->set ("access", VIPS_ACCESS_SEQUENTIAL));

    const int width = imageObject.width();
    const int height = imageObject.height();
    tile_width = width;
    tile_height = height;
    tile_widths.emplace_back(width);
    tile_heights.emplace_back(height);
    image_widths.emplace_back(width);
    image_heights.emplace_back(height);
    numResolutions = 1;

    colourspace = NONE;
    switch (imageObject.interpretation()) {
        case VIPS_INTERPRETATION_sRGB: colourspace = sRGB; break;
        case VIPS_INTERPRETATION_B_W: colourspace = GREYSCALE; break; //BINARY ignored...
        case VIPS_INTERPRETATION_LAB: colourspace = CIELAB; break;
        default:
            logfile << "Unsupported colour space: conversion to sRGB" << endl;
            imageObject = imageObject.colourspace(VIPS_INTERPRETATION_sRGB);
            colourspace = sRGB;
            break;
    }

    channels = imageObject.bands();
    int bandFormat = imageObject.format();
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

    const string& imagePath = getImagePath();
    const void* data = imageObject.data();
    if (data == NULL) {
        //todo find cause of failure some vips error decription function
        throw string( "Unable to read image " + imagePath );
    }

    RawTile rawtile( tile, resolution, seq, angle, tile_width, tile_height, channels, bpc);
    rawtile.filename = imagePath;
    rawtile.allocate();
    //TODO two pass copy too much? but we need to remoce const... is that safe?
    memcpy(rawtile.data, data, rawtile.capacity);
    return rawtile;
}


#endif
