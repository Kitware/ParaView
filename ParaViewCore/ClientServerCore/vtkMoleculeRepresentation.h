/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef _vtkMoleculeRepresentation_h
#define _vtkMoleculeRepresentation_h

#include "vtkPVDataRepresentation.h"

class vtkActor;
class vtkMoleculeMapper;

class VTK_EXPORT vtkMoleculeRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkMoleculeRepresentation* New();
  vtkTypeMacro(vtkMoleculeRepresentation, vtkPVDataRepresentation)
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual int ProcessViewRequest(vtkInformationRequestKey *requestType,
                                 vtkInformation *inputInfo,
                                 vtkInformation *outputInfo);

  virtual void SetVisibility(bool value);

  vtkGetMacro(MoleculeRenderMode, int)
  void SetMoleculeRenderMode(int mode);

//BTX
protected:
  vtkMoleculeRepresentation();
  ~vtkMoleculeRepresentation();

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual bool AddToView(vtkView *view);
  virtual bool RemoveFromView(vtkView *view);

  vtkActor *Actor;
  vtkMoleculeMapper *Mapper;

  int MoleculeRenderMode;

private:
  vtkMoleculeRepresentation(const vtkMoleculeRepresentation&); // Not implemented
  void operator=(const vtkMoleculeRepresentation&); // Not implemented
//ETX
};

#endif // _vtkMoleculeRepresentation_h
