/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractCTHPart.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractCTHPart - Generates surface of an CTH volume fraction.
// .SECTION Description
// vtkExtractCTHPart is a filter that is specialized for creating 
// visualization of a CTH simulation.  First it converts the cell data
// to point data.  It contours the selected volume fraction at a value
// of 0.5.  The user has the option of clipping the part with a plane.
// Clipped surfaces of the part are generated.

#ifndef __vtkExtractCTHPart_h
#define __vtkExtractCTHPart_h

#include "vtkRectilinearGridToPolyDataFilter.h"
class vtkPlane;
class vtkDataArray;
class vtkFloatArray;

class VTK_EXPORT vtkExtractCTHPart : public vtkRectilinearGridToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkExtractCTHPart,vtkRectilinearGridToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkExtractCTHPart *New();

  // Description:
  // Specify which volume fraction to extract.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}

  // Description:
  // Turn clipping on or off.  It is off by default.
  vtkSetMacro(Clipping,int);
  vtkGetMacro(Clipping,int);
  vtkBooleanMacro(Clipping,int);
  
  // Description:
  // Set, get or maninpulate the implicit clipping plane.
  void SetClipPlane(vtkPlane *clipPlane);
  vtkGetObjectMacro(ClipPlane, vtkPlane);

  // Description:
  // Look at clip plane to compute MTime.
  unsigned long GetMTime();

protected:
  vtkExtractCTHPart();
  ~vtkExtractCTHPart();

  void ComputeInputUpdateExtents(vtkDataObject *output);
  void Execute();
  void ExecuteCellDataToPointData(vtkDataArray *cellVolumeFraction, 
                       vtkFloatArray *pointVolumeFraction, int *dims);

  char *InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

  int Clipping;
  vtkPlane *ClipPlane;

private:
  vtkExtractCTHPart(const vtkExtractCTHPart&);  // Not implemented.
  void operator=(const vtkExtractCTHPart&);  // Not implemented.
};

#endif

