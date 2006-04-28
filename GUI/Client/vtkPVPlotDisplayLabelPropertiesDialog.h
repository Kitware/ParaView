/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlotDisplayLabelPropertiesDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPlotDisplayLabelPropertiesDialog
// .SECTION Description
// This is the pop-up dialog used to set the label properties for a plot display,
// such as number of ticks, num of labels etc.
//


#ifndef __vtkPVPlotDisplayLabelPropertiesDialog_h
#define __vtkPVPlotDisplayLabelPropertiesDialog_h

#include "vtkKWDialog.h"

class vtkKWCheckButton;
class vtkKWEntry;
class vtkKWEntryWithLabel;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWPushButton;
class vtkKWScaleWithEntry;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
class vtkSMProxy;
class vtkSMStringVectorProperty;
class vtkPVApplication;
class vtkPVTraceHelper;

class VTK_EXPORT vtkPVPlotDisplayLabelPropertiesDialog : public vtkKWDialog
{
public:
  static vtkPVPlotDisplayLabelPropertiesDialog* New();
  vtkTypeRevisionMacro(vtkPVPlotDisplayLabelPropertiesDialog, vtkKWDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the PVApplication pointer.
  vtkPVApplication* GetPVApplication();

  // Description:
  // vtkKWDialog overrides.
  virtual void OK();
  virtual int Invoke();
  virtual void Cancel();

  // Description:
  // Pushes the GUI values to the properties or vice-versa..
  void PushWidgetValues(int from_gui);

  // Description:
  // Method to set property values. Used in trace.
  void SetTitlePosition(double pos);
  void SetLabelFormat(const char* str);
  void SetNumberOfLabels(int num);
  void SetAutoAdjust(int state);
  void SetDataRangeAuto(int state);
  void SetDataRange(double min, double max);
  void SetNumberOfMinorTicks(int num);
  // callback.
  void SetTitlePositionCallback(const char* );
  void SetDataRangeCallback(const char*);
  
  // Description:
  // Set appropriate properties.
  void SetLabelFormatProperty(vtkSMStringVectorProperty*);
  void SetNumberOfLabelsProperty(vtkSMIntVectorProperty*);
  void SetAutoAdjustProperty(vtkSMIntVectorProperty*);
  void SetNumberOfMinorTicksProperty(vtkSMIntVectorProperty*);
  void SetTitlePositionProperty(vtkSMDoubleVectorProperty*);
  void SetDataRangeProperty(vtkSMDoubleVectorProperty*);
  void SetPlotDisplayProxy(vtkSMProxy*);
    

  // Description:
  // API to set the text displayed in the TitlePositionLabel.
  void SetPositionLabelText(const char* txt);

  // Description:
  // Get the trace helper object.
  vtkGetObjectMacro(TraceHelper, vtkPVTraceHelper)

protected:
  vtkPVPlotDisplayLabelPropertiesDialog();
  ~vtkPVPlotDisplayLabelPropertiesDialog();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkKWFrame* FrameForGrid;
  vtkKWLabel* TitlePositionLabel;
  vtkKWEntry* TitlePositionEntry;
  vtkKWLabel* DataRangeLabel;
  vtkKWEntryWithLabel* DataRangeMinWidget;
  vtkKWEntryWithLabel* DataRangeMaxWidget;
  vtkKWCheckButton* DataRangeAutoCheckButton;
  vtkKWLabel* LabelFormatLabel;
  vtkKWEntry* LabelFormatEntry;
  vtkKWLabel* NumberOfLabelsLabel;
  vtkKWScaleWithEntry* NumberOfLabelsScale;
  vtkKWCheckButton* AutoAdjustCheckButton;
  vtkKWLabel* NumberOfMinorTicksLabel;
  vtkKWScaleWithEntry* NumberOfMinorTicksScale;

  vtkKWPushButton* OKButton;
  vtkKWPushButton* CancelButton;

  vtkSMStringVectorProperty* LabelFormatProperty;
  vtkSMIntVectorProperty* NumberOfLabelsProperty;
  vtkSMIntVectorProperty* AutoAdjustProperty;
  vtkSMIntVectorProperty* NumberOfMinorTicksProperty;
  vtkSMDoubleVectorProperty* TitlePositionProperty;
  vtkSMDoubleVectorProperty* DataRangeProperty;
  vtkSMProxy* PlotDisplayProxy;
   
  vtkPVTraceHelper* TraceHelper;
private:
  vtkPVPlotDisplayLabelPropertiesDialog(const vtkPVPlotDisplayLabelPropertiesDialog&); // Not implemented.
  void operator=(const vtkPVPlotDisplayLabelPropertiesDialog&); // Not implemented.
};


#endif


