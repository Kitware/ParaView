/*=========================================================================

  Module:    vtkXMLProp3DReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLProp3DReader.h"

#include "vtkProp3D.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLProp3DReader);
vtkCxxRevisionMacro(vtkXMLProp3DReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLProp3DReader::GetRootElementName()
{
  return "Prop3D";
}

//----------------------------------------------------------------------------
int vtkXMLProp3DReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkProp3D *obj = vtkProp3D::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Prop3D is not set!");
    return 0;
    }

  // Get attributes

  float fbuffer3[3];

  if (elem->GetVectorAttribute("Position", 3, fbuffer3) == 3)
    {
    obj->SetPosition(fbuffer3);
    }
  
  if (elem->GetVectorAttribute("Origin", 3, fbuffer3) == 3)
    {
    obj->SetOrigin(fbuffer3);
    }

  if (elem->GetVectorAttribute("Scale", 3, fbuffer3) == 3)
    {
    obj->SetScale(fbuffer3);
    }

  if (elem->GetVectorAttribute("Orientation", 3, fbuffer3) == 3)
    {
    obj->SetOrientation(fbuffer3);
    }

  return 1;
}


