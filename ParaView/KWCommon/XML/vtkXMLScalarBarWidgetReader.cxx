/*=========================================================================

  Module:    vtkXMLScalarBarWidgetReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLScalarBarWidgetReader.h"

#include "vtkObjectFactory.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarWidget.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLScalarBarActorReader.h"

vtkStandardNewMacro(vtkXMLScalarBarWidgetReader);
vtkCxxRevisionMacro(vtkXMLScalarBarWidgetReader, "1.4");

//----------------------------------------------------------------------------
char* vtkXMLScalarBarWidgetReader::GetRootElementName()
{
  return "ScalarBarWidget";
}

//----------------------------------------------------------------------------
int vtkXMLScalarBarWidgetReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkScalarBarWidget *obj = vtkScalarBarWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ScalarBarWidget is not set!");
    return 0;
    }

  // Get nested elements
  
  // Scalar bar actor

  vtkXMLScalarBarActorReader *xmlr = vtkXMLScalarBarActorReader::New();
  if (xmlr->IsInElement(elem))
    {
    vtkScalarBarActor *scalarbara = obj->GetScalarBarActor();
    if (!scalarbara)
      {
      scalarbara = vtkScalarBarActor::New();
      obj->SetScalarBarActor(scalarbara);
      scalarbara->Delete();
      }
    xmlr->SetObject(scalarbara);
    xmlr->ParseInElement(elem);
    }
  xmlr->Delete();

  return 1;
}


