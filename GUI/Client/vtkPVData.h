/*=========================================================================

  Program:   ParaView
  Module:    vtkPVData.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVData - Object to represent the output of a PVSource.
// .SECTION Description
// This object combines methods for accessing parallel VTK data, and also an 
// interface for changing the view of the data.  The interface used to be in a 
// superclass called vtkPVActorComposite.  I want to separate the interface 
// from this object, but a superclass is not the way to do it.

#ifndef __vtkPVData_h
#define __vtkPVData_h


#include "vtkKWWidget.h"

class vtkCollection;
class vtkCubeAxesActor2D;
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
class vtkPVColorMap;
class vtkPVSource;
class vtkPVRenderView;
class vtkPVDataSetAttributesInformation;
class vtkPVVolumeAppearanceEditor;

// Try to eliminate this !!!!
class vtkData;

class VTK_EXPORT vtkPVData : public vtkKWWidget
{
public:
  static vtkPVData* New();
  vtkTypeRevisionMacro(vtkPVData, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This also creates the parallel vtk objects for the composite.
  // (actor, mapper, ...)
  void SetPVApplication(vtkPVApplication *pvApp);
  
  void SetApplication(vtkKWApplication *)
    {
      vtkErrorMacro("vtkPVData::SetApplication should not be used. Use SetPVApplcation instead.");
    }
  
  // Description:
  // Just like in vtk data objects, this method makes a data object
  // that is of the same type as the original.  It is used for creating
  // the output pvData in pvDataSetToDataSetFilters.
  virtual vtkPVData *MakeObject(){vtkErrorMacro("No MakeObject");return NULL;}
  
  // Description:
  // DO NOT CALL THIS IF YOU ARE NOT A PVSOURCE!
  // The composite sets this so this data widget will know who owns it.
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
  void CreateProperties();
  
  // Description:
  // This updates the user interface.  It checks first to see if the
  // data has changed.  If nothing has changes, it is smart enough
  // to do nothing.
  void UpdateProperties();

  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  void CreateParallelTclObjects(vtkPVApplication *pvApp);
      
  // Description:
  // This method is meant to setup the actor/mapper
  // to best disply it input.  This will involve setting the scalar range,
  // and possibly other properties. 
  void Initialize();

  // Description:
  // This flag turns the visibility of the prop on and off.  These methods transmit
  // the state change to all of the satellite processes.
  void SetVisibility(int v);
  vtkGetMacro(Visibility, int);
  vtkBooleanMacro(Visibility, int);
  void VisibilityCheckCallback();
    
  // Description:
  // Sets the color range of all the mappers (all procs) and updates
  // the user interface as well.
  void SetColorRange(double min, double max);

  // Description:
  // This computes the union of the range of the data (current color by)
  // across all process. Of cousre it returns the range.
  void GetColorRange(double range[2]);
  
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();
  
  // Description:
  // to change the ambient component of the light
  void AmbientChanged();
  void SetAmbient(double ambient);
  
  // Description:
  // Methods called when item chosen from RepresentationMenu
  void SetRepresentation(const char*);
  void DrawWireframe();
  void DrawSurface();
  void DrawPoints();
  void DrawVolume();
  void DrawOutline();
  
  // Description:
  // Methods called when item chosen from RepresentationMenu
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
  // Callback for the ResetColorRange button.
  void ResetColorRange();

  // Description:
  // Called when the user presses the "Edit Color Map" button.
  void EditColorMapCallback();

  // Description:
  // Called when the user presses the "Edit Volume Appearance" button.
  void EditVolumeAppearanceCallback();
  void ShowVolumeAppearanceEditor();
  
  void SetScalarBarVisibility(int val);  
  void ScalarBarCheckCallback();
  vtkGetObjectMacro(ScalarBarCheck, vtkKWCheckButton);

  void SetCubeAxesVisibility(int val);
  void CubeAxesCheckCallback();
  vtkGetObjectMacro(CubeAxesCheck, vtkKWCheckButton);

  void CenterCamera();
  
  // Description:
  // Save out the mapper and actor to a file.
  void SaveInBatchScript(ofstream *file);
  void SaveState(ofstream *file);

  // Description:
  // Callback for the change color button.
  void ChangeActorColor(double r, double g, double b);
  double* GetActorColor();
    
  //BTX
  // Description:
  // Get the name of the cube axes actor.
  vtkGetObjectMacro(CubeAxes, vtkCubeAxesActor2D);
  //ETX

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
  // This allows you to set the propertiesParent to any widget you like.  
  // If you do not specify a parent, then the views->PropertyParent is used.  
  // If the composite does not have a view, then a top level window is created.
  void SetPropertiesParent(vtkKWWidget *parent);
  vtkGetObjectMacro(PropertiesParent, vtkKWWidget);
  vtkKWWidget *PropertiesParent;

  // Description:
  // Returns true if CreateProperties() has been called.
  vtkGetMacro(PropertiesCreated, int);

  // Description:
  // Called by vtkPVSource::DeleteCallback().
  void DeleteCallback();
  
  // Description:
  // Convenience method for rendering.
  vtkPVRenderView* GetPVRenderView(); 

  // Description:
  // The source calls this to update the visibility check button,
  // and to determine whether the scalar bar should be visible.
  void SetVisibilityCheckState(int v);

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
  vtkGetObjectMacro(PVColorMap, vtkPVColorMap);

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
  vtkPVData();
  ~vtkPVData();

  int InstanceCount;
  
  // This points to the source widget that owns this data widget.
  vtkPVSource *PVSource;

  // If we are the last source to unregister a color map,
  // this method will turn its scalar bar visibility off.
  void SetPVColorMap(vtkPVColorMap *colorMap);

  void SetVolumeAppearanceEditor(vtkPVVolumeAppearanceEditor *appearanceEditor);
  
  //==================================================================
  // Internal versions that do not add to the trace.
  void ColorByPropertyInternal();
  void ColorByPointFieldInternal(const char *name, int numComps);
  void ColorByCellFieldInternal(const char *name, int numComps);
  void SetColorRangeInternal(double min, double max);
  void SetActorColor(double r, double g, double b);
  void VolumeRenderPointFieldInternal(const char *name);
  
  // A flag that helps UpdateProperties determine 
  // whether to set the default color.
  int ColorSetByUser;
  int ArraySetByUser;
  
  // Note properties does not mean the same thing as vtk.
  vtkKWFrame *Properties;
  vtkKWFrame *InformationFrame;
  vtkKWLabel *TypeLabel;
  vtkKWLabel *NumCellsLabel;
  vtkKWLabel *NumPointsLabel;
  vtkKWLabel *MemorySizeLabel;
  
  vtkKWBoundsDisplay *BoundsDisplay;
  vtkKWBoundsDisplay *ExtentDisplay;
  
  vtkKWScale *AmbientScale;
  
  vtkKWLabeledFrame *ColorFrame;
  vtkKWLabeledFrame *VolumeAppearanceFrame;
  vtkKWLabeledFrame *DisplayStyleFrame;
  vtkKWLabeledFrame *StatsFrame;
  vtkKWLabeledFrame *ViewFrame;
  
  vtkKWLabel *ColorMenuLabel;
  vtkKWOptionMenu *ColorMenu;

  vtkKWChangeColorButton *ColorButton;
  vtkKWPushButton *EditColorMapButton;

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
  // Need a separate value for visibility to properly manage 
  // color map "UseCount".
  int Visibility;
    
  // True if CreateProperties() has been called.
  int PropertiesCreated;

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
  vtkCubeAxesActor2D* CubeAxes;

  vtkKWPushButton *ResetCameraButton;

  double PreviousAmbient;
  double PreviousDiffuse;
  double PreviousSpecular;
  int PreviousWasSolid;

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
  // We have to remember when the current color attribute is from cell data.
  // InterpolateColors is not applicable for cell attriubtes we have
  // to know a cell attribute is current so that we do not enable the
  // interpolate button when map scalars is turned on.
  int CurrentColorAttributeIsFromCellData;

  vtkPVColorMap *PVColorMap;

  vtkPVVolumeAppearanceEditor *VolumeAppearanceEditor;
  
  // Adding point labelling back in.  This only works in single-process mode.
  // This code will be changed/moved when we rework 2D rendering in ParaView.
  vtkKWCheckButton *PointLabelCheck;
  
  void UpdatePropertiesInternal();
  void UpdateActorControlResolutions();
  void UpdateMapScalarsCheck(vtkPVDataSetAttributesInformation* info,
                              const char* name);

  vtkPVData(const vtkPVData&); // Not implemented
  void operator=(const vtkPVData&); // Not implemented
};

#endif
