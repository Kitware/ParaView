/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContainerWidget.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkPVContainerWidget -
// .SECTION Description


#ifndef __vtkPVContainerWidget_h
#define __vtkPVContainerWidget_h

#include "vtkPVWidget.h"

//BTX
template <class key, class data> 
class vtkArrayMap;
template <class value>
class vtkLinkedList;
//ETX

class VTK_EXPORT vtkPVContainerWidget : public vtkPVWidget
{
public:
  static vtkPVContainerWidget* New();
  vtkTypeMacro(vtkPVContainerWidget, vtkPVWidget);
  
  // Description:
  // Creates common widgets.
  void Create(vtkKWApplication *app);

  // Description:
  // Add widgets to the possible selection.  The vtkValue
  // is value used to set the vtk object variable.
  void AddPVWidget(vtkPVWidget *pvw);
  
  // Description:
  // Called when accept button is pushed.
  // Adds to the trace file and sets the objects variable from UI.
  virtual void Accept();

  // Description:
  // Called when reset button is pushed.
  // Sets UI current value from objects variable.
  virtual void Reset();

  // Description:
  // For saving the widget into a VTK tcl script.
  void SaveInTclScript(ofstream *file);

  // Description:
  // Return ith widget.
  vtkPVWidget* GetPVWidget(vtkIdType i);

  // Description:
  // Return the widget with the corresponding trace name.
  vtkPVWidget* GetPVWidget(const char* traceName);
    
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVContainerWidget* ClonePrototype(vtkPVSource* pvSource,
				    vtkArrayMap<vtkPVWidget*, 
				    vtkPVWidget*>* map);
//ETX

protected:
  vtkPVContainerWidget();
  ~vtkPVContainerWidget();

//BTX
  vtkLinkedList<vtkPVWidget*>* Widgets;

  virtual vtkPVWidget* ClonePrototypeInternal(vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVContainerWidget(const vtkPVContainerWidget&); // Not implemented
  void operator=(const vtkPVContainerWidget&); // Not implemented
};

#endif
