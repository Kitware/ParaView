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
// A simple icon wrapper. It can either be used with file KWIcons.h to 
// provide a unified interface for internal icons or a wrapper for 
// custom icons. The icons are defined with width, height, pixel_size, 
// and array of unsigned char values.

#ifndef __vtkKWIcon_h
#define __vtkKWIcon_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives

class vtkKWApplication;
class vtkKWIcon;

class KWWIDGETS_EXPORT vtkKWIcon : public vtkObject
{
public:
  static vtkKWIcon* New();
  vtkTypeRevisionMacro(vtkKWIcon,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // There are several predefined icons in the Resources/KWIcons.h. Since we
  // want to save space, we only include that file to vtkKWIcons.cxx.
  // These constants specify different icons.
  enum { 
    IconNoIcon = 0,
    IconConnection,
    IconError,
    IconExpand,
    IconFolder,
    IconFolderOpen,
    IconGridLinear,
    IconGridLog,
    IconHelpBubble,
    IconInfoMini,
    IconLock,
    IconMagGlass,
    IconMinus,
    IconMove,
    IconMoveH,
    IconMoveV,
    IconPlus,
    IconQuestion,
    IconReload,
    IconShrink,
    IconSmallError,
    IconSmallErrorRed,
    IconStopwatch,
    IconTransportBeginning,
    IconTransportEnd,
    IconTransportFastForward,
    IconTransportFastForwardToKey,
    IconTransportLoop,
    IconTransportPause,
    IconTransportPlay,
    IconTransportPlayToKey,
    IconTransportRewind,
    IconTransportRewindToKey,
    IconTransportStop,
    IconTrashcan,
    IconTreeClose,
    IconTreeOpen,
    IconWarning,
    IconWarningMini,
    IconWindowLevel,
    LastIcon
  };
  //ETX

  // Description:
  // Select an icon based on the icon name.
  void SetImage(int image);

  // Description:
  // Set image data from another vtkKWIcon.
  void SetImage(vtkKWIcon*);

  // Description:
  // Set image data from pixel data, eventually zlib and base64.
  // If 'buffer_length' is 0, compute it automatically by multiplying
  // 'pixel_size', 'width' and 'height' together.
  // If ImageOptionFlipVertical is set in 'option', flip the image vertically
  //BTX
  enum 
  { 
    ImageOptionFlipVertical = 1
  };
  //ETX
  void SetImage(const unsigned char* data, 
                int width, int height, 
                int pixel_size, 
                unsigned long buffer_length = 0,
                int options = 0);

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
               int width, int height, 
               int pixel_size,
               int options = 0);

  unsigned char* Data;
  int Width;
  int Height;
  int PixelSize;

private:
  vtkKWIcon(const vtkKWIcon&); // Not implemented
  void operator=(const vtkKWIcon&); // Not implemented
};

#endif



