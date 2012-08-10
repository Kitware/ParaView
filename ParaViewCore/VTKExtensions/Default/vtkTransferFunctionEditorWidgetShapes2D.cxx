/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorWidgetShapes2D.cxx,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionEditorWidgetShapes2D.h"

#include "vtkObjectFactory.h"
#include "vtkTransferFunctionEditorRepresentationShapes2D.h"

vtkStandardNewMacro(vtkTransferFunctionEditorWidgetShapes2D);

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetShapes2D::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
    {
    this->WidgetRep = vtkTransferFunctionEditorRepresentationShapes2D::New();
    this->Superclass::CreateDefaultRepresentation();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetShapes2D::PrintSelf(ostream& os,
                                                        vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
