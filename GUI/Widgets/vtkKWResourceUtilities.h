/*=========================================================================

  Module:    vtkKWResourceUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWResourceUtilities - class that supports resource functions
// .SECTION Description
// vtkKWResourceUtilities provides methods to perform common resources
//  operations.

#ifndef __vtkKWResourceUtilities_h
#define __vtkKWResourceUtilities_h

#include "vtkObject.h"

class vtkKWWidget;
class vtkKWApplication;

class VTK_EXPORT vtkKWResourceUtilities : public vtkObject
{
public:
  static vtkKWResourceUtilities* New();
  vtkTypeRevisionMacro(vtkKWResourceUtilities,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Read a PNG file given its 'filename'.
  // On success, modifies 'width', 'height', 'pixel_size' and 'pixels' 
  // accordingly. Note that 'pixels' is allocated automatically using the
  // 'new' operator (i.e. it is up to the caller to free the memory using
  // the 'delete' operator).
  // Note that grayscale images are promoted to RGB. Transparent images are
  // promoted to RGBA. The bit depth is promoted (or shrunk) to 8 bits
  // per component. 'pixel_size' is therefore always 3 (RGB) or 4 (RGBA).
  // Return 1 on success, 0 otherwise.
  static int ReadPNGImage(const char *filename,
                          int *width, int *height, 
                          int *pixel_size,
                          unsigned char **pixels);

protected:
  vtkKWResourceUtilities() {};
  ~vtkKWResourceUtilities() {};

private:
  vtkKWResourceUtilities(const vtkKWResourceUtilities&); // Not implemented
  void operator=(const vtkKWResourceUtilities&); // Not implemented
};

#endif

