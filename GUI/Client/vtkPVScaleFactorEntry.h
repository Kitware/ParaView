/*=========================================================================

  Program:   ParaView
  Module:    vtkPVScaleFactorEntry.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVScaleFactorEntry - entry specifically for scale factors
// .SECTION Description
// vtkPVScaleFactorEntry is a subclass of vtkPVVectorEntry that depends
// on a vtkPVInputMenu to determine what its default scale value should be.

#ifndef __vtkPVScaleFactorEntry_h
#define __vtkPVScaleFactorEntry_h

#include "vtkPVVectorEntry.h"

class vtkPVInputMenu;

class VTK_EXPORT vtkPVScaleFactorEntry : public vtkPVVectorEntry
{
public:
  static vtkPVScaleFactorEntry* New();
  vtkTypeRevisionMacro(vtkPVScaleFactorEntry, vtkPVVectorEntry);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This input menu supplies the data set.
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);
  
  // Description:
  // This is called to update the menus if something (InputMenu) changes.
  virtual void Update();

  // Description:
  // Move widget state to vtk object or back.
  virtual void ResetInternal();

  vtkSetMacro(ScaleFactor, float);
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkPVScaleFactorEntry();
  ~vtkPVScaleFactorEntry();
  
  virtual void UpdateScaleFactor();

//BTX
  virtual void CopyProperties(vtkPVWidget *clone, vtkPVSource *pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  virtual int ReadXMLAttributes(vtkPVXMLElement* element,
                                vtkPVXMLPackageParser* parser);  
  
  vtkPVInputMenu *InputMenu;
  vtkPVSource *Input;
  void SetInput(vtkPVSource *input);

  float ScaleFactor;

private:
  vtkPVScaleFactorEntry(const vtkPVScaleFactorEntry&); // Not implemented
  void operator=(const vtkPVScaleFactorEntry&); // Not implemented
};

#endif
