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

class vtkKWCheckButton;
class vtkKWLabel;
class vtkKWLabel;
class vtkKWLabeledEntry;
class vtkKWOptionMenu;
class vtkKWWidget;
class vtkPVArrayMenu;
class vtkPVLineWidget;
class vtkPVPointWidget;

class VTK_EXPORT vtkPVProbe : public vtkPVSource
{
public:
  static vtkPVProbe* New();
  vtkTypeMacro(vtkPVProbe, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  void CreateProperties();

  // Description:
  // We are redefine the input to the VTK probes source.
  virtual void SetPVInput(vtkPVData *input);

  // Description:
  // Called when the accept button is clicked.
  void UpdateProbe();

  // Description:
  // Methods to call when this pv source is selected/deselected
  virtual void Deselect() { this->Deselect(1); }
  virtual void Deselect(int doPackForget);

  // Description:
  // Called when the accept button is pressed.
  virtual void AcceptCallback();
  
  // Description:
  // Callbacks for Dimensionality menu
  void UsePoint();
  void UseLine();

  // Description:
  // Set the entries for SelectedPoint, EndPoint1, and EndPoint2.
  void SetSelectedPoint(float point[3]);
  void SetEndPoint1(float point[3]);
  void SetEndPoint2(float point[3]);
  void SetEndPoint1(float x, float y, float z) 
    { 
    float p[3]; 
    p[0] = x; p[1] = y; p[2] = z;
    this->SetEndPoint1(p);
    }
  void SetEndPoint2(float x, float y, float z) 
    { 
    float p[3]; 
    p[0] = x; p[1] = y; p[2] = z;
    this->SetEndPoint2(p);
    }
  vtkGetVector3Macro(EndPoint1, float);
  vtkGetVector3Macro(EndPoint2, float);

  void SetPointPosition(float point[3]);
  void SetPointPosition(float x, float y, float z)
    {
    float p[3];
    p[0] = x; p[1] = y; p[2] = z;
    this->SetPointPosition(p);
    }
  vtkGetVector3Macro(PointPosition, float);

  // Description:
  // Get the dimensionality of the input to vtkProbeFilter.
  vtkGetMacro(Dimensionality, int);

  // Description:
  // Set and get the number of line divisions.
  void SetNumberOfLineDivisions(int);
  vtkGetMacro(NumberOfLineDivisions, int);
  
  // Description:
  // Write out the part of the tcl script cooresponding to vtkPVProbe
  void SaveInTclScript(ofstream *file);
  
  // Description:
  // Method to update which scalars are being used in the xyplot
  virtual void UpdateScalars();

  // Description:
  // Access to the ShowXYPlotToggle from Tcl
  vtkGetObjectMacro(ShowXYPlotToggle, vtkKWCheckButton);
  
protected:
  vtkPVProbe();
  ~vtkPVProbe();
  
  // Description:
  // Create a menu to select the input.
  virtual vtkPVInputMenu *AddInputMenu(char* label, char* inputName, 
				       char* inputType, char* help, 
				       vtkPVSourceCollection* sources);
  vtkPVInputMenu* InputMenu;
  virtual void SetInputMenu(vtkPVInputMenu* im);

  vtkKWLabel *DimensionalityLabel;
  vtkKWOptionMenu *DimensionalityMenu;
  vtkKWWidget *ProbeFrame;

  vtkPVArrayMenu *ScalarArrayMenu;

  vtkKWWidget *SelectedPointFrame;
  vtkKWLabel *SelectedPointLabel;
  vtkKWLabeledEntry *SelectedXEntry;
  vtkKWLabeledEntry *SelectedYEntry;
  vtkKWLabeledEntry *SelectedZEntry;  
  vtkKWLabel *PointDataLabel;
  
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
  vtkKWCheckButton *ShowXYPlotToggle;
  vtkKWLabeledEntry *DivisionsEntry;

  vtkPVLineWidget *LineWidget;
  vtkPVPointWidget *PointWidget;
  
  int Dimensionality; // point = 0, line = 1
  
  char* XYPlotTclName;
  vtkSetStringMacro(XYPlotTclName);

  int InstanceCount;

  float EndPoint1[3];
  float EndPoint2[3];
  int NumberOfLineDivisions;

  float PointPosition[3];

  virtual int ClonePrototypeInternal(int makeCurrent, vtkPVSource*& clone);

  vtkPVProbe(const vtkPVProbe&); // Not implemented
  void operator=(const vtkPVProbe&); // Not implemented
};

#endif
