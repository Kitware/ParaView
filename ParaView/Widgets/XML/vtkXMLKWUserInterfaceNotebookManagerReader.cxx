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
#include "vtkXMLKWUserInterfaceNotebookManagerReader.h"

#include "vtkKWNotebook.h"
#include "vtkKWUserInterfaceNotebookManager.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLKWUserInterfaceNotebookManagerWriter.h"

vtkStandardNewMacro(vtkXMLKWUserInterfaceNotebookManagerReader);
vtkCxxRevisionMacro(vtkXMLKWUserInterfaceNotebookManagerReader, "1.2");

//----------------------------------------------------------------------------
char* vtkXMLKWUserInterfaceNotebookManagerReader::GetRootElementName()
{
  return "KWUserInterfaceNotebookManager";
}

//----------------------------------------------------------------------------
int vtkXMLKWUserInterfaceNotebookManagerReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
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
    vtkXMLDataElement *vp_elem = elem->FindNestedElementWithName(
     vtkXMLKWUserInterfaceNotebookManagerWriter::GetVisiblePagesElementName());
    if (vp_elem)
      {
      int nb_vp_elems = vp_elem->GetNumberOfNestedElements();
      for (int idx = 0; idx < nb_vp_elems; idx++)
        {
        vtkXMLDataElement *p_elem = vp_elem->GetNestedElement(idx);
        if (!strcmp(
             p_elem->GetName(), 
             vtkXMLKWUserInterfaceNotebookManagerWriter::GetPageElementName()))
          {
          const char *page_title = p_elem->GetAttribute("PageTitle");
          const char *panel_name = p_elem->GetAttribute("PanelName");

          // As a convenience, the XML writer did not output the
          // panel name if its the same as the page title.

          if (!panel_name)
            {
            panel_name = page_title;
            }

          if (page_title && panel_name)
            {
            vtkKWUserInterfacePanel *panel = obj->GetPanel(panel_name);
            if (panel)
              {
              panel->RaisePage(page_title);
              int pinned;
              if (p_elem->GetScalarAttribute("Pinned", pinned))
                {
                if (pinned)
                  {
                  notebook->PinPage(notebook->GetRaisedPageId());
                  }
                else
                  {
                  notebook->UnpinPage(notebook->GetRaisedPageId());
                  }
                }
              }
            }
          }
        }
      }
    }
  
  // Drag&Drop

  vtkXMLDataElement *dd_elem = elem->FindNestedElementWithName(
    vtkXMLKWUserInterfaceNotebookManagerWriter::
    GetDragAndDropEntriesElementName());
  if (dd_elem)
    {
    int nb_dd_elems = dd_elem->GetNumberOfNestedElements();
    for (int idx = 0; idx < nb_dd_elems; idx++)
      {
      vtkXMLDataElement *p_elem = dd_elem->GetNestedElement(idx);
      if (!strcmp(
            p_elem->GetName(), 
            vtkXMLKWUserInterfaceNotebookManagerWriter::
            GetDragAndDropEntryElementName()))
        {
        const char *widget_label = p_elem->GetAttribute("WidgetLabel");
        if (widget_label)
          {
          vtkXMLDataElement *from_elem = p_elem->FindNestedElementWithName(
            "From");
          vtkXMLDataElement *to_elem = p_elem->FindNestedElementWithName(
            "To");
          if (from_elem && to_elem)
            {
            const char *from_panel_name = from_elem->GetAttribute("PanelName");
            const char *from_page_title = from_elem->GetAttribute("PageTitle");
            const char *from_after_widget_label = 
              from_elem->GetAttribute("AfterWidgetLabel");
            const char *to_panel_name = to_elem->GetAttribute("PanelName"); 
            const char *to_page_title = to_elem->GetAttribute("PageTitle");
            const char *to_after_widget_label = 
              to_elem->GetAttribute("AfterWidgetLabel");
            
            obj->DragAndDropWidget(widget_label, 
                                   from_panel_name, 
                                   from_page_title, 
                                   from_after_widget_label,
                                   to_panel_name, 
                                   to_page_title, 
                                   to_after_widget_label);
            }
          }
        }
      }
    }

  return 1;
}

