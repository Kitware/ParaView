/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVVisibleCellSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVVisibleCellSelector - Color buffer selections for ParaView.
//
// .SECTION Description
// This exposes some of the internals in vtkVisibleCellSelector so that 
// ParaView's vtkSMRenderModuleProxy can drive the color buffer selection.
// It also provides post selection mapping of selection ids to 
// vtkClientServerIds for selected actors.

#ifndef __vtkPVVisibleCellSelector_h
#define __vtkPVVisibleCellSelector_h

#include "vtkVisibleCellSelector.h"

class VTK_EXPORT vtkPVVisibleCellSelector : public vtkVisibleCellSelector
{
public:
  static vtkPVVisibleCellSelector *New();
  vtkTypeRevisionMacro(vtkPVVisibleCellSelector,vtkVisibleCellSelector);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Give this a selected region of the render window after a selection render
  // with one of the passes defined above.
  void SavePixelBuffer(int pass, unsigned char *src);

  // Description:
  // After one or more calls to SavePixelBuffer(), this will convert the saved
  // pixel buffers into a list of Ids.
  void ComputeSelectedIds();


  // Description:
  // This uses the ProcessModule to look up the rank of the executing node.
  void SetProcessorId();

  //BTX
  enum {NOT_SELECTING = 0, COLOR_BY_PROCESSOR, COLOR_BY_ACTOR, 
        COLOR_BY_CELL_ID_HIGH, COLOR_BY_CELL_ID_MID, COLOR_BY_CELL_ID_LOW};  
  //ETX
  void SetSelectMode(int mode);


protected:
  vtkPVVisibleCellSelector() {};
  ~vtkPVVisibleCellSelector() {};

  // Description:
  // We override this to map from selection ids to client server ids.
  virtual vtkIdType MapActorIdToActorId(vtkIdType id);

private:
  vtkPVVisibleCellSelector(const vtkPVVisibleCellSelector&);  // Not implemented.
  void operator=(const vtkPVVisibleCellSelector&);  // Not implemented.
};

#endif
