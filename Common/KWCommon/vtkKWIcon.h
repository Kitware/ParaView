/*=========================================================================

  Module:    vtkKWIcon.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWIcon - simple wrapper for icons
// .SECTION Description
// A simple icon wrapper. It can either be used with file icons.h to 
// provide a unified interface for internal icons or a wrapper for 
// custom icons. The icons are defined with width, height, pixel_size, 
// and array of unsigned char values.

#ifndef __vtkKWIcon_h
#define __vtkKWIcon_h

#include "vtkObject.h"

class vtkKWApplication;
class vtkKWIcon;
class vtkImageData;

class VTK_EXPORT vtkKWIcon : public vtkObject
{
public:
  static vtkKWIcon* New();
  vtkTypeRevisionMacro(vtkKWIcon,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // There are several predefined icons in the icons.h. Since we
  // want to save space, we only incldue that file to vtkKWIcons.cxx.
  // These constants specify different icons.
  enum { 
    ICON_NOICON = 0,
    ICON_ANNOTATE,
    ICON_AXES,
    ICON_CONNECTION,
    ICON_CONTOURS,
    ICON_CUT,
    ICON_DOC_AIF,
    ICON_DOC_ASF,
    ICON_DOC_AVI,
    ICON_DOC_BMP,
    ICON_DOC_CHM,
    ICON_DOC_EXE,
    ICON_DOC_GIF,
    ICON_DOC_HLP,
    ICON_DOC_HTTP,
    ICON_DOC_JPG,
    ICON_DOC_MP3,
    ICON_DOC_MPEG,
    ICON_DOC_MSI,
    ICON_DOC_PDF,
    ICON_DOC_PNG,
    ICON_DOC_POSTSCRIPT,
    ICON_DOC_POWERPOINT,
    ICON_DOC_QUICKTIME,
    ICON_DOC_REALAUDIO,
    ICON_DOC_TGA,
    ICON_DOC_TIF,
    ICON_DOC_TXT,
    ICON_DOC_WAV,
    ICON_DOC_WORD,
    ICON_DOC_ZIP,
    ICON_ERROR,
    ICON_EXPAND,
    ICON_FILTERS,
    ICON_FOLDER,
    ICON_FOLDER_OPEN,
    ICON_GENERAL,
    ICON_GRID_LINEAR,
    ICON_GRID_LOG,
    ICON_HELPBUBBLE,
    ICON_INFO_MINI,
    ICON_LAYOUT,
    ICON_LOCK,
    ICON_MACROS,
    ICON_MAG_GLASS,
    ICON_MATERIAL,
    ICON_MINUS,
    ICON_MOVE,
    ICON_MOVE_H,
    ICON_MOVE_V,
    ICON_PLUS,
    ICON_PREFERENCES,
    ICON_QUESTION,
    ICON_RELOAD,
    ICON_SHRINK,
    ICON_SMALLERROR,
    ICON_SMALLERRORRED,
    ICON_STOPWATCH,
    ICON_TRANSFER,
    ICON_TRANSPORT_BEGINNING,
    ICON_TRANSPORT_END,
    ICON_TRANSPORT_FAST_FORWARD,
    ICON_TRANSPORT_FAST_FORWARD_TO_KEY,
    ICON_TRANSPORT_LOOP,
    ICON_TRANSPORT_PAUSE,
    ICON_TRANSPORT_PLAY,
    ICON_TRANSPORT_PLAY_TO_KEY,
    ICON_TRANSPORT_REWIND,
    ICON_TRANSPORT_REWIND_TO_KEY,
    ICON_TRANSPORT_STOP,
    ICON_TRASHCAN,
    ICON_TREE_CLOSE,
    ICON_TREE_OPEN,
    ICON_WARNING,
    ICON_WARNING_MINI,
    ICON_WINDOW_LEVEL,
    LAST_ICON
  };
  //ETX

  // Description:
  // Select an icon based on the icon name.
  void SetImage(int image);

  // Description:
  // Set image data from vtkImageData. 
  // Pixel data is converted/padded to RGBA for backward compatibility.
  void SetImage(vtkImageData*);

  // Description:
  // Set image data from another vtkKWIcon.
  void SetImage(vtkKWIcon*);

  // Description:
  // Set image data from pixel data, eventually zlib and base64.
  void SetImage(const unsigned char* data, 
                int width, int height, int pixel_size, 
                unsigned long buffer_length);

  // Description:
  // Get the raw image data.
  const unsigned char* GetData();

  // Description:
  // Get the width of the image.
  vtkGetMacro(Width, int);

  // Description:
  // Get the height of the image.
  vtkGetMacro(Height, int);
  
  // Description:
  // Get the pixel size of the image.
  vtkGetMacro(PixelSize, int);

protected:
  vtkKWIcon();
  ~vtkKWIcon();

  // Description:
  // Set icon to the custom data.
  void SetData(const unsigned char* data, 
               int width, int height, int pixel_size);

  unsigned char* Data;
  int Width;
  int Height;
  int PixelSize;

  // Description:
  // Set data to the internal image.
  void SetInternalData(const unsigned char* data, 
                       int width, int height, int pixel_size);
  
  const unsigned char* InternalData;
  int Internal;

private:
  vtkKWIcon(const vtkKWIcon&); // Not implemented
  void operator=(const vtkKWIcon&); // Not implemented
};

#endif



