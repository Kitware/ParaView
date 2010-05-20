/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtentTranslator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVExtentTranslator - Uses alternative source for whole extent.
// .SECTION Description
// vtkPVExtentTranslator is like extent translator, but it uses an 
// alternative source as a whole extent. The whole extent passed is assumed 
// to be a subextent of the original source.  we simple take the intersection 
// of the split extent and the whole extdent passed in.  We are attempting to
// make branning piplines request consistent extents with the same piece 
// requests.  

// .SECTION Caveats
// This object is still under development.

#ifndef __vtkPVExtentTranslator_h
#define __vtkPVExtentTranslator_h

#include "vtkExtentTranslator.h"

class vtkAlgorithm;

class VTK_EXPORT vtkPVExtentTranslator : public vtkExtentTranslator
{
public:
  static vtkPVExtentTranslator *New();

  vtkTypeMacro(vtkPVExtentTranslator,vtkExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the original upstream data set
  virtual void SetOriginalSource(vtkAlgorithm*);
  vtkGetObjectMacro(OriginalSource,vtkAlgorithm);

  // Description:
  // Get/Set the port index for the OriginalSource from which to obtain
  // the extents.
  vtkSetMacro(PortIndex, int);
  vtkGetMacro(PortIndex, int);

  virtual int PieceToExtentThreadSafe(int piece, int numPieces, 
                                      int ghostLevel, int *wholeExtent, 
                                      int *resultExtent, int splitMode, 
                                      int byPoints);

protected:
  vtkPVExtentTranslator();
  ~vtkPVExtentTranslator();

  vtkAlgorithm* OriginalSource;
  int PortIndex;

  vtkPVExtentTranslator(const vtkPVExtentTranslator&); // Not implemented
  void operator=(const vtkPVExtentTranslator&); // Not implemented
};

#endif
