/*=========================================================================

  Module:    vtkKWActorComposite.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWActorComposite.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkKWRadioButton.h"
#include "vtkKWOptionMenu.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWActorComposite );
vtkCxxRevisionMacro(vtkKWActorComposite, "1.15");

//----------------------------------------------------------------------------
int vtkKWActorCompositeCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWActorComposite::vtkKWActorComposite()
{
  this->CommandFunction = vtkKWActorCompositeCommand;

  this->Actor = vtkActor::New();
  this->Mapper = vtkPolyDataMapper::New();
  this->Actor->SetMapper(this->Mapper);
}

//----------------------------------------------------------------------------
vtkKWActorComposite::~vtkKWActorComposite()
{
  if (this->Actor)
    {
    this->Actor->Delete();
    this->Actor = NULL;
    }
  
  if (this->Mapper)
    {
    this->Mapper->Delete();
    this->Mapper = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWActorComposite::SetInput(vtkPolyData *input)
{
  this->Mapper->SetInput(input); 
}

//----------------------------------------------------------------------------
void vtkKWActorComposite::CreateProperties()
{
  // invoke superclass always
  this->Superclass::CreateProperties();
}

//----------------------------------------------------------------------------
vtkPolyData *vtkKWActorComposite::GetInput() 
{
  return this->Mapper->GetInput();
}

//----------------------------------------------------------------------------
vtkProp* vtkKWActorComposite::GetPropInternal()
{
  return this->Actor;
}

//----------------------------------------------------------------------------
void vtkKWActorComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Mapper: " << this->GetMapper() << endl;
}


