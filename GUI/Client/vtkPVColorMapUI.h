/*=========================================================================

  Program:   ParaView
  Module:    vtkPVColorMapUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVColorMapUI - UI for color maps
// .SECTION Description
// This object contains a the user interface code for vtkPVColorMaps.

#ifndef __vtkPVColorMapUI_h
#define __vtkPVColorMapUI_h

#include "vtkPVTracedWidget.h"

class vtkKWChangeColorButton;
class vtkKWCheckButton;
class vtkKWEntry;
class vtkKWFrameWithLabel;
class vtkKWLabel;
class vtkKWMenuButton;
class vtkKWMenuButtonWithLabel;
class vtkKWRange;
class vtkKWScaleWithEntry;
class vtkKWThumbWheel;
class vtkPVColorMap;
class vtkPVTextPropertyEditor;
class vtkPVWindow;

class VTK_EXPORT vtkPVColorMapUI : public vtkPVTracedWidget
{
public:
  static vtkPVColorMapUI* New();
  vtkTypeRevisionMacro(vtkPVColorMapUI, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provide access ot the title and label text property widgets.
  vtkGetObjectMacro(TitleTextPropertyWidget, vtkPVTextPropertyEditor);
  vtkGetObjectMacro(LabelTextPropertyWidget, vtkPVTextPropertyEditor);

  // Description:
  // Callbacks for the widgets used in this UI
  void VisibilityCheckCallback(int state);
  void DefaultCheckCallback(int state);
  void VectorModeMagnitudeCallback();
  void VectorModeComponentCallback();
  void LockCheckCallback(int state);
  void ScalarRangeWidgetCallback(double r0, double r1);
  void StartColorButtonCallback(double r, double g, double b);
  void MapConfigureCallback(int width, int height);
  void EndColorButtonCallback(double r, double g, double b);
  void SetColorSchemeToBlueRed();
  void SetColorSchemeToRedBlue();
  void SetColorSchemeToGrayscale();
  void SetColorSchemeToLabBlueRed();
  void NumberOfColorsScaleCallback(double value);
  void OutOfRangeCheckCallback(int state);
  void LowColorButtonCallback(double r, double g, double b);
  void HighColorButtonCallback(double r, double g, double b);
  void ScalarBarTitleEntryCallback();
  void ScalarBarVectorTitleEntryCallback();
  void TitleTextPropertyWidgetCallback();
  void ScalarBarLabelFormatEntryCallback();
  void LabelTextPropertyWidgetCallback();
  void VectorComponentMenuCallback(int idx);
  void NumberOfLabelsThumbWheelCallback(int num);

  // Description:
  // Update the list of arrays in the Parameter menu
  void UpdateParameterList(vtkPVWindow *win);

  // Description:
  // Update the widgets when the selected parameter changes
  void UpdateColorMapUI(const char *name, int numComponents, int field);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  void UpdateEnableState();

  // Description:
  // Get a pointer to the vtkPVColorMap whose scalar bar is being controlled.
  vtkGetObjectMacro(CurrentColorMap, vtkPVColorMap);

protected:
  vtkPVColorMapUI();
  ~vtkPVColorMapUI();

  // Description:
  // Create the widget
  void CreateWidget();

  vtkPVColorMap *CurrentColorMap;
  void SetCurrentColorMap(vtkPVColorMap *colorMap);

  vtkKWFrameWithLabel *ScalarColorBarFrame;
  vtkKWCheckButton *VisibilityCheck;
  vtkKWCheckButton *DefaultCheck;
  vtkKWFrame *ParameterFrame;
  vtkKWLabel *ParameterLabel;
  vtkKWMenuButton *ParameterMenu;
  vtkKWLabel *VectorLabel;
  vtkKWMenuButton *VectorModeMenu;
  vtkKWMenuButton *VectorComponentMenu;
  vtkKWFrame *ScalarRangeFrame;
  vtkKWCheckButton *ScalarRangeLockCheck;
  vtkKWRange *ScalarRangeWidget;
  vtkKWScaleWithEntry *NumberOfColorsScale;
  vtkKWFrame *ColorEditorFrame;
  vtkKWMenuButton *PresetsMenuButton; 
  vtkKWChangeColorButton *StartColorButton;
  vtkKWLabel *Map;
  vtkKWChangeColorButton *EndColorButton;
  vtkKWFrame *OutOfRangeFrame;
  vtkKWLabel *OutOfRangeLabel;
  vtkKWCheckButton *OutOfRangeCheck;
  vtkKWLabel *LowColorLabel;
  vtkKWChangeColorButton *LowColorButton;
  vtkKWLabel *HighColorLabel;
  vtkKWChangeColorButton *HighColorButton;
  vtkKWFrame *ScalarBarTitleFrame;
  vtkKWLabel *ScalarBarTitleLabel;
  vtkKWEntry *ScalarBarTitleEntry;
  vtkKWEntry *ScalarBarVectorTitleEntry;
  vtkPVTextPropertyEditor *TitleTextPropertyWidget;
  vtkKWFrame *ScalarBarLabelFormatFrame;
  vtkKWLabel *ScalarBarLabelFormatLabel;
  vtkKWEntry *ScalarBarLabelFormatEntry;
  vtkPVTextPropertyEditor *LabelTextPropertyWidget;
  vtkKWThumbWheel *NumberOfLabelsThumbWheel;

  void UpdateMapFromCurrentColorMap();
  void UpdateScalarBarTitle();
  void UpdateVectorComponentMenu();
  const char* ConvertVectorModeToString(int mode);
  void UpdateScalarRangeWidget();
  void UpdateOutOfRangeColors();

private:
  vtkPVColorMapUI(const vtkPVColorMapUI&); // Not implemented
  void operator=(const vtkPVColorMapUI&); // Not implemented
};

#endif
