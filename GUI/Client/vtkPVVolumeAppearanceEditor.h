/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVolumeAppearanceEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVVolumeAppearanceEditor - Edit volume appearance
// .SECTION Description
// This is a simple volume appearance editor that provides some
// basic controls for adjusting the color and opacity transfer functions.


#ifndef __vtkPVVolumeAppearanceEditor_h
#define __vtkPVVolumeAppearanceEditor_h


#include "vtkKWWidget.h"

class vtkKWPushButton;
class vtkKWApplication;
class vtkPVRenderView;

class VTK_EXPORT vtkPVVolumeAppearanceEditor : public vtkKWWidget
{
public:
  static vtkPVVolumeAppearanceEditor* New();
  vtkTypeRevisionMacro(vtkPVVolumeAppearanceEditor, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Just like in vtk data objects, this method makes a data object
  // that is of the same type as the original.  It is used for creating
  // the output pvData in pvDataSetToDataSetFilters.
  virtual vtkPVVolumeAppearanceEditor *MakeObject()
    { vtkErrorMacro("No MakeObject"); return NULL;}
      
  // Description:
  // Looks at all of the data object for a global range.
  // This also sets the color map to automatic.  In the future,
  // it will rescale to match changes in the global scalar range.
  void ResetScalarRange();
  void ResetScalarRangeInternal();
  
  // Description:
  // This method returns the user to the source page.
  // I would eventually like to replace this by 
  // a more general back/forward ParaView navigation.
  void BackButtonCallback();

  // Description:
  // Reference to the view is needed for the back callback
  void SetPVRenderView(vtkPVRenderView *view);
  vtkPVRenderView* GetPVRenderView() { return this->PVRenderView;}

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkPVVolumeAppearanceEditor();
  ~vtkPVVolumeAppearanceEditor();

  vtkKWPushButton*   BackButton;
  
  vtkPVRenderView *PVRenderView;

  
  vtkPVVolumeAppearanceEditor(const vtkPVVolumeAppearanceEditor&); // Not implemented
  void operator=(const vtkPVVolumeAppearanceEditor&); // Not implemented
};

#endif
