/*=========================================================================

  Module:    vtkXMLKWUserInterfaceNotebookManagerReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLKWUserInterfaceNotebookManagerReader.h"

#include "vtkKWNotebook.h"
#include "vtkKWUserInterfaceNotebookManager.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLKWUserInterfaceNotebookManagerWriter.h"

vtkStandardNewMacro(vtkXMLKWUserInterfaceNotebookManagerReader);
vtkCxxRevisionMacro(vtkXMLKWUserInterfaceNotebookManagerReader, "1.3");

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

