/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPointSourceWidget.h
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
// .NAME vtkPVPointSourceWidget -  a PointWidget which contains a separate point source
// This widget contains a vtkPointWidget as well as a vtkPointSource. 
// This vtkPointSource (which is created on all processes) can be used as 
// input or source to filters (for example as streamline seed).

#ifndef __vtkPVPointSourceWidget_h
#define __vtkPVPointSourceWidget_h

#include "vtkPVSourceWidget.h"

class vtkPVPointWidget;
class vtkPVVectorEntry;
class vtkPVWidgetProperty;

class VTK_EXPORT vtkPVPointSourceWidget : public vtkPVSourceWidget
{
public:
  static vtkPVPointSourceWidget* New();
  vtkTypeRevisionMacro(vtkPVPointSourceWidget, vtkPVSourceWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates common widgets.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // The underlying line widget.
  vtkGetObjectMacro(PointWidget, vtkPVPointWidget);

  // Description:
  // Controls the radius of the point cloud.
  vtkGetObjectMacro(RadiusWidget, vtkPVVectorEntry);

  // Description:
  // Controls the number of points in the point cloud.
  vtkGetObjectMacro(NumberOfPointsWidget, vtkPVVectorEntry);

  // Description:
  // Returns if any subwidgets are modified.
  virtual int GetModifiedFlag();

  // Description:
  // This method is called when the source that contains this widget
  // is selected.
  virtual void Select();

  // Description:
  // This method is called when the source that contains this widget
  // is deselected.
  virtual void Deselect();

  // Description:
  // Create the point source in the VTK Tcl script.
  // Savea a point source (one for all parts).
  virtual void SaveInBatchScript(ofstream *file);

  //BTX
  // Description:
  // The methods get called when the Accept button is pressed. 
  // It sets the VTK objects value using this widgets value.
  virtual void AcceptInternal(vtkClientServerID);
  //ETX

  // Description:
  // The methods get called when the Reset button is pressed. 
  // It sets this widgets value using the VTK objects value.
  virtual void ResetInternal();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

protected:
  vtkPVPointSourceWidget();
  ~vtkPVPointSourceWidget();


  vtkPVPointWidget* PointWidget;

  static int InstanceCount;

  vtkPVVectorEntry* RadiusWidget;
  vtkPVVectorEntry* NumberOfPointsWidget;
  vtkPVWidgetProperty *RadiusProperty;
  vtkPVWidgetProperty *NumberOfPointsProperty;

  vtkPVPointSourceWidget(const vtkPVPointSourceWidget&); // Not implemented
  void operator=(const vtkPVPointSourceWidget&); // Not implemented

};

#endif
