/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWIcon.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkKWIcon - simple wrapper for icons
// .SECTION Description
// A simple icon wrapper. It can either be used with file icons.h to 
// provide a unified interface for internal icons or a wrapper for 
// custom icons. The icons are defined with width, height, and array
// of unsigned char values; four values per pixel (RGBA).

#ifndef __vtkKWIcon_h
#define __vtkKWIcon_h

#include "vtkObject.h"
class vtkKWApplication;
class vtkKWIcon;

class VTK_EXPORT vtkKWIcon : public vtkObject
{
public:
  static vtkKWIcon* New();
  vtkTypeMacro(vtkKWIcon,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set image data.
  void SetImageData(const unsigned char* data, int width, int height);

//BTX
  // Description:
  // There are several predefined icons in the icons.h. Since we
  // want to save space, we only incldue that file to vtkKWIcons.cxx.
  // These constnats specify different icons.
  enum { 
    ICON_NOICON = 0,
    ICON_ANNOTATE,
    ICON_CONTOURS,
    ICON_CUT,
    ICON_ERROR,
    ICON_FILTERS,
    ICON_GENERAL,
    ICON_LAYOUT,
    ICON_MACROS,
    ICON_MATERIAL,
    ICON_PREFERENCES,
    ICON_QUESTION,
    ICON_TRANSFER,
    ICON_WARNING,
    ICON_SHRINK,
    ICON_EXPAND,
    ICON_SMALLERROR,
    ICON_SMALLERRORRED,
    LAST_ICON
  };
//ETX

  // Description:
  // Select an icon based on the icon name.
  void SetImageData(int image);

  // Description:
  // Set icon to the custom data.
  void SetData(const unsigned char* data, int width, int height);

  // Description:
  // Get the raw image data (RGBA).
  const unsigned char* GetData();

  // Description:
  // Get the width of the image.
  vtkGetMacro(Width, int);

  // Description:
  // Get the height of the image.
  vtkGetMacro(Height, int);
  
protected:
  vtkKWIcon();
  ~vtkKWIcon();
  unsigned char* Data;
  int Width;
  int Height;

  // Description:
  // Set data to the internal image.
  void SetInternalData(const unsigned char* data, int width, int height);
  
  const unsigned char* InternalData;
  int Internal;
private:
  vtkKWIcon(const vtkKWIcon&); // Not implemented
  void operator=(const vtkKWIcon&); // Not implemented
};


#endif


