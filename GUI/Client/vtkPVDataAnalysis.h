/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataAnalysis.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDataAnalysis
// .SECTION Description

#ifndef __vtkPVDataAnalysis_h
#define __vtkPVDataAnalysis_h

#include "vtkPVSource.h"

class vtkFieldData;
class vtkKWCheckButton;
class vtkKWEntry;
class vtkKWEntryWithLabel;
class vtkKWFrame;
class vtkKWFrameWithLabel;
class vtkKWLabel;
class vtkKWLoadSaveButton;
class vtkKWMenuButton;
class vtkKWMultiColumnListWithScrollbars;
class vtkKWPushButton;
class vtkKWRange;
class vtkPVPlotArraySelection;
class vtkPVDataAnalysisObserver;
class vtkPVPlotDisplayLabelPropertiesDialog;
class vtkPVReaderModule;
class vtkSMProxy;
class vtkSMTemporalXYPlotDisplayProxy;

class VTK_EXPORT vtkPVDataAnalysis : public vtkPVSource
{
public:
  static vtkPVDataAnalysis* New();
  vtkTypeRevisionMacro(vtkPVDataAnalysis, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set up the UI for this source
  void CreateProperties();

  // Description:
  // This method gets called to set the VTK source parameters
  // from the widget values.
  virtual void UpdateVTKSourceParameters();

  // Description:
  // Internal method; called by AcceptCallbackInternal.
  // Hide flag is used for hiding creation of 
  // the glyph sources from the user.
  virtual void Accept() { this->Superclass::Accept();}
  virtual void Accept(int hideFlag) { this->Superclass::Accept(hideFlag, 0); }
  virtual void Accept(int hideFlag, int vtkNotUsed(hideSource))
    { this->Superclass::Accept(hideFlag, 0); }

  // Description:
  // Internal callback method. Called on a request to Save CSV.
  void SaveDialogCallback();

  // Description:
  // Internal callback method. Called when the array selection is
  // changed.
  void PointArraySelectionModifiedCallback();
  void CellArraySelectionModifiedCallback();

  // Description:
  // Internal callback, called when PlotOverTimeCheckButton is toggled.
  void PlotOverTimeCheckButtonCallback();
  void SetPlotOverTime(int state);
  int GetPlotOverTime();

  // Description:
  void LockTemporalCacheCheckButtonCallback();
  void SetLockTemporalCache(int state);

  // Description:
  // Set Plot title.
  void PlotTitleCallback();
  void SetPlotTitle(const char* str);

  // Description:
  // Set X/Y Axis Label.
  void XAxisLabelCallback();
  void SetXAxisLabel(const char* str);
  void YAxisLabelCallback();
  void SetYAxisLabel(const char* str);

  // Description:
  // Set legend X/Y positions.
  void SetLegendPosition(double pos[2]) { this->SetLegendPosition(pos[0], pos[1]); }
  void SetLegendPosition(double x, double y);

  // Description:
  // Internal Callback. Called when legend position entries are changed.
  void LegendPositionCallback();

  // Description:
  // Set plot legend visibility.
  void LegendVisibilityCallback();
  void SetLegendVisibility(int visible);

  // Description:
  // Set the visibility of the Plot Display.
  void SetPlotDisplayVisibility(int state);

  // Description:
  // Propagate enabled state to the widgets.
  virtual void UpdateEnableState();

  // Description:
  // Triggers the generate of temporal plot.
  void GenerateTemporalPlot();

  // Description:
  // Aborts GenerateTemporalPlot.
  void AbortGenerateTemporalPlot();

  // Description:
  // This method is called when any of the PVSources upstream (or this PVSource
  // itself) are changed. We override this method, to ensure that the 
  // Temporal parameters are up-to-date. Since, if the input any where
  // up the pipeline changed, then it is possible that this probe now
  // connected to a source that does not have time support
  // and hence, no temporal plotting.
  virtual void MarkSourcesForUpdate();

  // Description:
  // Control the visibility of the plot display as well.
  virtual void SetVisibilityNoTrace(int val);

  void EditXLabelCallback(int popup_dialog);
  void EditYLabelCallback(int popup_dialog);

  // Description:
  // Set the plot type to plot lines/points/both.
  void SetPlotTypeToPoints();
  void SetPlotTypeToLines();
  void SetPlotTypeToPointsAndLines();
  void SetPlotType(int plot_lines, int plot_points);

  // Callbacks for PlotTitle position.
  void SetPlotTitlePosition(double x, double y);
  void SetPlotTitlePositionCallback(const char*);
  void SetAdjustTitlePosition(int state);
  void AdjustTitlePositionCallback();

  // Description:
  // Trace access to the internal widgets properties dialog.
  vtkGetObjectMacro(LabelPropertiesDialog, 
    vtkPVPlotDisplayLabelPropertiesDialog);
  vtkGetObjectMacro(PointArraySelection, vtkPVPlotArraySelection);
  vtkGetObjectMacro(CellArraySelection, vtkPVPlotArraySelection);
  
  // Description:
  // Saves the pipeline in a ParaView script.  This is similar
  // to saveing a trace, except only the last state is stored.
  virtual void SaveState(ofstream *file);

  // Description:
  // Overridden to save the plot display to batch.
  virtual void SaveInBatchScript(ofstream *file);

  // Description:;
  // Get the plot display used by this source.
  vtkGetObjectMacro(PlotDisplayProxy, vtkSMTemporalXYPlotDisplayProxy);

  // Plot display properties such as XTitle, YTitle 
  // may be changed by the XYPlotActor during rendering. Hence, we need
  // to update the GUI so show the correct values.
  void UpdatePlotDisplayGUI();
protected:
  vtkPVDataAnalysis();
  ~vtkPVDataAnalysis();

  // The real AcceptCallback method.
  virtual void AcceptCallbackInternal();  

  // Since all properties of vtkDataAnalysisFilter proxy are not exposed
  // as GUI widgets, the superclass does not write all properties.
  virtual void SaveWidgetsInBatchScript(ofstream* file);
  
  // Description:
  // Setups up the displays for this source. Overridden to create/cleanup
  // the plot display.
  virtual void SetupDisplays();
  virtual void CleanupDisplays();

  // Description:
  // Determines if temporal support can be provided for the given
  // input and updates the gui.
  void InitializeTemporalSupport();

  void UpdateDataInformationList();
  void AppendData(int point_data, int index, vtkFieldData* fieldData);
 
  // Description:
  // Check is the selection widget needs to be initialized again.
  // This is necessary when the list of arrays available at the input
  // changes.
  void CheckAndUpdateArraySelections(vtkPVPlotArraySelection*);

  // Description:
  // Locate source with time support.
  vtkPVReaderModule* LocateUpstreamSourceWithTimeSupport();

  void SetPlotDisplayVisibilityInternal(int state);

  // The plot display proxy.
  vtkSMTemporalXYPlotDisplayProxy* PlotDisplayProxy;
  char* PlotDisplayProxyName;
  vtkSetStringMacro(PlotDisplayProxyName);

  vtkSMProxy* AnimationCueProxy;
  vtkSMProxy* AnimationManipulatorProxy;

  // Frame that shows the data information, when small enough to show.
  vtkKWFrameWithLabel* DataInformationFrame;
  
  vtkKWMultiColumnListWithScrollbars* DataInformationList;
  
  // Frame that contains all the widgets dealing with plotting.
  vtkKWFrameWithLabel *PlotParametersFrame;


  // Check box to show/hide the XY plot.
  vtkKWCheckButton *ShowXYPlotCheckButton;

  // Check box to enable/disable temporal plotting.
  vtkKWCheckButton *PlotOverTimeCheckButton;

  // Button to save plot data as CSV.
  vtkKWLoadSaveButton *SaveCSVButton;

  // Container Frame for array selection widgets.
  vtkKWFrame* ArraySelectionFrame;
  
  // The selection is used to select the point/cell data arrays 
  // to be used for plotting.
  vtkPVPlotArraySelection *PointArraySelection;
  vtkPVPlotArraySelection *CellArraySelection;

  // Frame for temporal plot generation.
  vtkKWFrameWithLabel *TemporalParametersFrame; 
  vtkKWLabel*   SourceNameLabelLabel;
  vtkKWLabel*   SourceNameLabel;
  vtkKWLabel*   RangeLabel;
  vtkKWRange*   Range;
  vtkKWPushButton* GenerateButton;
  vtkKWCheckButton* LockTemporalCacheCheckButton;

  // Frame collecting Plot display properties.
  vtkKWFrameWithLabel *PlotDisplayPropertiesFrame;
  vtkKWLabel*   PlotTitleLabel;
  vtkKWFrame*   PlotTitleFrame;
  vtkKWEntry*   PlotTitleEntry;
  vtkKWEntryWithLabel* PlotTitleXPositionWidget;
  vtkKWEntryWithLabel* PlotTitleYPositionWidget;
  vtkKWCheckButton* AdjustTitlePositionCheckButton;

  vtkKWLabel*   XLabelLabel;
  vtkKWEntry*   XLabelEntry;
  vtkKWLabel*   YLabelLabel;
  vtkKWEntry*   YLabelEntry;
  vtkKWPushButton* XLabelEditButton;
  vtkKWPushButton* YLabelEditButton;

  vtkKWLabel*   LegendLabel;
  vtkKWCheckButton* ShowLegendCheckButton;
  vtkKWEntryWithLabel* LegendXPositionWidget;
  vtkKWEntryWithLabel* LegendYPositionWidget;
  vtkKWLabel*   PlotTypeLabel;
  vtkKWMenuButton* PlotTypeMenuButton;
 
  vtkPVPlotDisplayLabelPropertiesDialog* LabelPropertiesDialog;
  int PlottingPointData;
  int TimeSupportAvailable;

  // When query method changes, we need to uncheck the temporal "Lock"
  // if it is set.
  char* LastAcceptedQueryMethod;
  vtkSetStringMacro(LastAcceptedQueryMethod);
private:
  vtkPVDataAnalysis(const vtkPVDataAnalysis&); // Not implemented.
  void operator=(const vtkPVDataAnalysis&); // Not implemented.

//BTX
  vtkPVDataAnalysisObserver* Observer;
//ETX
};


#endif


