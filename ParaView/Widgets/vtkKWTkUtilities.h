/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWTkUtilities.h
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
// .NAME vtkKWTkUtilities - class that supports basic Tk functions
// .SECTION Description
// vtkKWTkUtilities provides methods to perform common Tk operations.

#ifndef __vtkKWTkUtilities_h
#define __vtkKWTkUtilities_h

#include "vtkObject.h"

class vtkImageData;
struct Tcl_Interp;

class VTK_EXPORT vtkKWTkUtilities : public vtkObject
{
public:
  static vtkKWTkUtilities* New();
  vtkTypeRevisionMacro(vtkKWTkUtilities,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Get RGB component for color (given a window)
  //BTX  
  static void GetRGBColor(Tcl_Interp *interp,
                          const char *window, 
                          const char *color, 
                          int *rr, int *gg, int *bb);
  //ETX

  // Get background color of window/widget
  //BTX  
  static void GetBackgroundColor(Tcl_Interp *interp,
                                 const char *window, 
                                 int *r, int *g, int *b);
  //ETX
  
  // Update a photo given a pixel structure. 
  // If RGBA (pixel_size > 3), blend pixels with background color of
  // the blend_with_name widget (otherwise 0.5, 0.5, 0.5 gray)
  //BTX  
  static int UpdatePhoto(Tcl_Interp *interp,
                         const char *photo_name,
                         const unsigned char *pixels, 
                         int width, int height,
                         int pixel_size,
                         unsigned long buffer_length = 0,
                         const char *blend_with_name = 0);

  static int UpdatePhoto(Tcl_Interp *interp,
                         const char *photo_name,
                         vtkImageData *image, 
                         const char *blend_with_name = 0);

  static int GetPhotoHeight(Tcl_Interp *interp,
                            const char *photo_name);

  static int GetPhotoWidth(Tcl_Interp *interp,
                           const char *photo_name);

  //ETX

protected:
  vtkKWTkUtilities() {};
  ~vtkKWTkUtilities() {};

private:
  vtkKWTkUtilities(const vtkKWTkUtilities&); // Not implemented
  void operator=(const vtkKWTkUtilities&); // Not implemented
};

#endif
