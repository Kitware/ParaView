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
class vtkPoints;
class VTK_EXPORT vtkSMUtilities : public vtkSMObject
{
public:
  static vtkSMUtilities* New();
  vtkTypeMacro(vtkSMUtilities, vtkSMObject);
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

  // Description:
  // Save the image to a file using a vtkImageWriter subclass given by writerName.
  // The file is created on the process on which this method is called.
  static int SaveImage(vtkImageData* image, const char* filename,
                                          const char* writerName);

  // Description:
  // Calls SaveImage(image, filename, writerName) only on process 0.
  // Other processes will recieve the return code through a broadcast.
  static int SaveImageOnProcessZero(vtkImageData* image,
                const char* filename, const char* writerName);

  // Description:
  // Returns the points an orbit to revolve around the \c center at a distance
  // of \c radius in the plane defined by the \c center and the \c normal. The
  // number of points returned is equal to \c resolution.
  // Returns a new instance of vtkPoints. The caller is responsible for freeing
  // the allocated memory.
  static vtkPoints* CreateOrbit(const double center[3], const double normal[3],
                                int resolution, const double startPoint[3]);

  // Will pick an arbitrary starting point
  static vtkPoints* CreateOrbit(const double center[3], const double normal[3],
                                double radius, int resolution);

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

