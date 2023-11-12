
/*  IIP fcgi server module

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

#ifndef _PLAINIMAGE_H
#define _PLAINIMAGE_H

#include <vips/vips8>

#include "IIPImage.h"

using namespace vips;

class FallbackVipsImage : public IIPImage {

private:
    VImage imageObject;
    unsigned int tile_width;
    unsigned int tile_height;

public:

    /// Constructor
    FallbackVipsImage() : IIPImage() {
        tile_width = 0; tile_height = 0;
        numResolutions = 0;
    };

    /// Constructor
    /** \param s image path
    */
    FallbackVipsImage( const std::string& s ) : IIPImage( s ) {
        tile_width = 0; tile_height = 0;
        numResolutions = 0;
    };

    /// Copy Constructor
    /** \param image IIPImage object
    */
    FallbackVipsImage( const IIPImage& image ) : IIPImage( image ) {
        tile_width = 0; tile_height = 0;
        numResolutions = 0;
    };

    /// Destructor
    ~FallbackVipsImage();

    /// Overloaded function to open and read the image
    void openImage();

    /// Overloaded function to close the image
    void closeImage();


    /// Overloaded function to get a specific tile
    /** Return a RawTile object: Overloaded by child class.
      \param h horizontal angle
      \param v vertical angle
      \param r resolution
      \param l quality layers
      \param t tile number
    */
    RawTile getTile( int h, int v, unsigned int r, int l, unsigned int t );
};


#endif

#endif
