/*=========================================================================

  Module:    vtkXMLKWWindowReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLKWWindowReader.h"

#include "vtkKWNotebook.h"
#include "vtkKWUserInterfaceNotebookManager.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLKWUserInterfaceNotebookManagerReader.h"
#include "vtkXMLKWWindowWriter.h"

vtkStandardNewMacro(vtkXMLKWWindowReader);
vtkCxxRevisionMacro(vtkXMLKWWindowReader, "1.4");

//----------------------------------------------------------------------------
char* vtkXMLKWWindowReader::GetRootElementName()
{
  return "KWWindow";
}

//----------------------------------------------------------------------------
int vtkXMLKWWindowReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkKWWindow *obj = vtkKWWindow::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The KWWindow is not set!");
    return 0;
    }

  // User Interface

  vtkXMLDataElement *ui_elem = elem->FindNestedElementWithName(
    vtkXMLKWWindowWriter::GetUserInterfaceElementName());
  if (ui_elem)
    {
    this->ParseUserInterfaceElement(ui_elem);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLKWWindowReader::ParseUserInterfaceElement(
  vtkXMLDataElement *ui_elem)
{
  if (!ui_elem)
    {
    return 0;
    }

  vtkKWWindow *obj = vtkKWWindow::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The KWWindow is not set!");
    return 0;
    }

  // Get attributes

  int ival;

  if (ui_elem->GetScalarAttribute("PropertiesVisibility", ival))
    {
    obj->SetPropertiesVisiblity(ival);
    }

  vtkXMLDataElement *nested_elem;

  // User Interface Manager

  vtkKWUserInterfaceNotebookManager *uim_nb = 
    vtkKWUserInterfaceNotebookManager::SafeDownCast(
      obj->GetUserInterfaceManager());
  if (uim_nb)
    {
    nested_elem = ui_elem->FindNestedElementWithName(
      vtkXMLKWWindowWriter::GetUserInterfaceManagerElementName());
    if (nested_elem)
      {
      vtkXMLKWUserInterfaceNotebookManagerReader *xmlr = 
        vtkXMLKWUserInterfaceNotebookManagerReader::New();
      xmlr->SetObject(uim_nb);
      xmlr->ParseInElement(nested_elem);
      xmlr->Delete();
      }
    }

  return 1;
}

