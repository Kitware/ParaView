/*=========================================================================

  Module:    vtkXMLKWUserInterfaceNotebookManagerWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLKWUserInterfaceNotebookManagerWriter - vtkKWUserInterfaceNotebookManager XML Writer.
// .SECTION Description
// vtkXMLKWUserInterfaceNotebookManagerWriter provides XML writing functionality to 
// vtkKWUserInterfaceNotebookManager.
// .SECTION See Also
// vtkXMLKWUserInterfaceNotebookManagerReader

#ifndef __vtkXMLKWUserInterfaceNotebookManagerWriter_h
#define __vtkXMLKWUserInterfaceNotebookManagerWriter_h

#include "vtkXMLKWUserInterfaceManagerWriter.h"

class VTK_EXPORT vtkXMLKWUserInterfaceNotebookManagerWriter : public vtkXMLKWUserInterfaceManagerWriter
{
public:
  static vtkXMLKWUserInterfaceNotebookManagerWriter* New();
  vtkTypeRevisionMacro(vtkXMLKWUserInterfaceNotebookManagerWriter,vtkXMLKWUserInterfaceManagerWriter);

  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the element used inside that tree to
  // store the visible pages.
  static char* GetVisiblePagesElementName();

  // Description:
  // Return the name of the element used inside that tree to
  // store a page.
  static char* GetPageElementName();

  // Description:
  // Return the name of the element used inside that tree to
  // store the Drag&Drop entries.
  static char* GetDragAndDropEntriesElementName();

  // Description:
  // Return the name of the element used inside that tree to
  // store a Drag&Drop entry.
  static char* GetDragAndDropEntryElementName();

protected:
  vtkXMLKWUserInterfaceNotebookManagerWriter() {};
  ~vtkXMLKWUserInterfaceNotebookManagerWriter() {};  
  
  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLKWUserInterfaceNotebookManagerWriter(const vtkXMLKWUserInterfaceNotebookManagerWriter&);  // Not implemented.
  void operator=(const vtkXMLKWUserInterfaceNotebookManagerWriter&);  // Not implemented.
};

#endif

