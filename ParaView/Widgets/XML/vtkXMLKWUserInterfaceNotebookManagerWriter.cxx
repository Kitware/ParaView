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
#include "vtkXMLKWUserInterfaceNotebookManagerWriter.h"

#include "vtkKWNotebook.h"
#include "vtkKWUserInterfaceNotebookManager.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLKWUserInterfaceNotebookManagerWriter);
vtkCxxRevisionMacro(vtkXMLKWUserInterfaceNotebookManagerWriter, "1.2");

//----------------------------------------------------------------------------
char* vtkXMLKWUserInterfaceNotebookManagerWriter::GetRootElementName()
{
  return "KWUserInterfaceNotebookManager";
}

//----------------------------------------------------------------------------
char* vtkXMLKWUserInterfaceNotebookManagerWriter::GetVisiblePagesElementName()
{
  return "VisiblePages";
}

//----------------------------------------------------------------------------
char* vtkXMLKWUserInterfaceNotebookManagerWriter::GetPageElementName()
{
  return "Page";
}

//----------------------------------------------------------------------------
int vtkXMLKWUserInterfaceNotebookManagerWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkKWUserInterfaceNotebookManager *obj = 
    vtkKWUserInterfaceNotebookManager::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The KWUserInterfaceNotebookManager is not set!");
    return 0;
    }

  // Visible pages

  vtkKWNotebook *notebook = obj->GetNotebook();
  if (notebook)
    {
    int page_nb = notebook->GetNumberOfVisiblePages();
    if (page_nb)
      {
      vtkXMLDataElement *vp_elem = this->NewDataElement();
      elem->AddNestedElement(vp_elem);
      vp_elem->Delete();
      vp_elem->SetName(this->GetVisiblePagesElementName());

      for (int idx = page_nb - 1; idx >= 0; idx--)
        {
        vtkIdType id = notebook->GetVisiblePageId(idx);
        if (id < 0)
          {
          continue;
          }
        int tag = notebook->GetPageTag(id);
        vtkKWUserInterfacePanel *panel = obj->GetPanel(tag);
        if (!panel)
          {
          continue;
          }
        
        vtkXMLDataElement *p_elem = this->NewDataElement();
        vp_elem->AddNestedElement(p_elem);
        p_elem->Delete();
        p_elem->SetName(this->GetPageElementName());
        p_elem->SetAttribute("PanelName", panel->GetName());
        p_elem->SetAttribute("PageTitle", notebook->GetPageTitle(id));
        p_elem->SetIntAttribute("Pinned", notebook->GetPagePinned(id));
        }
      }
    }

  return 1;
}

