/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveOutlineRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAdaptiveOutlineRepresentation - A representation to show piece
// bounds for streaming.
// .SECTION Description

#ifndef __vtkSMAdaptiveOutlineRepresentation_h
#define __vtkSMAdaptiveOutlineRepresentation_h

#include "vtkSMOutlineRepresentationProxy.h"

class vtkSMViewProxy;

class VTK_EXPORT vtkSMAdaptiveOutlineRepresentation : 
  public vtkSMOutlineRepresentationProxy
{
public:
  static vtkSMAdaptiveOutlineRepresentation* New();
  vtkTypeMacro(vtkSMAdaptiveOutlineRepresentation,
    vtkSMOutlineRepresentationProxy);

  virtual bool AddToView(vtkSMViewProxy *view);

  virtual void SetNextPiece(int P, int NP, double R, double pri, bool hit, bool append);

  virtual void Update(vtkSMViewProxy* view);

  virtual bool RemoveMyselfFromView(vtkSMViewProxy* view); 

  virtual bool EmptyCache(); 

protected:
  vtkSMAdaptiveOutlineRepresentation();
  ~vtkSMAdaptiveOutlineRepresentation();

  virtual bool EndCreateVTKObjects();

  int Piece;
  int Pieces;
  double Resolution;
private:
  vtkSMAdaptiveOutlineRepresentation(const vtkSMAdaptiveOutlineRepresentation&); // Not implemented
  void operator=(const vtkSMAdaptiveOutlineRepresentation&); // Not implemented
//ETX
};

#endif

