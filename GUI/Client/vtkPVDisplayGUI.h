/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDisplayGUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDisplayGUI - Object to represent the output of a PVSource.
// .SECTION Description
// This object combines methods for accessing parallel VTK data, and also an 
// interface for changing the view of the data.  The interface used to be in a 
// superclass called vtkPVActorComposite.  I want to separate the interface 
// from this object, but a superclass is not the way to do it.

#ifndef __vtkPVDisplayGUI_h
#define __vtkPVDisplayGUI_h


#include "vtkPVTracedWidget.h"

class vtkCollection;
class vtkKWBoundsDisplay;
class vtkKWChangeColorButton;
class vtkKWCheckButton;
class vtkKWCheckButton;
class vtkKWEntry;
class vtkKWFrame;
class vtkKWFrameWithScrollbar;
class vtkKWLabel;
class vtkKWFrameLabeled;
class vtkKWOptionMenu;
class vtkKWPushButton;
class vtkKWScale;
class vtkKWThumbWheel;
class vtkKWWidget;
class vtkPVApplication;
class vtkPVSource;
class vtkPVRenderView;
class vtkPVDataSetAttributesInformation;
class vtkPVVolumeAppearanceEditor;
class vtkPVColorSelectionWidget;

// Try to eliminate this !!!!
class vtkPVColorMap;

