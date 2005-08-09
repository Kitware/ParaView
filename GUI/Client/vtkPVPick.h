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
// .NAME vtkPVPick - A special PVSource.
// .SECTION Description
// This class will set up defaults for thePVData.
// It will also create a special PointLabelDisplay.
// Both of these features should be specified in XML so we
// can get rid of this special case.


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

  void PointLabelCheckCallback();
  void ChangePointLabelFontSize();

  void UpdatePointLabelCheck();
  void UpdatePointLabelFontSize();

protected:
  vtkPVPick();
  ~vtkPVPick();

  vtkKWFrame *DataFrame;
  vtkCollection* LabelCollection;

  virtual void Select();
  void UpdateGUI();
  void ClearDataLabels();
  void InsertDataLabel(const char* label, vtkIdType idx,
                       vtkDataSetAttributes* attr, double* x=0);
  int LabelRow;

  // The real AcceptCallback method.
  virtual void AcceptCallbackInternal();  

  vtkKWFrameWithLabel *PointLabelFrame;
  vtkKWCheckButton *PointLabelCheck;
  vtkKWLabel       *PointLabelFontSizeLabel;
  vtkKWThumbWheel  *PointLabelFontSizeThumbWheel;

  //FOR TEMPORAL PLOT
  /*
  vtkSMXYPlotDisplayProxy* PlotDisplayProxy;
  char* PlotDisplayProxyName; // Name used to register the plot display proxy
                              // with the Proxy Manager.
  vtkSetStringMacro(PlotDisplayProxyName);
  vtkPVArraySelection *ArraySelection;
  */

private:
  vtkPVPick(const vtkPVPick&); // Not implemented
  void operator=(const vtkPVPick&); // Not implemented
};

#endif
