/*=========================================================================

  Program:   ParaView
  Module:    vtkPVColorMap.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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


#include "vtkKWWidget.h"
#include "vtkClientServerID.h"
class vtkKWChangeColorButton;
class vtkKWCheckButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWLabeledEntry;
class vtkKWLabeledFrame;
class vtkKWMenuButton;
class vtkKWOptionMenu;
class vtkKWPushButton;
class vtkKWScale;
class vtkKWTextProperty;
class vtkKWWidget;
class vtkLookupTable;
class vtkPVApplication;
class vtkPVRenderView;
class vtkScalarBarWidget;
class vtkScalarBarWidgetObserver;

class VTK_EXPORT vtkPVColorMap : public vtkKWWidget
{
public:
  static vtkPVColorMap* New();
  vtkTypeRevisionMacro(vtkPVColorMap, vtkKWWidget);
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
  void SetScalarBarTitleInternal(const char* Name);
  const char* GetScalarBarTitle() {return this->ScalarBarTitle;}

  // Description:
  // This map is used for arrays with this name 
  // and this number of components.  In the future, they may
  // handle more than one type of array.
  void SetArrayName(const char* name);
  const char* GetArrayName() { return this->ArrayName;}
  int MatchArrayName(const char* name, int numberOfComponents);
  void SetNumberOfVectorComponents(int num);
  vtkGetMacro(NumberOfVectorComponents, int);

  // Description:
  // The format of the scalar bar labels.
  void SetScalarBarLabelFormat(const char* Name);
  vtkGetStringMacro(ScalarBarLabelFormat);

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
  // Sets the color range of all the mappers (all procs) and updates
  // the user interface as well.
  void SetScalarRange(float min, float max);
  void SetScalarRangeInternal(float min, float max);
  float *GetScalarRange() {return this->ScalarRange;}
      
  // Description:
  // Looks at all of the data object for a global range.
  // This also sets the color map to automatic.  In the future,
  // it will rescale to match changes in the global scalar range.
  void ResetScalarRange();
  void ResetScalarRangeInternal();
  
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
  void SetColorSchemeToRGBW();

  // Description:
  // Choose which component to color with.
  void SetVectorComponent(int component);
  vtkGetMacro(VectorComponent, int);

  // Description:
  // Save out the mapper and actor to a file.
  void SaveInBatchScript(ofstream *file);
    
  // Description:
  // The data needs to lookup table name to set the lookup table of the mapper.
  vtkGetMacro(LookupTableID,vtkClientServerID);

  // --- UI Stuff ---

  // Description:
  // Callbacks.
  void ScalarBarCheckCallback();
  void ColorRangeEntryCallback();

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
  void StartColorButtonCallback(float r, float g, float b);
  void EndColorButtonCallback(float r, float g, float b);
  void SetStartHSV(float h, float s, float v);
  void SetEndHSV(float h, float s, float v);

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
  vtkGetObjectMacro(TitleTextPropertyWidget, vtkKWTextProperty);
  vtkGetObjectMacro(LabelTextPropertyWidget, vtkKWTextProperty);

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

//BTX
  enum VectorModes {
    MAGNITUDE=0,
    COMPONENT=1
  };
//ETX

  // Description:
  // If the PVRenderView is set, render it
  virtual void RenderView();

protected:
  vtkPVColorMap();
  ~vtkPVColorMap();

  char* ArrayName;
  int NumberOfVectorComponents;
  char* ScalarBarTitle;
  char* ScalarBarLabelFormat;

  char *VectorMagnitudeTitle;
  char **VectorComponentTitles;
    
  // Here to create unique Tcl names.
  int InstanceCount;

  int NumberOfColors;
  float ScalarRange[2];
  int VectorMode;
  int VectorComponent;

  float StartHSV[3];
  float EndHSV[3];

  vtkScalarBarWidget* ScalarBar;
  vtkScalarBarWidgetObserver* ScalarBarObserver;

  void UpdateScalarBarTitle();
  void UpdateVectorComponentMenu();
  void UpdateScalarBarLabelFormat();
  void UpdateLookupTable();
  // Visibility depends on check and UseCount.
  void UpdateInternalScalarBarVisibility();
  void RGBToHSV(float rgb[3], float hsv[3]);
  void HSVToRGB(float hsv[3], float rgb[3]);

  vtkLookupTable* LookupTable;
  vtkClientServerID LookupTableID;

  vtkPVRenderView *PVRenderView;

  int Initialized;
  int ScalarBarVisibility;
  int InternalScalarBarVisibility;

  // User interaface.
  vtkKWLabeledFrame* ColorMapFrame;
  vtkKWLabel*        ArrayNameLabel;
  vtkKWScale*        NumberOfColorsScale;
  // Stuff for selecting start and end colors.
  vtkKWWidget*            ColorEditorFrame;
  vtkKWChangeColorButton* StartColorButton;
  vtkKWLabel*        Map;
  vtkKWChangeColorButton* EndColorButton;

  vtkKWLabeledFrame* VectorFrame;
  vtkKWOptionMenu*   VectorModeMenu;
  vtkKWOptionMenu*   VectorComponentMenu;
  vtkKWEntry*        ScalarBarVectorTitleEntry;

  vtkKWLabeledFrame* ScalarBarFrame;
  vtkKWCheckButton*  ScalarBarCheck;
  vtkKWWidget*       ScalarBarTitleFrame;
  vtkKWLabel*        ScalarBarTitleLabel;
  vtkKWEntry*        ScalarBarTitleEntry;
  vtkKWWidget*       ScalarBarLabelFormatFrame;
  vtkKWLabel*        ScalarBarLabelFormatLabel;
  vtkKWEntry*        ScalarBarLabelFormatEntry;
  
  vtkKWTextProperty *TitleTextPropertyWidget;
  vtkKWTextProperty *LabelTextPropertyWidget;
  
  // For the map image.
  unsigned char *MapData;
  int MapDataSize;
  int MapWidth;
  int MapHeight;
  void UpdateMap(int width, int height);

  vtkKWMenuButton* PresetsMenuButton;

  // Stuff for setting the range of the color map.
  vtkKWWidget*       ColorRangeFrame;
  vtkKWPushButton*   ColorRangeResetButton;
  vtkKWLabeledEntry* ColorRangeMinEntry;
  vtkKWLabeledEntry* ColorRangeMaxEntry;
  
  vtkKWPushButton*   BackButton;

  // For Saving into a tcl script.
  int VisitedFlag;

  // For determining how many data objects are using the color map.
  //  This is used to make the scalar bar invisible when not used.
  int UseCount;

  vtkPVColorMap(const vtkPVColorMap&); // Not implemented
  void operator=(const vtkPVColorMap&); // Not implemented
};

#endif
