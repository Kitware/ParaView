/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWComposite.h
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
// .NAME vtkKWComposite - an element in a view containing props / properties
// .SECTION Description
// A composite represents an element in the view. It is very similar to
// the notion of an actor in a renderer. The composite is different in 
// that it combines the Actor (vtkProp actually) with some user interface
// code called the properties, and it may also contain more complex
// elements such as filters etc.

#ifndef __vtkKWComposite_h
#define __vtkKWComposite_h

#include "vtkKWNotebook.h"
class vtkKWView;
class vtkKWApplication;
class vtkProp;
class vtkKWWidget;

class VTK_EXPORT vtkKWComposite : public vtkKWObject
{
public:
  vtkTypeMacro(vtkKWComposite,vtkKWObject);

  // Description:
  // Get the View for this class.
  vtkGetObjectMacro(View,vtkKWView);
  virtual void SetView(vtkKWView *view);

  // Description:
  // Get the Prop for this class.
  virtual vtkProp *GetProp() = 0;

  // Description:
  // Initialize properties should be called by any methods
  // that rely on the proerties being created first.
  void InitializeProperties();

  // Description:
  // Displays and/or updates the property ui display
  virtual void ShowProperties() {};

  // Description:
  // Create the properties object, called by InitializeProperties.
  virtual void CreateProperties();

  // Description:
  // Methods to indicate when this composite is the selected composite.
  // These methods are used by subclasses to modify the menu bar
  // for example. When a volume composite is selected it might 
  // add an option to the menu bar to view the 2D slices.
  virtual void Select(vtkKWView *);
  virtual void Deselect(vtkKWView *);
  virtual void MakeSelected();

  // Description:
  // Allow access to the notebook objects.
  vtkGetObjectMacro(Notebook,vtkKWNotebook);
  vtkGetObjectMacro(Notebook2,vtkKWNotebook);

  // Description:
  // Give a composite a chance to close. Called from the vtkKWView when
  // it is closed. The default behavior is to do nothing - can be
  // overridden in a subclass for example to close dialogs.
  virtual void Close() {};

  // Description:
  // Chaining method to serialize an object and its superclasses.
  virtual void SerializeRevision(ostream& os, vtkIndent indent);

  // Description:
  // A chance for the composite to reset itself
  virtual void Reset() {};
  
  // Description:
  // This allows you to set the propertiesParent to any widget you like.  
  // If you do not specify a parent, then the views->PropertyParent is used.  
  // If the composite does not have a view, then a top level window is created.
  void SetPropertiesParent(vtkKWWidget *parent);
  vtkGetObjectMacro(PropertiesParent, vtkKWWidget);
  
protected:
  vtkKWComposite();
  ~vtkKWComposite();
  vtkKWComposite(const vtkKWComposite&) {};
  void operator=(const vtkKWComposite&) {};

  vtkKWNotebook *Notebook;
  vtkKWNotebook *Notebook2;
  int LastSelectedProperty;
  int PropertiesCreated;
  vtkKWWidget *TopLevel;
  vtkKWView *View;

  vtkKWWidget *PropertiesParent;
};


#endif


