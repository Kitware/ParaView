/*=========================================================================

  Module:    vtkXMLScalarBarWidgetWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLScalarBarWidgetWriter.h"

#include "vtkObjectFactory.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarWidget.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLScalarBarActorWriter.h"

vtkStandardNewMacro(vtkXMLScalarBarWidgetWriter);
vtkCxxRevisionMacro(vtkXMLScalarBarWidgetWriter, "1.4");

//----------------------------------------------------------------------------
char* vtkXMLScalarBarWidgetWriter::GetRootElementName()
{
  return "ScalarBarWidget";
}

//----------------------------------------------------------------------------
int vtkXMLScalarBarWidgetWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkScalarBarWidget *obj = vtkScalarBarWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ScalarBarWidget is not set!");
    return 0;
    }

  // Scalar bar actor

  vtkScalarBarActor *scalarbara = obj->GetScalarBarActor();
  if (scalarbara)
    {
    vtkXMLScalarBarActorWriter *xmlw = vtkXMLScalarBarActorWriter::New();
    xmlw->SetObject(scalarbara);
    xmlw->CreateInElement(elem);
    xmlw->Delete();
    }
 
  return 1;
}


