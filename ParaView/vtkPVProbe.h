/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProbe.h
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
// .NAME vtkPVProbe - A class to handle the UI for vtkProbeFilter
// .SECTION Description


#ifndef __vtkPVProbe_h
#define __vtkPVProbe_h

#include "vtkPVSource.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWRadioButton.h"
#include "vtkKWSelectPointInteractor.h"
#include "vtkKWLabeledEntry.h"

class VTK_EXPORT vtkPVProbe : public vtkPVSource
{
public:
  static vtkPVProbe* New();
  vtkTypeMacro(vtkPVProbe, vtkPVSource);
    
  // Description:
  // Set up the UI for this source
  void CreateProperties();

  // Description:
  // Set the interactor to use for choosing a point for probing.
  void SetInteractor();
  
  // Description:
  // Called when the accept button is clicked.
  void UpdateProbe();

  // Description:
  // Methods to call when this pv source is selected/deselected
  void Select(vtkKWView *view);
  void Deselect(vtkKWView *view);

  // Description:
  // Set/Get the ProbeSourceTclName
  vtkSetStringMacro(ProbeSourceTclName);
  vtkGetStringMacro(ProbeSourceTclName);
  
  // Description:
  // Called when the accept button is pressed.
  virtual void AcceptCallback();
  
  // Description:
  // Called when the delete button is pressed.
  virtual void DeleteCallback();

  // Description:
  // Callbacks for Dimensionality menu
  void UsePoint();
  void UseLine();

  // Description:
  // Set the vtkPVData object whose VTK data will be used as the source for
  // vtkProbeFilter.
  vtkSetObjectMacro(PVProbeSource, vtkPVData);

  // Description:
  // Set the entries for SelectedPoint, EndPoint1, and EndPoint2.
  void SetSelectedPoint(float point[3]);
  void SetEndPoint1(float point[3]);
  void SetEndPoint2(float point[3]);

  // Description:
  // Get the dimensionality of the input to vtkProbeFilter.
  vtkGetMacro(Dimensionality, int);
  
  // Description:
  // Get the current end point id of the line input to vtkProbeFilter.
  vtkGetMacro(CurrentEndPoint, int);
  void SetCurrentEndPoint(int id);

  // Description:
  // Change the 3D cursor to match the values in the UI.
  void ChangeXPosition();
  void ChangeYPosition();
  void ChangeZPosition();

  // Description:
  // Write out the part of the tcl script cooresponding to vtkPVProbe
  void SaveInTclScript(ofstream *file);
  
protected:
  vtkPVProbe();
  ~vtkPVProbe();
  vtkPVProbe(const vtkPVProbe&) {};
  void operator=(const vtkPVProbe&) {};

  virtual void UpdateScalarsMenu();
  virtual void UpdateScalars();
  
  char *ProbeSourceTclName;
  
  vtkKWLabel *DimensionalityLabel;
  vtkKWOptionMenu *DimensionalityMenu;
  vtkKWRadioButton *SelectPointButton;
  vtkKWWidget *ProbeFrame;

  vtkKWWidget *SelectedPointFrame;
  vtkKWLabel *SelectedPointLabel;
  vtkKWLabeledEntry *SelectedXEntry;
  vtkKWLabeledEntry *SelectedYEntry;
  vtkKWLabeledEntry *SelectedZEntry;  
  vtkKWLabel *PointDataLabel;
  
  vtkKWLabel *EndPointLabel;
  vtkKWOptionMenu *EndPointMenu;
  vtkKWWidget *EndPointMenuFrame;
  vtkKWWidget *EndPoint1Frame;
  vtkKWLabel *EndPoint1Label;
  vtkKWLabeledEntry *End1XEntry;
  vtkKWLabeledEntry *End1YEntry;
  vtkKWLabeledEntry *End1ZEntry;
  vtkKWWidget *EndPoint2Frame;
  vtkKWLabel *EndPoint2Label;
  vtkKWLabeledEntry *End2XEntry;
  vtkKWLabeledEntry *End2YEntry;
  vtkKWLabeledEntry *End2ZEntry;
  vtkKWLabeledEntry *DivisionsEntry;
  
  vtkKWSelectPointInteractor *Interactor;
  
  int Dimensionality; // point = 0, line = 1
  int CurrentEndPoint;
  
  vtkPVData *PVProbeSource;
  
  char *XYPlotTclName;
  vtkSetStringMacro(XYPlotTclName);

  int InstanceCount;
  vtkKWInteractor *PreviousInteractor;
};

#endif
