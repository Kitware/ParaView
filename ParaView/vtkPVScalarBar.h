/*=========================================================================

  Program:   ParaView
  Module:    vtkPVScalarBar.h
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
// .NAME vtkPVScalarBar - Class to add scalar bars into ParaView
// .SECTION Description
// Create a scalar bar to be associated with each data object.


#ifndef __vtkPVScalarBar_h
#define __vtkPVScalarBar_h

#include "vtkKWComposite.h"
#include "vtkScalarBarActor.h"
#include "vtkPVData.h"
#include "vtkKWPushButton.h"
#include "vtkKWMenuButton.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWCheckButton.h"
#include "vtkKWScale.h"

class VTK_EXPORT vtkPVScalarBar : public vtkKWComposite
{
public:
  static vtkPVScalarBar* New();
  vtkTypeMacro(vtkPVScalarBar, vtkKWComposite);

  // Description:
  // Get the scalar bar.  (need to override pure virtual function in
  // vtkKWComposite)
  vtkProp *GetProp();

  // Description:
  // Create/Show the UI.
  void CreateProperties();

  // Description:
  // Set/Get the underlying vtkScalarBarActor.
  vtkSetObjectMacro(ScalarBar, vtkScalarBarActor);
  vtkGetObjectMacro(ScalarBar, vtkScalarBarActor);
  
  // Description:
  // ONLY SET THIS IF YOU ARE A PVDATA!
  // The scalar bar needs to know who owns it.
  void SetPVData(vtkPVData *data);
  vtkGetObjectMacro(PVData, vtkPVData);
  
  // Description:
  // Method called by the push button to return to the data notebook.
  void ShowDataNotebook();

  void SetOrientationToHorizontal();
  void SetOrientationToVertical();

  // Description:
  // This flag turns the visibility of the prop on and off.
  void SetVisibility(int v);
  int GetVisibility();
  vtkBooleanMacro(Visibility, int);
  
  // Description:
  // This method is a callback from the checkbutton that controls the
  // visibility of the PVScalarBar.
  void SetScalarBarVisibility();
  
  // Description:
  // This method is a callback from the entry box that controls the
  // title of the PVScalarBar.
  void SetScalarBarTitle();

  // Description:
  // These methods are callbacks from the sliders that control the
  // height, width, X position, and Y position of the PVScalarBar.
  void SetScalarBarHeight();
  void SetScalarBarWidth();
  void SetScalarBarXPosition();
  void SetScalarBarYPosition();

  void UpdateLookupTable();
  
protected:
  vtkPVScalarBar();
  ~vtkPVScalarBar();
  vtkPVScalarBar(const vtkPVScalarBar&) {};
  void operator=(const vtkPVScalarBar&) {};
  
  int Visibility;
  
  vtkScalarBarActor *ScalarBar;
  vtkKWPushButton *DataNotebookButton;
  vtkKWMenuButton *OrientationMenu;
  vtkKWCheckButton *VisibilityButton;
  vtkKWLabeledEntry *TitleEntry;
  vtkKWScale *WidthScale;
  vtkKWScale *HeightScale;
  vtkKWScale *XPositionScale;
  vtkKWScale *YPositionScale;
  vtkKWWidget *Properties;
  
  // The vtkPVData object that owns this vtkPVScalarBar object.
  vtkPVData *PVData;
};

#endif
