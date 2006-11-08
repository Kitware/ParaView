/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPick.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPick - ParaViews client side control for the vtkPickFilter.
// .SECTION Description
// 

#ifndef __vtkPVPick_h
#define __vtkPVPick_h

#include "vtkPVSource.h"

class vtkSMPointLabelDisplayProxy;

class vtkCollection;
class vtkKWFrame;
class vtkKWLabel;
class vtkDataSetAttributes;
class vtkKWFrameWithLabel;
class vtkKWCheckButton;
class vtkKWThumbWheel;
class vtkPVArraySelection;
class vtkSMXYPlotDisplayProxy;
class vtkSMProxy;
class vtkTemporalPickObserver;
class vtkKWLoadSaveButton;

class VTK_EXPORT vtkPVPick : public vtkPVSource
{
public:
  static vtkPVPick* New();
  vtkTypeRevisionMacro(vtkPVPick, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  virtual void CreateProperties();

  // Description:
  // Called when the delete button is pressed.
  virtual void DeleteCallback();

  // Description:
  // Called by GUI to change Point Label appearance
  void PointLabelCheckCallback(int state);
  void ChangePointLabelFontSizeCallback(double value);

  // Description:
  // Refreshes GUI with current Point Label appearance
  void UpdatePointLabelCheck();
  void UpdatePointLabelFontSize();

  // Description:
  // Save the pipeline to a batch file which can be run without
  // a user interface.
  // Overridden to save the plot display in batch.
  virtual void SaveInBatchScript(ofstream *file);
  virtual void SaveState(ofstream *file);

  // Description:
  // Called when scalars are selected or deselected for the plot.
  void ArraySelectionInternalCallback();

  // Description:
  // Callback for the Save as comma separated values button.
  void SaveDialogCallback();

  // Description:
  // Access to the ShowXYPlotToggle from Tcl
  vtkGetObjectMacro(ShowXYPlotToggle, vtkKWCheckButton);

  // Description:
  // Attempts to find the real time from the source.
  bool GetSourceTimeNow(double &TimeNow);

  void UpdateGUI();

protected:
  vtkPVPick();
  ~vtkPVPick();

  vtkKWFrame *DataFrame;
  vtkCollection* LabelCollection;

  virtual void Select();
  void ClearDataLabels();
  void InsertDataLabel(const char* label, vtkIdType idx,
                       vtkDataSetAttributes* attr, double* x=0);
  int LabelRow;

  // The real AcceptCallback method.
  virtual void AcceptCallbackInternal();  

  // Point label controls
  vtkKWFrameWithLabel *PointLabelFrame;
  vtkKWCheckButton *PointLabelCheck;
  vtkKWLabel       *PointLabelFontSizeLabel;
  vtkKWThumbWheel  *PointLabelFontSizeThumbWheel;

  // Added for temporal plot
  vtkKWFrameWithLabel *XYPlotFrame;
  vtkKWCheckButton *ShowXYPlotToggle;
  vtkPVArraySelection *ArraySelection;
  vtkSMXYPlotDisplayProxy* PlotDisplayProxy;
  char* PlotDisplayProxyName; 
  vtkSetStringMacro(PlotDisplayProxyName);
  vtkSMProxy* TemporalPickProxy;
  char* TemporalPickProxyName; 
  vtkSetStringMacro(TemporalPickProxyName);
  vtkTemporalPickObserver *Observer;
  vtkKWLoadSaveButton *SaveButton;
  int LastPorC;
  int LastUseId;

  virtual void SaveTemporalPickInBatchScript(ofstream *file);

private:
  vtkPVPick(const vtkPVPick&); // Not implemented
  void operator=(const vtkPVPick&); // Not implemented
};

#endif
