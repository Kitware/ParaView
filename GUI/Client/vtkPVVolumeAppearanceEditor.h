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
class vtkKWRange;
class vtkKWLabeledFrame;
class vtkPVArrayInformation;
class vtkPVSource;
class vtkKWLabel;
class vtkKWScale;
class vtkKWChangeColorButton;
class vtkKWOptionMenu;
class vtkKWMenuButton;
class vtkKWWidget;

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

  void ColorButtonCallback( float r, float g, float b );
  
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

  void SetPVSourceAndArrayInfo( vtkPVSource *source,
                                vtkPVArrayInformation *arrayInfo );
  
  void ScalarOpacityRampChanged();
  void ScalarOpacityRampChangedInternal();

  void ScalarOpacityUnitDistanceChanged();
  void ScalarOpacityUnitDistanceChangedInternal();

  void ColorRampChanged();
  void ColorRampChangedInternal();

  void ColorMapLabelConfigureCallback(int width, int height);
  
protected:
  vtkPVVolumeAppearanceEditor();
  ~vtkPVVolumeAppearanceEditor();

  vtkKWLabeledFrame      *ScalarOpacityFrame;
  vtkKWLabeledFrame      *ColorFrame;
  vtkKWPushButton        *BackButton;
  
  vtkKWLabel             *ScalarOpacityRampLabel;
  vtkKWRange             *ScalarOpacityRampRange;
  vtkKWLabel             *ScalarOpacityStartValueLabel;
  vtkKWScale             *ScalarOpacityStartValueScale;
  vtkKWLabel             *ScalarOpacityEndValueLabel;
  vtkKWScale             *ScalarOpacityEndValueScale;
  vtkKWLabel             *ScalarOpacityUnitDistanceLabel;
  vtkKWScale             *ScalarOpacityUnitDistanceScale;
  
  vtkKWLabel             *ColorRampLabel;
  vtkKWRange             *ColorRampRange;
  vtkKWWidget            *ColorEditorFrame;
  vtkKWChangeColorButton *ColorStartValueButton;
  vtkKWChangeColorButton *ColorEndValueButton;
  vtkKWLabel             *ColorMapLabel;
  
  unsigned char          *MapData;
  int                     MapDataSize;
  int                     MapWidth;
  int                     MapHeight;
  
  void                    UpdateMap(int width, int height);
  
  vtkPVRenderView        *PVRenderView;

  double                  ScalarRange[2];
  float                   StartOpacity;
  float                   EndOpacity;
  
  vtkPVSource            *PVSource;
  vtkPVArrayInformation  *ArrayInfo;

  void                    RenderView();
  
  vtkPVVolumeAppearanceEditor(const vtkPVVolumeAppearanceEditor&); // Not implemented
  void operator=(const vtkPVVolumeAppearanceEditor&); // Not implemented
};

#endif
