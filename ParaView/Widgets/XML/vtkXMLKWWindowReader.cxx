/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
vtkCxxRevisionMacro(vtkXMLKWWindowReader, "1.3");

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