class VTK_EXPORT vtkPVDisplayGUI : public vtkPVTracedWidget
{
public:
  static vtkPVDisplayGUI* New();
  vtkTypeRevisionMacro(vtkPVDisplayGUI, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Legacy method for old scripts.
  VTK_LEGACY(void SetVisibility(int v));
  VTK_LEGACY(void SetCubeAxesVisibility(int v));
  VTK_LEGACY(void SetPointLabelVisibility(int v));
  VTK_LEGACY(void SetScalarBarVisibility(int v));
  VTK_LEGACY(vtkPVColorMap* GetPVColorMap());

  // Description:
  // Properly close tk widget
  void Close();
  
  // Description:
  // This is set when the source is selected as the current source.
  // Only PVWindow should set this.
  void SetPVSource(vtkPVSource *source);
  vtkGetObjectMacro(PVSource, vtkPVSource);
      
  //===================

  // Description:
  // Translate the actor to the specified location. Also modify the
  // entry widget that controles the translation.
  void SetActorTranslate(double* p);
  void SetActorTranslate(double x, double y, double z);
  void SetActorTranslateNoTrace(double x, double y, double z);
  void GetActorTranslate(double* p);
  void ActorTranslateCallback();
  void ActorTranslateEndCallback();
  
  // Description:
  // Scale the actor. Also modify the entry widget that controles the scaling.
  void SetActorScale(double* p);
  void SetActorScale(double x, double y, double z);
  void SetActorScaleNoTrace(double x, double y, double z);
  void GetActorScale(double* p);
  void ActorScaleCallback();
  void ActorScaleEndCallback();
  
  // Description:
  // Orient the actor. 
  // Also modify the entry widget that controles the orientation.
  void SetActorOrientation(double* p);
  void SetActorOrientation(double x, double y, double z);
  void SetActorOrientationNoTrace(double x, double y, double z);
  void GetActorOrientation(double* p);
  void ActorOrientationCallback();
  void ActorOrientationEndCallback();
  
  // Description:
  // Set the actor origin. 
  // Also modify the entry widget that controles the origin.
  void SetActorOrigin(double* p);
  void SetActorOrigin(double x, double y, double z);
  void SetActorOriginNoTrace(double x, double y, double z);
  void GetActorOrigin(double* p);
  void ActorOriginCallback();
  void ActorOriginEndCallback();
  
  // Description:
  // Set the transparency of the actor.
  void SetOpacity(float f);
  void OpacityChangedCallback();
  void OpacityChangedEndCallback();

  // Description:
  // Create the user interface.
  void Create(vtkKWApplication* app, const char* options);
  
  // Description:
  // This updates the user interface.  It checks first to see if the
  // data has changed.  If nothing has changes, it is smart enough
  // to do nothing.
  void Update();
  void UpdateCubeAxesVisibilityCheck();
  void UpdatePointLabelVisibilityCheck();
  void UpdateColorGUI();
  void UpdateVolumeGUI();
     
  // Description:
  // This method is meant to setup the actor/mapper
  // to best disply it input.  This will involve setting the scalar range,
  // and possibly other properties. 
  void Initialize();

  // Description:
  // This flag turns the visibility of the prop on and off.  These methods transmit
  // the state change to all of the satellite processes.
  void VisibilityCheckCallback();

  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();
  
  // Description:
  // Methods called when item chosen from RepresentationMenu
  void SetRepresentation(const char*);
  void DrawWireframe();
  void DrawSurface();
  void DrawPoints();
  void DrawVolume();
  void DrawOutline();
  
  // Description:
  // Methods called when item chosen from InterpolationMenu
  void SetInterpolation(const char*);
  void SetInterpolationToFlat();
  void SetInterpolationToGouraud();

  // Description:
  // Get the representation menu.
  vtkGetObjectMacro(RepresentationMenu, vtkKWOptionMenu);

  // Description:
  // Get the interpolation menu.
  vtkGetObjectMacro(InterpolationMenu, vtkKWOptionMenu);
    
  // Description:
  // Called when the user presses the "Edit Color Map" button.
  void EditColorMapCallback();
  void DataColorRangeCallback();

  // Description:
  // Called when the user presses the "Edit Volume Appearance" button.
  void EditVolumeAppearanceCallback();
  void ShowVolumeAppearanceEditor();
  
  void ScalarBarCheckCallback();
  vtkGetObjectMacro(ScalarBarCheck, vtkKWCheckButton);

  void CubeAxesCheckCallback();
  vtkGetObjectMacro(CubeAxesCheck, vtkKWCheckButton);
  void PointLabelCheckCallback();
  vtkGetObjectMacro(PointLabelCheck, vtkKWCheckButton);

  void CenterCamera();
  
  // Description:
  // Callback for the change color button.
  void ChangeActorColor(double r, double g, double b);
  vtkGetVector3Macro(ActorColor,double);
    
  // Description:
  // Access to pointSize for scripting.
  void SetPointSize(int size);
  void SetLineWidth(int width);
  
  // Description:
  // Callbacks for point size and line width sliders.
  void ChangePointSize();
  void ChangePointSizeEndCallback();
  void ChangeLineWidth();
  void ChangeLineWidthEndCallback();

  // Description:
  // Access to option menus for scripting.
  vtkGetObjectMacro(ColorSelectionMenu, vtkPVColorSelectionWidget);

  // Description:
  // Callback methods when item chosen from ColorMenu
  void ColorByArray(const char* name, int field);
  void VolumeRenderByArray(const char* name ,int field);
  void ColorByProperty();
  
  // Description:
  // Called by vtkPVSource::DeleteCallback().
  void DeleteCallback();
  
  // Description:
  // Convenience method for rendering.
  vtkPVRenderView* GetPVRenderView(); 

  // Description:
  // The source calls this to update the visibility check button,
  // and to determine whether the scalar bar should be visible.
  void UpdateVisibilityCheck();

  // Description:
  // This determines whether to map array values through a color
  // map or use arrays values as colors directly.  The direct option
  // is only available for unsigned chanr arrays with 1 or 3 components.
  void SetMapScalarsFlag(int val);
  void MapScalarsCheckCallback();

  // Description:
  // This determines whether we will interpolate before or after
  // scalrs are mapped.  Interpolate after is the standard VTK
  // OpenGL vertex coloring.  It gives smooth coloring, but
  // may generate colors not in the map  Interpolating before 
  // mapping uses a 1d texture map.
  void SetInterpolateColorsFlag(int val);
  void InterpolateColorsCheckCallback();

  vtkGetMacro(ColorSetByUser, int);
  vtkGetMacro(ArraySetByUser, int);

  vtkGetMacro(ShouldReinitialize, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Access to these objects from a script
  vtkGetObjectMacro(ResetCameraButton, vtkKWPushButton);
  vtkGetObjectMacro(ActorControlFrame, vtkKWFrameLabeled);
  vtkGetObjectMacro(MainFrame, vtkKWFrameWithScrollbar);

protected:
  vtkPVDisplayGUI();
  ~vtkPVDisplayGUI();

  // Order of these calls makes a difference.  
  // might want to merge some of them.
  // UpdateColorGUI calls them all.
  void UpdateScalarBarVisibilityCheck();
  void UpdateColorMenu();
  void UpdateColorButton();
  void UpdateEditColorMapButton();
  void UpdateInterpolateColorsCheck();
  void UpdateMapScalarsCheck(); 
   
  // This points to the source widget that owns this data widget.
  vtkPVSource *PVSource;

  void SetVolumeAppearanceEditor(vtkPVVolumeAppearanceEditor *appearanceEditor);
  
  //==================================================================
  // Internal versions that do not add to the trace.
  void ColorByPropertyInternal();
  void SetActorColor(double r, double g, double b);

  // Switch between showing the properties for actors and volumes
  void VolumeRenderModeOn();
  void VolumeRenderModeOff();
  
  // A flag that helps Update determine 
  // whether to set the default color.
  int ColorSetByUser;
  int ArraySetByUser;

  // A flag to let the source know that Initialize should be called again.
  // This flag is set when the data set type is unknown.
  int ShouldReinitialize;

  vtkKWFrameWithScrollbar *MainFrame;

  vtkKWFrameLabeled *ColorFrame;
  vtkKWFrameLabeled *VolumeAppearanceFrame;
  vtkKWFrameLabeled *DisplayStyleFrame;
  vtkKWFrameLabeled *ViewFrame;
  
  vtkKWLabel *ColorMenuLabel;
  vtkPVColorSelectionWidget* ColorSelectionMenu;
  
  vtkKWChangeColorButton* ColorButton;
  vtkKWWidget*     EditColorMapButtonFrame;
  vtkKWPushButton* EditColorMapButton;
  vtkKWPushButton* DataColorRangeButton;

  vtkKWLabel *VolumeScalarsMenuLabel;
  vtkPVColorSelectionWidget* VolumeScalarSelectionWidget;

  vtkKWPushButton *EditVolumeAppearanceButton;

  vtkKWLabel *RepresentationMenuLabel;
  vtkKWOptionMenu *RepresentationMenu;
  vtkKWLabel *InterpolationMenuLabel;
  vtkKWOptionMenu *InterpolationMenu;

  vtkKWLabel      *PointSizeLabel;
  vtkKWThumbWheel *PointSizeThumbWheel;
  vtkKWLabel      *LineWidthLabel;
  vtkKWThumbWheel *LineWidthThumbWheel;
  
  vtkKWCheckButton *VisibilityCheck;
  vtkKWCheckButton *ScalarBarCheck;

  vtkKWCheckButton *MapScalarsCheck;
  vtkKWCheckButton *InterpolateColorsCheck;
  
  // For translating actor
  vtkKWFrameLabeled* ActorControlFrame;
  vtkKWLabel*        TranslateLabel;
  vtkKWThumbWheel*   TranslateThumbWheel[3];
  vtkKWLabel*        ScaleLabel;
  vtkKWThumbWheel*   ScaleThumbWheel[3];
  vtkKWLabel*        OrientationLabel;
  vtkKWScale*        OrientationScale[3];
  vtkKWLabel*        OriginLabel;
  vtkKWThumbWheel*   OriginThumbWheel[3];
  vtkKWLabel*        OpacityLabel;
  vtkKWScale*        OpacityScale;

  vtkKWCheckButton *CubeAxesCheck;
  // Adding point labelling back in.  This should now work in multi process mode too.
  // The point label display should mask points if there are too many.
  vtkKWCheckButton *PointLabelCheck;
  vtkKWPushButton *ResetCameraButton;

  double ActorColor[3];

  int VolumeRenderMode;

  // Keep widget enabled state so they can be disabled globally
  int MapScalarsCheckVisible;
  int InterpolateColorsCheckVisible;
  int EditColorMapButtonVisible;
  int ColorButtonVisible;
  int ScalarBarCheckVisible;

  vtkPVVolumeAppearanceEditor *VolumeAppearanceEditor;
    
  void UpdateInternal();
  void UpdateActorControl();  
  void UpdateActorControlResolutions();

private:
  vtkPVDisplayGUI(const vtkPVDisplayGUI&); // Not implemented
  void operator=(const vtkPVDisplayGUI&); // Not implemented
};

#endif
