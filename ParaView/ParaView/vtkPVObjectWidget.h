/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVObjectWidget.h
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
// .NAME vtkPVObjectWidget - Widget the represents an objects variable.
// .SECTION Description
// vtkPVObjectWidget is a special class of vtkPVWidget that
// represents a VTK object's variable.  It has ivars for the VTK objects
// name and its variable name.  The Reset and Accept commands can format
// scripts from these variables.  The name of this class may not be the best.

#ifndef __vtkPVObjectWidget_h
#define __vtkPVObjectWidget_h

#include "vtkPVWidget.h"

class VTK_EXPORT vtkPVObjectWidget : public vtkPVWidget
{
public:
  vtkTypeMacro(vtkPVObjectWidget, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The point of a PV widget is that it is an interface for
  // some objects state/ivars.  This is one way the object/variable
  // can be specified. Subclasses may have seperate or addition
  // variables for specifying the relationship.
  void SetObjectVariable(const char *objectTclName, const char *var);
  vtkSetStringMacro(ObjectTclName);
  vtkGetStringMacro(ObjectTclName);
  vtkSetStringMacro(VariableName);
  vtkGetStringMacro(VariableName);

  // Description:
  // An interface for saving a widget into a script.
  virtual void SaveInTclScript(ofstream *file);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVObjectWidget* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

protected:
  vtkPVObjectWidget();
  ~vtkPVObjectWidget();
  virtual void SerializeRevision(ostream& os, vtkIndent indent);
  virtual void SerializeSelf(ostream& os, vtkIndent indent);

  char *ObjectTclName;
  char *VariableName;

  vtkPVObjectWidget(const vtkPVObjectWidget&); // Not implemented
  void operator=(const vtkPVObjectWidget&); // Not implemented

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};

#endif
