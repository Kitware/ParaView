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

class vtkKWIcon;

class KWWidgets_EXPORT vtkKWIcon : public vtkObject
{
public:
  static vtkKWIcon* New();
  vtkTypeRevisionMacro(vtkKWIcon,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  void DeepCopy(vtkKWIcon *p);

  //BTX
  // Description:
  // There are several predefined icons in the Resources/KWIcons.h. Since we
  // want to save space, we only include that file to vtkKWIcons.cxx.
  // These constants specify different icons.
  enum { 
    IconNoIcon                    = 0,
    IconAngleTool                 = 14,
    IconBiDimensionalTool         = 34,
    IconBoundingBox               = 13,
    IconCamera                    = 19,
    IconColorBarAnnotation        = 12,
    IconColorSquares              = 18,
    IconConnection                = 1,
    IconContourTool               = 16,
    IconContourSegment            = 33,
    IconCornerAnnotation          = 11,
    IconCropTool                  = 22,
    IconDistanceTool              = 15,
    IconDocument                  = 9,
    IconEmpty16x16                = 6,
    IconEmpty1x1                  = 10,
    IconError                     = 2, 
    IconErrorMini                 = 3,
    IconErrorRedMini              = 4,
    IconExpand                    = 5,
    IconExpandMini                = 23,
    IconEye                       = 17,
    IconFileOpen                  = 8,
    IconFloppy                    = 7,
    IconFolder                    = 20,
    IconFolderOpen                = 21,
    IconGridLinear                = 40,
    IconGridLog                   = 41,
    IconHeaderAnnotation          = 50,
    IconHelpBubble                = 60,
    IconInfoMini                  = 61,
    IconLock                      = 62,
    IconMagGlass                  = 63,
    IconMinus                     = 64,
    IconMove                      = 80,
    IconMoveH                     = 81,
    IconMoveV                     = 82,
    IconOrientationCubeAnnotation = 83,
    IconPanHand                   = 84,
    IconPlus                      = 100,
    IconPointFinger               = 90,
    IconPresetAdd                 = 94,
    IconPresetApply               = 95,
    IconPresetDelete              = 96,
    IconPresetEmail               = 97,
    IconPresetLocate              = 98,
    IconPresetUpdate              = 99,
    IconQuestion                  = 101,
    IconReload                    = 102,
    IconRotate                    = 105,
    IconScaleBarAnnotation        = 106,
    IconSideAnnotation            = 110,
    IconSpinDown                  = 111,
    IconSpinLeft                  = 112,
    IconSpinRight                 = 113,
    IconSpinUp                    = 114,
    IconShrink                    = 103,
    IconStopwatch                 = 104,
    IconTransportBeginning        = 120,
    IconTransportEnd              = 121,
    IconTransportFastForward      = 122,
    IconTransportFastForwardToKey = 123,
    IconTransportLoop             = 124,
    IconTransportPause            = 125,
    IconTransportPlay             = 126,
    IconTransportPlayToKey        = 127,
    IconTransportRewind           = 128,
    IconTransportRewindToKey      = 129,
    IconTransportStop             = 130,
    IconTrashcan                  = 131,
    IconTreeClose                 = 150,
    IconTreeOpen                  = 151,
    IconWarning                   = 170,
    IconWarningMini               = 171,
    IconWindowLevel               = 172,
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

  // Description:
  // Fade the icon by a factor.
  // If the icon has an alpha channel, this multiplies each element in
  // the alpha channel by 'factor'.
  virtual void Fade(double factor);

  // Description:
  // Flatten the image against a color.
  // If the icon has an alpha channel, this blends the image against
  // a 'r', 'g', 'b' background and discards the alpha channel.
  virtual void Flatten(double r, double g, double b);
  virtual void Flatten(double rgb[3])
    { this->Flatten(rgb[0], rgb[1], rgb[2]); };

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



