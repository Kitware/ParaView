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

