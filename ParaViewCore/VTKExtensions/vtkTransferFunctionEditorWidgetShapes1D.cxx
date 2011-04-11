/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorWidgetShapes1D.cxx,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionEditorWidgetShapes1D.h"

#include "vtkObjectFactory.h"
#include "vtkTransferFunctionEditorRepresentationShapes1D.h"

vtkStandardNewMacro(vtkTransferFunctionEditorWidgetShapes1D);

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetShapes1D::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
    {
    this->WidgetRep = vtkTransferFunctionEditorRepresentationShapes1D::New();
    this->Superclass::CreateDefaultRepresentation();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetShapes1D::PrintSelf(ostream& os,
                                                        vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
