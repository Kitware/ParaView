/*=========================================================================

  Program:   ParaView
  Module:    vtkPVColorMap.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVColorMap - Object to represent a color map of a parameter.
// .SECTION Description
// This object contains a global color represention for a parameter / unit.
// For the moment, I am keeping the minimal color map editor as
// part of the data object.  Multiple data objects may point to and
// edit this color map.
// .SECTION Note
// To remove actors fropm render view, please turn visilibty off before 
// deleting this object.

#ifndef __vtkPVColorMap_h
#define __vtkPVColorMap_h


#include "vtkPVTracedWidget.h"
#include "vtkClientServerID.h" // Needed for LookupTableID

class vtkKWChangeColorButton;
class vtkKWCheckButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWFrameLabeled;
class vtkKWMenuButton;
class vtkKWOptionMenu;
class vtkKWPushButton;
class vtkKWScale;
class vtkPVTextPropertyEditor;
class vtkPVApplication;
class vtkPVRenderView;
class vtkPVColorMapObserver;
class vtkPVSource;
class vtkKWRange;
class vtkTextProperty;
class vtkSMScalarBarWidgetProxy;
class vtkSMLookupTableProxy;
class vtkSMProxy;
class VTK_EXPORT vtkPVColorMap : public vtkPVTracedWidget
{
public:
  static vtkPVColorMap* New();
  vtkTypeRevisionMacro(vtkPVColorMap, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  vtkPVApplication *GetPVApplication();
  
  // Description:
  // Reference to the view is needed to display the scalar bar actor.
  void SetPVRenderView(vtkPVRenderView *view);
  vtkPVRenderView* GetPVRenderView() { return this->PVRenderView;}

  // Description:
  // The name of the color map serves as the label of the ScalarBar 
  // (e.g. Temperature). Currently it also indicates the arrays mapped
  // by this color map object.
  void SetScalarBarTitle(const char* Name);
  vtkGetStringMacro(ScalarBarTitle);

  // Description:
  // This map is used for arrays with this name 
  // and this number of components.  In the future, they may
  // handle more than one type of array.
  void SetArrayName(const char* name);
  const char* GetArrayName();
  int MatchArrayName(const char* name, int numberOfComponents);

  void SetNumberOfVectorComponents(int num);
  int GetNumberOfVectorComponents();


  // Description:
  // The format of the scalar bar labels.
  void SetScalarBarLabelFormat(const char* Name);
  const char* GetScalarBarLabelFormat();

  // Description:
  // Just like in vtk data objects, this method makes a data object
  // that is of the same type as the original.  It is used for creating
  // the output pvData in pvDataSetToDataSetFilters.
  virtual vtkPVColorMap *MakeObject()
    { vtkErrorMacro("No MakeObject"); return NULL;}
      
  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exist on all processes.
  void CreateParallelTclObjects(vtkPVApplication *pvApp);
                
  // Description:
  // Looks at all of the data object for a global range.
  // This also sets the color map to automatic.  In the future,
  // it will rescale to match changes in the global scalar range.
  void ResetScalarRange();
  void ResetScalarRangeInternal();
  void ResetScalarRangeInternal(vtkPVSource* source);
  
  // Descriptions:
  // Adds and removes scalar bar from renderer.
  void SetScalarBarVisibility(int val);
  int GetScalarBarVisibility() { return this->ScalarBarVisibility;}  

  // Descriptions:
  // Set the position, the size, and orientation of scalar bar.
  void SetScalarBarPosition1(float x, float y);
  void SetScalarBarPosition2(float x, float y);
  void SetScalarBarOrientation(int);
  
  // Description:
  // Choose preset color schemes.
  void SetColorSchemeToRedBlue();
  void SetColorSchemeToBlueRed();
  void SetColorSchemeToGrayscale();
  void SetColorSchemeToLabBlueRed();

  // Description:
  // Choose which component to color with.
  void SetVectorComponent(int component);
  vtkGetMacro(VectorComponent,int);

  // Description:
  // Save out the mapper and actor to a file.
  void SaveInBatchScript(ofstream *file);

  // --- UI Stuff ---

  // Description:
  // Callbacks.
  void ScalarBarCheckCallback();
  void ScalarRangeWidgetCallback();
  void LockCheckCallback();

  // Description:
  // Access for scripts.
  void SetScalarRangeLock(int val);

  // Description:
  // This method returns the user to the source page.
  // I would eventually like to replace this by 
  // a more general back/forward ParaView navigation.
  void BackButtonCallback();

  // Description:
  // This method is called when the user changes the name of the scalar bar.
  void ScalarBarTitleEntryCallback();

  // Description:
  // For setting the title suffix for vectors.
  void ScalarBarVectorTitleEntryCallback();
  void SetScalarBarVectorTitle(const char* name);

  // Description:
  // This method is called when the user changes the format of the scalar bar
  // labels.
  void ScalarBarLabelFormatEntryCallback();

  // Description:
  // Callbacks to change the color map.
  void StartColorButtonCallback(double r, double g, double b);
  void EndColorButtonCallback(double r, double g, double b);
  void SetStartHSV(double h, double s, double v);
  void SetEndHSV(double h, double s, double v);

  // Description:
  // Internal call used when the color map image changes shape.
  void MapConfigureCallback(int width, int height);

  // Description:
  // Called when the slider that select the resolution changes.
  void NumberOfColorsScaleCallback();
  void SetNumberOfColors(int num);

  // Description:
  // For internal use.
  // This is just a flag that is used to mark that the source has been saved
  // into the tcl script (visited) during the recursive saving process.
  vtkSetMacro(VisitedFlag,int);
  vtkGetMacro(VisitedFlag,int);

  // Description:
  // This method is called when event is triggered on the scalar bar.
//BTX
  virtual void ExecuteEvent(vtkObject* wdg, unsigned long event,  
                            void* calldata);
//ETX

  // Description:
  // GUI components access
  vtkGetObjectMacro(ScalarBarCheck, vtkKWCheckButton);
  vtkGetObjectMacro(TitleTextPropertyWidget, vtkPVTextPropertyEditor);
  vtkGetObjectMacro(LabelTextPropertyWidget, vtkPVTextPropertyEditor);

  // Call backs from the vector mode frame.
  void VectorModeMagnitudeCallback();
  void VectorModeComponentCallback();
  void VectorComponentCallback(int component);

  // Description:
  // Data objects use this to "register" their use of the map.
  // Scalar bar becomes invisible when use count reaches zero.
  void IncrementUseCount();
  void DecrementUseCount();

  // Description:
  // Save the state of the color map in the state file.
  void SaveState(ofstream *file);

  // Description:
  // If the PVRenderView is set, render it
  virtual void RenderView();

  // Description:
  // Call this when a source starts using this map, or
  // when the source changes.  It expands the whole range to include
  // the range of the source.
  void UpdateForSource(vtkPVSource* source);

  // Description:
  // Sets the whole range of color map slider.
  void SetWholeScalarRange(double min, double max);
  vtkGetVector2Macro(WholeScalarRange,double);

  // Description:
  // Sets the color range of all the mappers (all procs) and updates
  // the user interface as well.
  void SetScalarRange(double min, double max);
  void SetScalarRangeInternal(double min, double max);
  vtkGetVector2Macro(ScalarRange,double);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callback when the text property for the title is changed
  void TitleTextPropertyWidgetCallback();

  // Description:
  // Methods to modify the Title text property.
  virtual void SetTitleColor(double r, double g, double b);
  virtual void SetTitleOpacity(double opacity);
  virtual void SetTitleFontFamily(int font);
  virtual void SetTitleBold(int bold);
  virtual void SetTitleItalic(int italic);
  virtual void SetTitleShadow(int shadow); 
  
  // Description:
  // Callback when the text property for the labels is changed
  void LabelTextPropertyWidgetCallback();

  // Description:
  // Methods to modify the Labels text property.
  virtual void SetLabelColor(double r, double g, double b);
  virtual void SetLabelOpacity(double opacity);
  virtual void SetLabelFontFamily(int font);
  virtual void SetLabelBold(int bold);
  virtual void SetLabelItalic(int italic);
  virtual void SetLabelShadow(int shadow); 

  // Description:
  // Get proxies:
  // name = 
  //  LookupTable:- vtkSMLookupTableProxy
  //  ScalarBarWidget:- vtkSMScalarBarWidgetProxy
  vtkSMProxy* GetProxyByName(const char* name);

protected:
  vtkPVColorMap();
  ~vtkPVColorMap();

  vtkPVColorMapObserver* ScalarBarObserver;

  void InitializeObservers();
  void UpdateVectorComponentMenu();
  // Visibility depends on check and UseCount.
  void UpdateInternalScalarBarVisibility();
  void ComputeScalarRangeForSource(vtkPVSource* pvs, double* range);

  vtkPVRenderView *PVRenderView;

  int Initialized;
  int ScalarBarVisibility;
  int InternalScalarBarVisibility;
  int UserModifiedScalarRange;

  // Keep local ivars to avoid depending on widget values.
  double ScalarRange[2];
  // WholeScalarRange contain the maximums of the scalarRange widget.
  double WholeScalarRange[2];
  int ScalarRangeLock;
  
  // User interaface.
  vtkKWFrameLabeled* ColorMapFrame;
  vtkKWLabel*        ArrayNameLabel;
  // Stuff for setting the range of the color map.
  vtkKWWidget*       ScalarRangeFrame;
  vtkKWCheckButton*  ScalarRangeLockCheck;
  vtkKWRange*        ScalarRangeWidget;
  vtkKWScale*        NumberOfColorsScale;
  // Stuff for selecting start and end colors.
  vtkKWWidget*            ColorEditorFrame;
  vtkKWChangeColorButton* StartColorButton;
  vtkKWLabel*        Map;
  vtkKWChangeColorButton* EndColorButton;

  vtkKWFrameLabeled* VectorFrame;
  vtkKWOptionMenu*   VectorModeMenu;
  vtkKWOptionMenu*   VectorComponentMenu;
  vtkKWEntry*        ScalarBarVectorTitleEntry;

  vtkKWFrameLabeled* ScalarBarFrame;
  vtkKWCheckButton*  ScalarBarCheck;
  vtkKWWidget*       ScalarBarTitleFrame;
  vtkKWLabel*        ScalarBarTitleLabel;
  vtkKWEntry*        ScalarBarTitleEntry;
  vtkKWWidget*       ScalarBarLabelFormatFrame;
  vtkKWLabel*        ScalarBarLabelFormatLabel;
  vtkKWEntry*        ScalarBarLabelFormatEntry;
  
  vtkPVTextPropertyEditor *TitleTextPropertyWidget;
  vtkPVTextPropertyEditor *LabelTextPropertyWidget;
  
  // For the map image.
  unsigned char *MapData;
  int MapDataSize;
  int MapWidth;
  int MapHeight;
  void UpdateMap(int width, int height);
  void UpdateMap();

  vtkKWMenuButton* PresetsMenuButton;  
  vtkKWPushButton*   BackButton;

  vtkSMScalarBarWidgetProxy *ScalarBarProxy;
  char *ScalarBarProxyName;
  vtkSetStringMacro(ScalarBarProxyName);

  vtkSMLookupTableProxy* LookupTableProxy;
  char* LookupTableProxyName;
  vtkSetStringMacro(LookupTableProxyName);

  // TextProperty dummies for the label and title property
  vtkTextProperty* LabelTextProperty;
  vtkTextProperty* TitleTextProperty;

  // For creating a proper title for the scalar bar.
  char *ScalarBarTitle;
  char *ScalarBarVectorTitle;
  char *VectorMagnitudeTitle;
  char **VectorComponentTitles;
  int NumberOfVectorComponents;
  int VectorComponent;
  void UpdateScalarBarTitle();
  char* ArrayName; 


  // Description:
  // Get/Set the Title color from/to the Proxy
  void SetTitleColorInternal(double r, double g, double b);
 
  // Description:
  // Get/Set the Title opacity from/to the Proxy
  void SetTitleOpacityInternal(double opacity);

  // Description:
  // Get/Set teh Title font family from/to the Proxy
  void SetTitleFontFamilyInternal(int font);

  // Description:
  // Get/Set teh Title bold from/to the Proxy
  void SetTitleBoldInternal(int bold);

  // Description:
  // Get/Set teh Title italic from/to the Proxy
  void SetTitleItalicInternal(int italic);

  // Description:
  // Get/Set teh Title shadow from/to the Proxy
  void SetTitleShadowInternal(int shadow);

    // Description:
  // Get/Set the Label color from/to the Proxy
  void SetLabelColorInternal(double r, double g, double b);
 
  // Description:
  // Get/Set the Label opacity from/to the Proxy
  void SetLabelOpacityInternal(double opacity);

  // Description:
  // Get/Set teh Label font family from/to the Proxy
  void SetLabelFontFamilyInternal(int font);

  // Description:
  // Get/Set teh Label bold from/to the Proxy
  void SetLabelBoldInternal(int bold);

  // Description:
  // Get/Set teh Label italic from/to the Proxy
  void SetLabelItalicInternal(int italic);

  // Description:
  // Get/Set teh Label shadow from/to the Proxy
  void SetLabelShadowInternal(int shadow);

  // Description:
  // Get/Set the complete Title for the Proxy
  void SetTitleInternal(const char* name);
  
  // Description:
  // Get/Set the Vector mode from the Scalar bar Proxy
  int GetVectorModeInternal();
  void SetVectorModeInternal(int mode);

  // Description:
  // Get/Set the Label format from the Scalarbar Proxy
  const char* GetLabelFormatInternal();
  void SetLabelFormatInternal(const char* format);

  // Description:
  // Get/Set ArrayName from the Scalarbar Proxy
  const char* GetArrayNameInternal();
  void SetArrayNameInternal(const char* name);

  // Description:
  // Get/Set Hue/Saturation/Value ranges for the lookuptable
  void GetHueRangeInternal(double range[2]);
  void GetSaturationRangeInternal(double range[2]);
  void GetValueRangeInternal(double range[2]);
  void SetHSVRangesInternal(double hrange[2],
    double srange[2], double vrange[2]);
  //void SetSaturationRangeInternal(double range[2]);
  //void SetValueRangeInternal(double range[2]);
 
  // Description:
  // Set the number of colors for the Proxy
  void SetNumberOfColorsInternal(int num);
  int GetNumberOfColorsInternal();
  
  // Description:
  // Get the vector mode from the property of the ScalarBarProxy
  void GetLabelTextPropertyInternal();
  void GetTitleTextPropertyInternal();
  
  void SetVisibilityInternal(int visible);
  void SetVectorComponentInternal(int component);

  // Description:
  // Get/Set the positions for the scalar bar Proxy
  // ScalarBarProxy->UpdateInformation() must be called
  // before calling the Get method.
  void SetPosition1Internal(double x, double y);
  void GetPosition1Internal(double pos[2]);
  
  // Description:
  // Get/Set the positions for the scalar bar Proxy
  // ScalarBarProxy->UpdateInformation() must be called
  // before calling the Get method.
  void SetPosition2Internal(double x, double y);
  void GetPosition2Internal(double pos[2]);
 
  // Description:
  // Get/Set the orientation for the Proxy
  // ScalarBarProxy->UpdateInformation() must be called
  // before calling the Get method
  void SetOrientationInternal(int orientation);
  int GetOrientationInternal();

  // Leaving this name for this function for the timebeing
  // till we can move the code from SetScalarRangeInternal
  // somewhere. All ...Internal methods push values onto 
  // proxies.
  void SetScalarBarWidgetScalarRangeInternal(double min, double max);
  
  // For Saving into a tcl script.
  int VisitedFlag;

  // For determining how many data objects are using the color map.
  //  This is used to make the scalar bar invisible when not used.
  int UseCount;
private:
  vtkPVColorMap(const vtkPVColorMap&); // Not implemented
  void operator=(const vtkPVColorMap&); // Not implemented
};

#endif
