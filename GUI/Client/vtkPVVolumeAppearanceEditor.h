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
class vtkPVRenderView;
class vtkPVSource;
class vtkPVArrayInformation;
class vtkPVVolumePropertyWidget;
class vtkVolumeProperty; //FIXME: Need a proxy/property instead

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
  // This method returns the user to the source page.
  // I would eventually like to replace this by 
  // a more general back/forward ParaView navigation.
  void BackButtonCallback();

  // Description:
  // Reference to the view is needed for the back callback
  void SetPVRenderView(vtkPVRenderView *view);

  void Close();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  void SetPVSourceAndArrayInfo(vtkPVSource *source,
                               vtkPVArrayInformation *arrayInfo );
  
  void VolumePropertyChangedCallback();
  void VolumePropertyChangingCallback();
  
protected:
  vtkPVVolumeAppearanceEditor();
  ~vtkPVVolumeAppearanceEditor();

  vtkKWPushButton        *BackButton;
  
  vtkPVRenderView        *PVRenderView;

  vtkPVSource            *PVSource;
  vtkPVArrayInformation  *ArrayInfo;

  void                    RenderView();

  vtkPVVolumePropertyWidget *VolumePropertyWidget;
//FIXME:
// Once the VolumeProperty is done properly with properties should remove this 
  vtkVolumeProperty         *InternalVolumeProperty;

  vtkPVVolumeAppearanceEditor(const vtkPVVolumeAppearanceEditor&); // Not implemented
  void operator=(const vtkPVVolumeAppearanceEditor&); // Not implemented
};

#endif
