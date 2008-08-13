/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUtilities - collection of utility methods.
// .SECTION Description
// vtkSMUtilities defines a collection of useful static methods.

#ifndef __vtkSMUtilities_h
#define __vtkSMUtilities_h

#include "vtkSMObject.h"

class vtkImageData;
class VTK_EXPORT vtkSMUtilities : public vtkSMObject
{
public:
  static vtkSMUtilities* New();
  vtkTypeRevisionMacro(vtkSMUtilities, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Save the image to a file.
  // The file is created on the process on which this method is called.
  // Return vtkErrorCode::NoError (0) on success, otherwise returns the error
  // code.
  /// quality [0,100] -- 0 = low, 100=high, -1=default
  static int SaveImage(vtkImageData* image, const char* filename, 
    int quality);
  static int SaveImage(vtkImageData* image, const char* filename)
    { return vtkSMUtilities::SaveImage(image, filename, -1); }

//BTX
protected:
  vtkSMUtilities() {}
  ~vtkSMUtilities(){}

private:
  vtkSMUtilities(const vtkSMUtilities&); // Not implemented
  void operator=(const vtkSMUtilities&); // Not implemented
//ETX
};

#endif

