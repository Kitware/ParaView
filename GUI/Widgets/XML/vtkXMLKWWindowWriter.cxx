/*=========================================================================

  Module:    vtkXMLKWWindowWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLKWWindowWriter.h"

#include "vtkKWUserInterfaceNotebookManager.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLKWUserInterfaceNotebookManagerWriter.h"

vtkStandardNewMacro(vtkXMLKWWindowWriter);
vtkCxxRevisionMacro(vtkXMLKWWindowWriter, "1.6");

//----------------------------------------------------------------------------
char* vtkXMLKWWindowWriter::GetRootElementName()
{
  return "KWWindow";
}

//----------------------------------------------------------------------------
char* vtkXMLKWWindowWriter::GetUserInterfaceElementName()
{
  return "UserInterface";
}

//----------------------------------------------------------------------------
char* vtkXMLKWWindowWriter::GetUserInterfaceManagerElementName()
{
  return "UserInterfaceManager";
}

//----------------------------------------------------------------------------
vtkXMLKWWindowWriter::vtkXMLKWWindowWriter()
{
  this->OutputUserInterfaceElementOnly = 0;
}

//----------------------------------------------------------------------------
int vtkXMLKWWindowWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
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

  vtkXMLDataElement *ui_elem = this->NewDataElement();
  elem->AddNestedElement(ui_elem);
  ui_elem->Delete();
  this->CreateUserInterfaceElement(ui_elem);

  if (this->OutputUserInterfaceElementOnly)
    {
    return 1;
    }

  // The rest (to be filled)

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLKWWindowWriter::CreateUserInterfaceElement(
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

  // Set name

  ui_elem->SetName(vtkXMLKWWindowWriter::GetUserInterfaceElementName());

  // Set attributes
 
  ui_elem->SetIntAttribute(
    "PropertiesVisibility", obj->GetPropertiesVisiblity());

  // User Interface Manager

  vtkKWUserInterfaceNotebookManager *uim_nb = 
    vtkKWUserInterfaceNotebookManager::SafeDownCast(
      obj->GetUserInterfaceManager());
  if (uim_nb)
    {
    vtkXMLKWUserInterfaceNotebookManagerWriter *xmlw = 
      vtkXMLKWUserInterfaceNotebookManagerWriter::New();
    xmlw->SetObject(uim_nb);
    xmlw->CreateInNestedElement(
      ui_elem, vtkXMLKWWindowWriter::GetUserInterfaceManagerElementName());
    xmlw->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLKWWindowWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutputUserInterfaceElementOnly: "
     << (this->OutputUserInterfaceElementOnly ? "On" : "Off") << endl;
}
