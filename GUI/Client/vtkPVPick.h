/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPick.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPick - A special PVSource.
// .SECTION Description
// This class will set up defaults for thePVData.
// It will also create a special PickDisplay.
// Both of these features should be specified in XML so we
// can get rid of this special case.


#ifndef __vtkPVPick_h
#define __vtkPVPick_h

#include "vtkPVSource.h"

class vtkSMPickDisplay;

class vtkCollection;
class vtkKWFrame;
class vtkKWLabel;
class vtkDataSetAttributes;

class VTK_EXPORT vtkPVPick : public vtkPVSource
{
public:
  static vtkPVPick* New();
  vtkTypeRevisionMacro(vtkPVPick, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  virtual void CreateProperties();

  // Description:
  // Control the visibility of the pick display as well.
  virtual void SetVisibilityNoTrace(int val);

protected:
  vtkPVPick();
  ~vtkPVPick();

  // I am putting this here so that the display is added to the render module
  // only once.  There may be a better check using the Proxy.
  int DisplayHasBeenAddedToTheRenderModule;
  
  vtkSMPickDisplay* PickDisplay;
  vtkKWWidget *DataFrame;

  vtkCollection* LabelCollection;
  virtual void Select();
  void UpdateGUI();
  void ClearDataLabels();
  void InsertDataLabel(const char* label, vtkIdType idx,
                       vtkDataSetAttributes* attr, double* x=0);
  int LabelRow;

  // The real AcceptCallback method.
  virtual void AcceptCallbackInternal();  

  vtkPVPick(const vtkPVPick&); // Not implemented
  void operator=(const vtkPVPick&); // Not implemented
};

#endif
