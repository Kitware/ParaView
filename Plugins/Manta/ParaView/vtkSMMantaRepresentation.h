/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMantaRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMMantaRepresentation - Representation that exposes manta
// specific parameters of the object's vtkMantaProperty
// .SECTION Description

#ifndef __vtkSMMantaRepresentation_h
#define __vtkSMMantaRepresentation_h

#include "vtkSMPVRepresentationProxy.h"

class vtkSMViewProxy;
class vtkInformation;
class vtkSMMantaOutlineRepresentation;

class VTK_EXPORT vtkSMMantaRepresentation : 
  public vtkSMPVRepresentationProxy
{
public:
  static vtkSMMantaRepresentation* New();
  vtkTypeMacro(vtkSMMantaRepresentation,
    vtkSMPVRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  //See MantaWrapping.xml for descriptions of what these control
  void SetMaterialType(char *);
  vtkGetStringMacro(MaterialType);

  void SetReflectance(double );
  vtkGetMacro(Reflectance, double);
  void SetThickness(double );
  vtkGetMacro(Thickness, double);
  void SetEta(double);
  vtkGetMacro(Eta, double);
  void SetN(double);
  vtkGetMacro(N, double);
  void SetNt(double);
  vtkGetMacro(Nt, double);

protected:
  vtkSMMantaRepresentation();
  ~vtkSMMantaRepresentation();

  // Description:
  // Called at the start of CreateVTKObjects().
  virtual bool BeginCreateVTKObjects();

  char *MaterialType;
  double Reflectance;
  double Thickness;
  double Eta;
  double N;
  double Nt;

private:
  vtkSMMantaRepresentation(const vtkSMMantaRepresentation&); // Not implemented
  void operator=(const vtkSMMantaRepresentation&); // Not implemented

  void CallMethod(char *methodName, char *strArg, double doubleArg);
};

#endif

