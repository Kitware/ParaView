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

  //BTX  

  // Description:
  // Get RGB component for color (given a window).
  static void GetRGBColor(Tcl_Interp *interp,
                          const char *window, 
                          const char *color, 
                          int *rr, int *gg, int *bb);

  // Description:
  // Get background color of window/widget.
  static void GetBackgroundColor(Tcl_Interp *interp,
                                 const char *window, 
                                 int *r, int *g, int *b);
  
  // Description:
  // Update a photo given a pixel structure. 
  // If RGBA (pixel_size > 3), blend pixels with background color of
  // the blend_with_name widget (otherwise 0.5, 0.5, 0.5 gray)
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

  // Description:
  // Quick way to get a photo height/width.
  static int GetPhotoHeight(Tcl_Interp *interp,
                            const char *photo_name);
  static int GetPhotoWidth(Tcl_Interp *interp,
                           const char *photo_name);

  // Description:
  // Boldify the -font attribute of widget.
  static int ChangeFontToBold(Tcl_Interp *interp,
                              const char *widget);

  // Description:
  // Get the size of a grid (i.e. the number of colums and rows in this 
  // master widget).
  static int GetGridSize(Tcl_Interp *interp,
                         const char *widget,
                         int *nb_of_cols,
                         int *nb_of_rows);

  // Description:
  // Get the column widths of a grid (i.e. a master widget that has been grid).
  // If 'allocate' is true, the resulting array (col_widths) is allocated
  // by the function to match the number of columns.
  // The function iterates over cells to request the width of
  // each slave (winfo reqwidth).
  static int GetGridColumnWidths(Tcl_Interp *interp,
                                 const char *widget,
                                 int *nb_of_cols,
                                 int **col_widths,
                                 int allocate = 0);
  // Description:
  // Synchronize the columns minimum size of different widgets that have
  // been grid. If 'factors' is non-null, it is used as an array of
  // multiplication factor to apply to each column minimum size.
  // If 'weights' is non-null, it is used as an array of weight
  // to apply to each column through columnconfigure -weight.
  static int SynchroniseGridsColumnMinimumSize(Tcl_Interp *interp,
                                               int nb_of_widgets,
                                               const char **widgets,
                                               const float *factors = 0,
                                               const int *weights = 0);

  //ETX

protected:
  vtkKWTkUtilities() {};
  ~vtkKWTkUtilities() {};

private:
  vtkKWTkUtilities(const vtkKWTkUtilities&); // Not implemented
  void operator=(const vtkKWTkUtilities&); // Not implemented
};

#endif
