/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMinMax.h
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
// .NAME vtkPVMinMax -
// .SECTION Description

#ifndef __vtkPVMinMax_h
#define __vtkPVMinMax_h

#include "vtkPVWidget.h"
#include "vtkKWApplication.h"
#include "vtkKWScale.h"
#include "vtkKWLabel.h"

class VTK_EXPORT vtkPVMinMax : public vtkPVWidget
{
public:
  static vtkPVMinMax* New();
  vtkTypeMacro(vtkPVMinMax, vtkPVWidget);

  void Create(vtkKWApplication *pvApp, char *minLabel, char *maxLabel,
              float min, float max, float resolution,
              char *setCmd, char *getMinCmd, char *getMaxCmd, char *minHelp,
              char *maxHelp);
  
  // Description:
  // Called when accept button is pushed.  Just adds to trace
  // file and calls supperclass accept.
  virtual void Accept();
  
  // Description:
  // This method allows scripts to modify the widgets value.
  void SetMinValue(float val);
  float GetMinValue() { return this->MinScale->GetValue(); }
  void SetMaxValue(float val);
  float GetMaxValue() { return this->MaxScale->GetValue(); }
  void SetResolution(float res);
  float GetResolution() { return this->MinScale->GetResolution(); }
  void SetRange(float min, float max);
  
  // Description:
  // Save this widget to a file
  virtual void SaveInTclScript(ofstream *file, const char *sourceName);

  // Description:
  // Callback for min scale
  void MinValueCallback();
  
  // Description:
  // Callback for max scale
  void MaxValueCallback();
  
protected:
  vtkPVMinMax();
  ~vtkPVMinMax();
  vtkPVMinMax(const vtkPVMinMax&) {};
  void operator=(const vtkPVMinMax&) {};
  
  vtkKWLabel *MinLabel;
  vtkKWLabel *MaxLabel;
  vtkKWScale *MinScale;
  vtkKWScale *MaxScale;
  vtkKWWidget *MinFrame;
  vtkKWWidget *MaxFrame;

  char *GetMinCommand;
  char *GetMaxCommand;
  
  vtkSetStringMacro(GetMinCommand);
  vtkSetStringMacro(GetMaxCommand);
};

#endif
