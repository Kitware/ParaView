/*=========================================================================

  Program:   ParaView
  Module:    vtkPVArrowSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVArrowSource - A quick fix for a bug.
// .SECTION Description
// This class does nothing but tell the pipeline it can generate pieces.
// This is a quick fix for a bug with glyphs and multidisplay with
// the zero empty option.  Zero does not update arrow when created.
// Glyph updates zero but not others.  Transmit poly data hangs.
// The correct solution is to not connect the pipeline
// on this seudo client.  I can get rid of this class then.



#ifndef __vtkPVArrowSource_h
#define __vtkPVArrowSource_h

#include "vtkArrowSource.h"

class VTK_EXPORT vtkPVArrowSource : public vtkArrowSource
{
public:
  // Description
  // Construct cone with angle of 45 degrees.
  static vtkPVArrowSource *New();

  vtkTypeMacro(vtkPVArrowSource,vtkArrowSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
protected:
  vtkPVArrowSource() {};
  ~vtkPVArrowSource() {};

  void ExecuteInformation();

private:
  vtkPVArrowSource(const vtkPVArrowSource&); // Not implemented.
  void operator=(const vtkPVArrowSource&); // Not implemented.
};

#endif


