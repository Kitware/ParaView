/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDummyWidget.h
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
// .NAME vtkPVDummyWidget - an empty widget
// .SECTION Description
// This empty widget can be used to create containers/select widgets which
// require at least one widget without anything visible.

#ifndef __vtkPVDummyWidget_h
#define __vtkPVDummyWidget_h

#include "vtkPVWidget.h"

class VTK_EXPORT vtkPVDummyWidget : public vtkPVWidget
{
public:
  static vtkPVDummyWidget* New();
  vtkTypeRevisionMacro(vtkPVDummyWidget, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVDummyWidget* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // Trying out a ne protocal.
  virtual void AcceptInternal(vtkClientServerID) { this->ModifiedFlag = 0;}
  virtual void ResetIntenral(vtkClientServerID) { this->ModifiedFlag = 0;}

  // Description:
  // Empty method to keep superclass from complaining.
  virtual void Trace(ofstream*) {};


  // Description:
  // This does nothing.  It is only here to avoid a paraview warning.
  virtual void SaveInBatchScript(ofstream*) {};

protected:
  vtkPVDummyWidget();
  ~vtkPVDummyWidget();


//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  vtkPVDummyWidget(const vtkPVDummyWidget&); // Not implemented
  void operator=(const vtkPVDummyWidget&); // Not implemented

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};

#endif
