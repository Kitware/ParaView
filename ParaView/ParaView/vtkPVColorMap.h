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

class vtkKWChangeColorButton;
class vtkKWCheckButton;
class vtkKWImageLabel;
class vtkKWLabel;
class vtkKWLabeledEntry;
class vtkKWLabeledFrame;
class vtkKWMenu;
class vtkKWOptionMenu;
class vtkKWPushButton;
class vtkKWScale;
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
  // The name of the color map serves as the label of the ScalarBar (e.g. Temperature).
  // Currently it also indicates the arrays mapped by this color map object.
  void SetScalarBarTitle(const char* Name);
  void SetScalarBarTitleNoTrace(const char* Name);
  const char* GetScalarBarTitle() {return this->ScalarBarTitle;}

  // Description:
  // The single (for now) parameter that is handled by this map.
  void SetArrayName(const char* name);
  const char* GetArrayName() { return this->ArrayName;}
  int MatchArrayName(const char* name);

  // Description:
  // Just like in vtk data objects, this method makes a data object
  // that is of the same type as the original.  It is used for creating
  // the output pvData in pvDataSetToDataSetFilters.
  virtual vtkPVColorMap *MakeObject(){vtkErrorMacro("No MakeObject");return NULL;}
      
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
  
  // Descriptions:
  // Adds and removes scalar bar from renderer.
  void SetScalarBarVisibility(int val);
  int GetScalarBarVisibility() { return this->ScalarBarVisibility;}  
  
  // Description:
  // Choose preset color schemes.
  void SetColorSchemeToRedBlue();
  void SetColorSchemeToBlueRed();
  void SetColorSchemeToGrayscale();

  // Description:
  // Choose which component to color with.
  void SetVectorComponent(int component, int numberOfComponents);
  vtkGetMacro(VectorComponent, int);

  // Description:
  // Save out the mapper and actor to a file.
  void SaveInTclScript(ofstream *file);
    
  // Description:
  // The data needs to lookup table name to set the lookup table of the mapper.
  vtkGetStringMacro(LookupTableTclName);

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
  void NameEntryCallback();

  // Description:
  // Callbacks to change the color map.
  void StartColorButtonCallback(float r, float g, float b);
  void EndColorButtonCallback(float r, float g, float b);

  // Description:
  // Internal call used when the color map image changes shape.
  void MapConfigureCallback(int width, int height);

  // Description:
  // Called when user select the map.
  void DisplayPopupMenu(int x, int y);

  // Description:
  // Called when the slider that select the resolution changes.
  void NumberOfColorsScaleCallback();

  // Description:
  // This method is called when event is triggered on the scalar bar.
  //BTX
  virtual void ExecuteEvent(vtkObject* wdg, unsigned long event,  
                            void* calldata);
  //ETX

protected:
  vtkPVColorMap();
  ~vtkPVColorMap();

  char* ArrayName;
  char* ScalarBarTitle;
    
  // Here to create unique Tcl names.
  int InstanceCount;

  int NumberOfColors;
  float ScalarRange[2];
  int VectorComponent;
  int NumberOfVectorComponents;

  float StartHSV[3];
  float EndHSV[3];

  vtkScalarBarWidget* ScalarBar;
  vtkScalarBarWidgetObserver* ScalarBarObserver;

  void UpdateScalarBarTitle();
  void UpdateLookupTable();
  void RGBToHSV(float rgb[3], float hsv[3]);

  vtkLookupTable* LookupTable;
  char* LookupTableTclName;
  vtkSetStringMacro(LookupTableTclName);

  vtkPVRenderView *PVRenderView;
  int Initialized;
  int ScalarBarVisibility;

  // User interaface.
  vtkKWLabeledFrame* ColorMapFrame;
  vtkKWLabel*        ArrayNameLabel;
  vtkKWScale*        NumberOfColorsScale;
  // Stuff for selecting start and end colors.
  vtkKWWidget*            ColorEditorFrame;
  vtkKWChangeColorButton* StartColorButton;
  vtkKWImageLabel*        Map;
  vtkKWChangeColorButton* EndColorButton;

  vtkKWLabeledFrame* ScalarBarFrame;
  vtkKWLabeledEntry* ScalarBarTitleEntry;
  vtkKWWidget*       ScalarBarCheckFrame;
  vtkKWCheckButton*  ScalarBarCheck;


  
  // For the map image.
  unsigned char *MapData;
  int MapDataSize;
  int MapWidth;
  int MapHeight;
  void UpdateMap(int width, int height);

  vtkKWMenu* PopupMenu;

  // Stuff for setting the range of the color map.
  vtkKWWidget*       ColorRangeFrame;
  vtkKWPushButton*   ColorRangeResetButton;
  vtkKWLabeledEntry* ColorRangeMinEntry;
  vtkKWLabeledEntry* ColorRangeMaxEntry;
  
  vtkKWPushButton*   BackButton;

  vtkPVColorMap(const vtkPVColorMap&); // Not implemented
  void operator=(const vtkPVColorMap&); // Not implemented
};

#endif
