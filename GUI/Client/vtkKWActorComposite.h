/*=========================================================================

  Module:    vtkKWActorComposite.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWActorComposite - a composite for actors
// .SECTION Description
// A composite designed for actors. The actor has a vtkPolyDataMapper as
// a mapper, and the user specifies vtkPolyData as the input of this 
// composite.

#ifndef __vtkKWActorComposite_h
#define __vtkKWActorComposite_h

#include "vtkKWComposite.h"

class vtkKWApplication;
class vtkPolyData;
class vtkActor;
class vtkPolyData;
class vtkPolyDataMapper;

class VTK_EXPORT vtkKWActorComposite : public vtkKWComposite
{
public:
  static vtkKWActorComposite* New();
  vtkTypeRevisionMacro(vtkKWActorComposite,vtkKWComposite);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the properties object, called by UpdateProperties.
  virtual void CreateProperties();

  //BTX
  // Description:
  // Set the input data for this Actor
  virtual void SetInput(vtkPolyData *input);
  vtkPolyData *GetInput();

  // Description:
  // Get the prop for this composite
  virtual vtkActor *GetActor() {return this->Actor;};
  
  // Description:
  // Get the mapper for the composite
  vtkGetObjectMacro( Mapper, vtkPolyDataMapper );
  //ETX
  
protected:
  vtkKWActorComposite();
  ~vtkKWActorComposite();

  vtkActor *Actor;
  vtkPolyDataMapper *Mapper;

  // Define method required by superclass.
  virtual vtkProp* GetPropInternal();

private:
  vtkKWActorComposite(const vtkKWActorComposite&);  // Not implemented.
  void operator=(const vtkKWActorComposite&);  // Not implemented.
};


#endif


