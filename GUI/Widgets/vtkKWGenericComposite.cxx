/*=========================================================================

  Module:    vtkKWGenericComposite.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWGenericComposite.h"

#include "vtkObjectFactory.h"
#include "vtkProp.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWGenericComposite );
vtkCxxRevisionMacro(vtkKWGenericComposite, "1.10");

vtkSetObjectImplementationMacro(vtkKWGenericComposite, Prop, vtkProp);



int vtkKWGenericCompositeCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

vtkKWGenericComposite::vtkKWGenericComposite()
{
  this->CommandFunction = vtkKWGenericCompositeCommand;

  this->Prop = NULL;
}

vtkKWGenericComposite::~vtkKWGenericComposite()
{
  this->SetProp(NULL);
}

//----------------------------------------------------------------------------
void vtkKWGenericComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

