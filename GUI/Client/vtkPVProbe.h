/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProbe.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVProbe - A class to handle the UI for vtkProbeFilter
// .SECTION Description


#ifndef __vtkPVProbe_h
#define __vtkPVProbe_h

#include "vtkPVSource.h"

class vtkKWCheckButton;
class vtkKWLabel;
class vtkKWOptionMenu;
class vtkKWWidget;
class vtkSMXYPlotDisplayProxy;

class VTK_EXPORT vtkPVProbe : public vtkPVSource
{
public:
  static vtkPVProbe* New();
  vtkTypeRevisionMacro(vtkPVProbe, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  void CreateProperties();

  // Description:
  // Access to the ShowXYPlotToggle from Tcl
  vtkGetObjectMacro(ShowXYPlotToggle, vtkKWCheckButton);

  // Description:
  // Control the visibility of the pick display as well.
  virtual void SetVisibilityNoTrace(int val);

  // Description:
  // Save the pipeline to a batch file which can be run without
  // a user interface.
  // Overridden to save the plot display in batch.
  virtual void SaveInBatchScript(ofstream *file);

protected:
  vtkPVProbe();
  ~vtkPVProbe();
  
  vtkSMXYPlotDisplayProxy* PlotDisplayProxy;
  char* PlotDisplayProxyName; // Name used to register the plot display proxy
                              // with the Proxy Manager.
  vtkSetStringMacro(PlotDisplayProxyName);
  
  // The real AcceptCallback method.
  virtual void AcceptCallbackInternal();  

  vtkKWLabel *DimensionalityLabel;
  vtkKWOptionMenu *DimensionalityMenu;
  vtkKWWidget *ProbeFrame;

  vtkKWWidget *SelectedPointFrame;
  vtkKWLabel *SelectedPointLabel;
  vtkKWLabel *PointDataLabel;
  
  vtkKWCheckButton *ShowXYPlotToggle;

  int CanShowPlot; // Flag indicating if the input is such that we can show 
    // the plot display.

private:
  vtkPVProbe(const vtkPVProbe&); // Not implemented
  void operator=(const vtkPVProbe&); // Not implemented
};

#endif
