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
vtkCxxRevisionMacro(vtkXMLKWUserInterfaceNotebookManagerWriter, "1.1.2.1");

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
char* vtkXMLKWUserInterfaceNotebookManagerWriter::GetDragAndDropEntriesElementName()
{
  return "DragAndDropEntries";
}

//----------------------------------------------------------------------------
char* vtkXMLKWUserInterfaceNotebookManagerWriter::GetDragAndDropEntryElementName()
{
  return "DragAndDropEntry";
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

        // If the page title and the panel name are the same, output the
        // page title only
        // Output the Pinned attribute only the page is pinned, since we
        // assumed a page is not pinned by default.
        
        const char *panel_name = panel->GetName();
        const char *page_title = notebook->GetPageTitle(id);
        if (panel_name && (!page_title || strcmp(panel_name, page_title)))
          {
          p_elem->SetAttribute("PanelName", panel->GetName());
          }
        p_elem->SetAttribute("PageTitle", notebook->GetPageTitle(id));
        int page_pinned = notebook->GetPagePinned(id);
        if (page_pinned)
          {
          p_elem->SetIntAttribute("Pinned", page_pinned);
          }
        }
      }
    }

  // Drag&Drop

  int nb_dd_entries = obj->GetNumberOfDragAndDropEntries();
  if (nb_dd_entries)
    {
    vtkXMLDataElement *dd_elem = this->NewDataElement();
    elem->AddNestedElement(dd_elem);
    dd_elem->Delete();
    dd_elem->SetName(this->GetDragAndDropEntriesElementName());

    for (int idx = 0; idx < nb_dd_entries; idx++)
      {
      ostrstream widget_label;
      ostrstream from_panel_name;
      ostrstream from_page_title;
      ostrstream from_after_widget_label;
      ostrstream to_panel_name; 
      ostrstream to_page_title;
      ostrstream to_after_widget_label;
      
      if (obj->GetDragAndDropEntry(idx, 
                                   widget_label, 
                                   from_panel_name, 
                                   from_page_title, 
                                   from_after_widget_label,
                                   to_panel_name, 
                                   to_page_title, 
                                   to_after_widget_label))
        {
        widget_label << ends;
        from_panel_name << ends;
        from_page_title << ends;
        from_after_widget_label << ends;
        to_panel_name << ends; 
        to_page_title << ends;
        to_after_widget_label << ends;

        vtkXMLDataElement *p_elem = this->NewDataElement();
        dd_elem->AddNestedElement(p_elem);
        p_elem->Delete();
        p_elem->SetName(this->GetDragAndDropEntryElementName());
        p_elem->SetAttribute("WidgetLabel", widget_label.str());

        vtkXMLDataElement *from_elem = this->NewDataElement();
        p_elem->AddNestedElement(from_elem);
        from_elem->Delete();
        from_elem->SetName("From");
        from_elem->SetAttribute("PanelName", from_panel_name.str());
        from_elem->SetAttribute("PageTitle", from_page_title.str());
        from_elem->SetAttribute(
          "AfterWidgetLabel", from_after_widget_label.str());

        vtkXMLDataElement *to_elem = this->NewDataElement();
        p_elem->AddNestedElement(to_elem);
        to_elem->Delete();
        to_elem->SetName("To");
        to_elem->SetAttribute("PanelName", to_panel_name.str());
        to_elem->SetAttribute("PageTitle", to_page_title.str());
        to_elem->SetAttribute(
          "AfterWidgetLabel", to_after_widget_label.str());
        }
      
      widget_label.rdbuf()->freeze(0);
      from_panel_name.rdbuf()->freeze(0);
      from_page_title.rdbuf()->freeze(0);
      from_after_widget_label.rdbuf()->freeze(0);
      to_panel_name.rdbuf()->freeze(0); 
      to_page_title.rdbuf()->freeze(0);
      to_after_widget_label.rdbuf()->freeze(0);
      }
    }

  return 1;
}

