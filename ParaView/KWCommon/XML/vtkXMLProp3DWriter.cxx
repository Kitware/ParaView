/*=========================================================================

  Module:    vtkXMLProp3DWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLProp3DWriter.h"

#include "vtkProp3D.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLProp3DWriter);
vtkCxxRevisionMacro(vtkXMLProp3DWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLProp3DWriter::GetRootElementName()
{
  return "Prop3D";
}

//----------------------------------------------------------------------------
int vtkXMLProp3DWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkProp3D *obj = vtkProp3D::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Prop3D is not set!");
    return 0;
    }

  elem->SetVectorAttribute("Position", 3, obj->GetPosition());

  elem->SetVectorAttribute("Origin", 3, obj->GetOrigin());

  elem->SetVectorAttribute("Scale", 3, obj->GetScale());

  elem->SetVectorAttribute("Orientation", 3, obj->GetOrientation());

  return 1;
}


