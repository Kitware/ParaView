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


#include "vtkKWFrame.h"

class vtkCollection;
class vtkKWBoundsDisplay;
class vtkKWChangeColorButton;
class vtkKWCheckButton;
class vtkKWCheckButton;
class vtkKWEntry;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWLabeledEntry;
class vtkKWLabeledFrame;
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

// Try to eliminate this !!!!
class vtkData;
class vtkPVColorMap;

class VTK_EXPORT vtkPVDisplayGUI : public vtkKWFrame
{
public:
  static vtkPVDisplayGUI* New();
  vtkTypeRevisionMacro(vtkPVDisplayGUI, vtkKWFrame);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Leagacy method for old scripts.
  void SetVisibility(int v);
  void SetCubeAxesVisibility(int v);
  void SetScalarBarVisibility(int v);
  vtkPVColorMap* GetPVColorMap();
  
  // Description:
  // Just like in vtk data objects, this method makes a data object
  // that is of the same type as the original.  It is used for creating
  // the output pvData in pvDataSetToDataSetFilters.
  virtual vtkPVDisplayGUI *MakeObject(){vtkErrorMacro("No MakeObject");return NULL;}
  
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
  void UpdateColorGUI();
     
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
  vtkGetObjectMacro(ColorMenu, vtkKWOptionMenu);

  // Description:
  // Callback methods when item chosen from ColorMenu
  void ColorByProperty();
  void ColorByPointField(const char *name, int numComps);
  void ColorByCellField(const char *name, int numComps);
  
  // Description:
  // Select which point field to use for volume rendering
  void VolumeRenderPointField(const char *name);
  
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

  // Description:
  // Methods for point labelling.  This feature only works in single-process
  // mode.  To be changed/moved when we rework 2D rendering in ParaView.
  void PointLabelCheckCallback();
  void SetPointLabelVisibility(int val, int changeButtonState = 1);
  
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
  vtkGetObjectMacro(ActorControlFrame, vtkKWLabeledFrame);

  // Description:
  // Used during state file loading to restore the transfer functions
  void ClearVolumeOpacity();
  void AddVolumeOpacity( double scalar,
                         double opacity );
  void ClearVolumeColor();
  void AddVolumeColor( double scalar,
                       double r, double g, double b );

  void SetVolumeOpacityUnitDistance( double d );
    
protected:
  vtkPVDisplayGUI();
  ~vtkPVDisplayGUI();

  int InstanceCount;
  
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
  void ColorByPointFieldInternal(const char *name, int numComps);
  void ColorByCellFieldInternal(const char *name, int numComps);
  void SetActorColor(double r, double g, double b);
  void VolumeRenderPointFieldInternal(const char *name);
  
  // A flag that helps Update determine 
  // whether to set the default color.
  int ColorSetByUser;
  int ArraySetByUser;
    
  vtkKWLabeledFrame *ColorFrame;
  vtkKWLabeledFrame *VolumeAppearanceFrame;
  vtkKWLabeledFrame *DisplayStyleFrame;
  vtkKWLabeledFrame *ViewFrame;
  
  vtkKWLabel *ColorMenuLabel;
  vtkKWOptionMenu *ColorMenu;

  vtkKWChangeColorButton* ColorButton;
  vtkKWWidget*     EditColorMapButtonFrame;
  vtkKWPushButton* EditColorMapButton;
  vtkKWPushButton* DataColorRangeButton;

  vtkKWLabel *VolumeScalarsMenuLabel;
  vtkKWOptionMenu *VolumeScalarsMenu;

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
  vtkKWLabeledFrame* ActorControlFrame;
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
  vtkKWPushButton *ResetCameraButton;

  double ActorColor[3];

  // Switch between showing the properties for actors and volumes
  void VolumeRenderModeOn();
  void VolumeRenderModeOff();

  int VolumeRenderMode;

  // Keep widget enabled state so they can be disabled globally
  int MapScalarsCheckVisible;
  int InterpolateColorsCheckVisible;
  int EditColorMapButtonVisible;
  int ColorButtonVisible;
  int ScalarBarCheckVisible;

  vtkPVVolumeAppearanceEditor *VolumeAppearanceEditor;
  
  // Adding point labelling back in.  This only works in single-process mode.
  // This code will be changed/moved when we rework 2D rendering in ParaView.
  vtkKWCheckButton *PointLabelCheck;
  
  void UpdateInternal();
  void UpdateActorControl();  
  void UpdateActorControlResolutions();

  vtkPVDisplayGUI(const vtkPVDisplayGUI&); // Not implemented
  void operator=(const vtkPVDisplayGUI&); // Not implemented
};

#endif
