/*=========================================================================

  Module:    vtkKWComposite.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#include "vtkKWObject.h"

class vtkKWApplication;
class vtkKWNotebook;
class vtkKWView;
class vtkKWWidget;
class vtkProp;

class VTK_EXPORT vtkKWComposite : public vtkKWObject
{
public:
  vtkTypeRevisionMacro(vtkKWComposite,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the View for this class.
  vtkGetObjectMacro(View,vtkKWView);
  virtual void SetView(vtkKWView *view);

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
  // Avoid windows name mangling.
# define GetPropA GetProp
# define GetPropW GetProp
#endif

  //BTX
  // Description:
  // Get the prop for this composite.
  vtkProp* GetProp();
  //ETX

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef GetPropW
# undef GetPropA
  //BTX
  // Define possible mangled names.
  vtkProp* GetPropA();
  vtkProp* GetPropW();
  //ETX
#endif

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

  vtkKWNotebook *Notebook;
  vtkKWNotebook *Notebook2;
  int LastSelectedProperty;
  int PropertiesCreated;
  vtkKWWidget *TopLevel;
  vtkKWView *View;

  vtkKWWidget *PropertiesParent;

  //BTX
  // Real implementation of GetProp method.  This is the one that
  // subclasses must define.
  virtual vtkProp* GetPropInternal() = 0;
  //ETX
private:
  vtkKWComposite(const vtkKWComposite&); // Not implemented
  void operator=(const vtkKWComposite&); // Not Implemented
};


#endif



