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
// .NAME vtkKWListSelectOrder - a single line text entry widget
// .SECTION Description
// A simple widget used for collecting keyboard input from the user. This
// widget provides support for single line input.

#ifndef __vtkKWListSelectOrder_h
#define __vtkKWListSelectOrder_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWListBox;
class vtkKWPushButton;

class VTK_EXPORT vtkKWListSelectOrder : public vtkKWWidget
{
public:
  static vtkKWListSelectOrder* New();
  vtkTypeRevisionMacro(vtkKWListSelectOrder,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Add a string element to the source list if it is not already there or on
  // the final list. The optional argument force will make sure the item is
  // added to the source list and removed from final if it is already there.
  virtual void AddSourceElement(const char*, int force = 0);

  // Description:
  // Add a string element to the final list if it is not already there or on
  // the final list. The optional argument force will make sure the item is
  // added to the final list and removed from source if it is already there.
  virtual void AddFinalElement(const char*, int force = 0);

  // Description:
  // Get the number of elements on the final list.
  virtual int GetNumberOfElementsOnSourceList();
  virtual int GetNumberOfElementsOnFinalList();

  // Description:
  // Get the element from the list.
  virtual const char* GetElementFromSourceList(int idx);
  virtual const char* GetElementFromFinalList(int idx);

  // Description:
  // Get the index of the item.
  virtual int GetElementIndexFromSourceList(const char* element);
  virtual int GetElementIndexFromFinalList(const char* element);

  // Description:
  // Callbacks.
  virtual void AddCallback();
  virtual void AddAllCallback();
  virtual void RemoveCallback();
  virtual void RemoveAllCallback();
  virtual void UpCallback();
  virtual void DownCallback();

protected:
  vtkKWListSelectOrder();
  ~vtkKWListSelectOrder();

  vtkKWListBox* SourceList;
  vtkKWListBox* FinalList;

  vtkKWPushButton* AddButton;
  vtkKWPushButton* AddAllButton;
  vtkKWPushButton* RemoveButton;
  vtkKWPushButton* RemoveAllButton;
  vtkKWPushButton* UpButton;
  vtkKWPushButton* DownButton;

  void MoveWholeList(vtkKWListBox* l1, vtkKWListBox* l2);
  void MoveSelectedList(vtkKWListBox* l1, vtkKWListBox* l2);
  void MoveList(vtkKWListBox* l1, vtkKWListBox* l2, const char* list);
  void ShiftItems(vtkKWListBox* l1, int down);
  void AddElement(vtkKWListBox* l1, vtkKWListBox* l2, const char* element, int force);
  
private:
  vtkKWListSelectOrder(const vtkKWListSelectOrder&); // Not implemented
  void operator=(const vtkKWListSelectOrder&); // Not Implemented
};


#endif



