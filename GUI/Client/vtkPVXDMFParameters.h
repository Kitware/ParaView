/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXDMFParameters.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVXDMFParameters -
// .SECTION Description

#ifndef __vtkPVXDMFParameters_h
#define __vtkPVXDMFParameters_h

#include "vtkPVObjectWidget.h"

class vtkKWLabel;
class vtkKWFrameLabeled;
class vtkKWScale;
class vtkPVXDMFParametersInternals;

class VTK_EXPORT vtkPVXDMFParameters : public vtkPVObjectWidget
{
public:
  static vtkPVXDMFParameters* New();
  vtkTypeRevisionMacro(vtkPVXDMFParameters, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);

  // Description:
  virtual void Accept();
  
  // Description:
  // Check if the widget was modified.
  void CheckModifiedCallback();
  
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVXDMFParameters* ClonePrototype(vtkPVSource* pvSource,
                             vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void ResetInternal();

  // Description:
  // Initializes widget after creation
  virtual void Initialize();

  // Description:
  // Called during animation and from trace file to set a parameter on
  // the reader on the server.
  void SetParameterIndex(const char* label, int value);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // The label.
  void SetLabel(const char* label);

  // Description:
  // This method updates values from the reader if fromReader is true,
  // from the property otherwise
  void UpdateParameters(int fromReader);

  // Description:
  // This method adds parameter with value and range to the list.
  void AddXDMFParameter(const char* pname, int value, int min, int step, int max);

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                         vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // This method gets called when the user selects this widget to animate.
  // It sets up the script and animation parameters.
  virtual void AnimationMenuCallback(
    vtkPVAnimationInterfaceEntry *ai, const char* name, unsigned int idx);
  virtual void AnimationMenuCallback(vtkPVAnimationInterfaceEntry*) {}


  //BTX
  // Description:
  // Get the frame
  vtkGetObjectMacro(Frame, vtkKWFrameLabeled);
  vtkGetMacro(VTKReaderID, vtkClientServerID);
  //ETX

  void SaveInBatchScript(ofstream *file);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
  // Description:
  // Resets the animation entries (start and end) to values obtained
  // from the range domain
  virtual void ResetAnimationRange(
    vtkPVAnimationInterfaceEntry* ai, const char* name);

protected:
  vtkPVXDMFParameters();
  ~vtkPVXDMFParameters();
  

  vtkPVXDMFParametersInternals* Internals;
  vtkKWFrameLabeled* Frame;

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  vtkSetStringMacro(FrameLabel);
  vtkGetStringMacro(FrameLabel);
  char* FrameLabel;

  // Description:
  // This is the ID of the VTK Xdmf reader.
  vtkClientServerID VTKReaderID;

  // ID of server-side helper.
  vtkClientServerID ServerSideID;
private:
  vtkPVXDMFParameters(const vtkPVXDMFParameters&); // Not implemented
  void operator=(const vtkPVXDMFParameters&); // Not implemented
};

#endif
