/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWidget.h
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
// .NAME vtkPVWidget - Intelligent widget for building source interfaces.
// .SECTION Description
// vtkPVWidget is a superclass for widgets that can be used to build
// interfaces for vtkPVSources.  These widgets combine the UI of vtkKWWidgets,
// but can synchronize themselves with vtkObjects.  When the "Accept" method
// is called, this widgets sets the vtkObjects ivars based on the widgets
// value.  When the "Reset" method is called, the vtkObjects state is used to 
// set the widgets value.  When the widget has been modified, and the "Accept"
// method is called,  the widget will save the transition in the trace file.

#ifndef __vtkPVWidget_h
#define __vtkPVWidget_h

#include "vtkKWWidget.h"

class vtkKWMenu;
class vtkPVSource;
class vtkPVApplication;
class vtkPVAnimationInterface;
class vtkPVXMLElement;
class vtkPVXMLPackageParser;
class vtkPVWindow;

//BTX
template <class key, class data> 
class vtkArrayMap;
template <class value>
class vtkLinkedList;
//ETX

class VTK_EXPORT vtkPVWidget : public vtkKWWidget
{
public:
  vtkTypeMacro(vtkPVWidget, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The methods get called when the Accept button is pressed. 
  // It sets the VTK objects value using this widgets value.
  virtual void Accept();

  // Description:
  // The methods get called when the Reset button is pressed. 
  // It sets this widgets value using the VTK objects value.
  virtual void Reset();
  void ForceReset();

  // Description:
  // The methods get called when reset is called.  
  // It can also get called on its own.  If the widget has options 
  // or configuration values dependent on the VTK object, this method
  // set these configuation object using the VTK object.
  virtual void Update();

  // Description:
  // Widgets that depend on the value of this widget can 
  // set up a dependance here.  When ModifedCallback or Update is called 
  // on this widget, it will call Update on widgets in this list.  
  // I could have used event callbacks, but descided
  // it would be easier to just keep a collection of dependances.
  void AddDependent(vtkPVWidget *widget);

  // Description
  // Remove a dependent
  void RemoveDependent(vtkPVWidget *widget);

  // Description
  // Remove all the dependents
  void RemoveAllDependents();

  // Description:
  // This method is called when the source that contains this widget
  // is selected. This is empty implementation but subclasses can
  // overwrite it.
  virtual void Select() {}

  // Description:
  // This method is called when the source that contains this widget
  // is deselected. This is empty implementation but subclasses can
  // overwrite it.
  virtual void Deselect() {}

  // Description:
  // This commands is an optional action that will be called when
  // the widget is modified.  Really we should change "ModifiedFlag" 
  // to some other name, because is gets confused with vtkObject::Modified().
  void SetModifiedCommand(const char* cmdObject, const char* methodAndArgs);

  // Description:
  // This callback gets called when the user changes the widgets value,
  // or a script changes the widgets value.  Ideally, this method should 
  // be protected.
  virtual void ModifiedCallback();

  // Description:
  // Access to the flag that indicates whether the widgets
  // has been modified and is out of sync with its VTK object.
  vtkGetMacro(ModifiedFlag,int);
  
  // Description:
  // Conveniance method that casts the application to a PV application.
  vtkPVApplication *GetPVApplication();

  // Description:
  // Save this widget to a file.  
  virtual void SaveInTclScript(ofstream *file);
  
  // Description:
  // This mehtod is used by the animation editor to access all the animation 
  // scripts available to modify the object.  The menu commands set the
  // script of the editor.  It would be nice if the returned script would
  // modify the user interface as well as the pipeline.  We would then have 
  // to find a new way of exporting the animation to a VTK script.
  // It would also be nice if the selection of a method would also
  // set the min/max/step animation parametets.
  virtual void AddAnimationScriptsToMenu(
    vtkKWMenu* vtkNotUsed(menu), 
    vtkPVAnimationInterface* vtkNotUsed(object) ) {};
 
  // Description:
  // Create the widget. All sub-classes should use this
  // signature because widgets are created using vtkPVWidget
  // pointers after cloning.
  virtual void Create(vtkKWApplication* app) = 0;

  // Description:
  // Need the source to get the input.
  // I would like to get rid of this ivar.
  // It is not reference counted for fear of loops.
  void SetPVSource(vtkPVSource *pvs) { this->PVSource = pvs;}
  vtkPVSource *GetPVSource() { return this->PVSource;}

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVWidget* ClonePrototype(vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
    {
      return this->ClonePrototypeInternal(pvSource, map);
    }
//ETX

  // Description:
  // Called by vtkPVXMLPackageParser to configure the widget from XML
  // attributes.
  virtual int ReadXMLAttributes(vtkPVXMLElement* element,
                                vtkPVXMLPackageParser* parser);
protected:
  vtkPVWidget();
  ~vtkPVWidget();

  char *ModifiedCommandObjectTclName;
  char *ModifiedCommandMethod;
  vtkSetStringMacro(ModifiedCommandObjectTclName);
  vtkSetStringMacro(ModifiedCommandMethod);
  
  // This flag indicates that the widget has changed and should be
  // added to the trace file.
  int ModifiedFlag;

  vtkPVSource* PVSource;

//BTX
  virtual vtkPVWidget* ClonePrototypeInternal(vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);


  vtkLinkedList<void*>* Dependents;
//ETX

  vtkPVWidget(const vtkPVWidget&); // Not implemented
  void operator=(const vtkPVWidget&); // Not implemented

  vtkPVWidget* GetPVWidgetFromParser(vtkPVXMLElement* element,
                                     vtkPVXMLPackageParser* parser);
  vtkPVWindow* GetPVWindowFormParser(vtkPVXMLPackageParser* parser);
};

#endif

