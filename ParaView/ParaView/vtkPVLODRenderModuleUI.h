/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODRenderModuleUI.h
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
// .NAME vtkPVLODRenderModuleUI - Default serial render module.
// .SECTION Description
// This render module uses quadric clustering to create a single level of detail.


#ifndef __vtkPVLODRenderModuleUI_h
#define __vtkPVLODRenderModuleUI_h

#include "vtkPVRenderModuleUI.h"

class vtkKWLabel;
class vtkKWPushButton;
class vtkKWRadioButton;
class vtkKWLabeledFrame;
class vtkKWScale;
class vtkKWCheckButton;
class vtkKWSplitFrame;
class vtkLabeledFrame;
class vtkMultiProcessController;
class vtkPVApplication;
class vtkPVCameraIcon;
class vtkPVData;
class vtkPVInteractorStyleControl;
class vtkPVLODRenderModuleUIObserver;
class vtkPVSource;
class vtkPVSourceList;
class vtkPVSourcesNavigationWindow;
class vtkPVTreeComposite;
class vtkPVWindow;
class vtkPVRenderModule;
class vtkPVLODRenderModule;

class VTK_EXPORT vtkPVLODRenderModuleUI : public vtkPVRenderModuleUI
{
public:
  static vtkPVLODRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVLODRenderModuleUI,vtkPVRenderModuleUI);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Create the TK widgets associated with the view.
  virtual void Create(vtkKWApplication *app, const char *);
  
  // Description:
  // The subclass should implement this method and 
  // downcast it to the right type.  It can then access
  // any unique methods of the specific render module.
  virtual void SetRenderModule(vtkPVRenderModule *rm);
  
  // Description:
  // Callback for the interrupt render check button
  void RenderInterruptsEnabledCheckCallback();
  
  // Description:
  // Threshold for individual actors as number of points.
  void LODThresholdScaleCallback();

  // Description:
  // This method sets the threshold without tracing or
  // changing the UI scale.
  void SetLODThresholdInternal(float threshold);

  // Description:
  // This methods can be used from a script.  
  // "Set" sets the value of the scale, and adds an entry to the trace.
  void SetLODThreshold(float);
  vtkGetMacro(LODThreshold, float);
  vtkBooleanMacro(LODThreshold, float);

  // Description:
  // LOD resolution determines how many cells are in decimated model.
  void LODResolutionScaleCallback();
  void LODResolutionLabelCallback();

  // Description:
  // This method sets the resolution without tracing or
  // changing the UI scale.
  void SetLODResolutionInternal(int threshold);

  // Description:
  // This methods can be used from a script.  
  // "Set" sets the value of the scale, and adds an entry to the trace.
  void SetLODResolution(int);
  vtkGetMacro(LODResolution, int);
  vtkBooleanMacro(LODResolution, int);


protected:
  vtkPVLODRenderModuleUI();
  ~vtkPVLODRenderModuleUI();
 
  vtkPVLODRenderModule* LODRenderModule;

  int UseReductionFactor;
  
  vtkKWLabeledFrame *LODFrame;
  vtkKWCheckButton *RenderInterruptsEnabledCheck;

  vtkKWWidget *LODScalesFrame;
  vtkKWLabel *LODResolutionLabel;
  vtkKWScale *LODResolutionScale;
  vtkKWLabel *LODResolutionValue;
  vtkKWLabel *LODThresholdLabel;
  vtkKWScale *LODThresholdScale;
  vtkKWLabel *LODThresholdValue;

  float LODThreshold;
  int LODResolution;
  int RenderInterruptsEnabled;

  vtkPVLODRenderModuleUI(const vtkPVLODRenderModuleUI&); // Not implemented
  void operator=(const vtkPVLODRenderModuleUI&); // Not implemented
};


#endif
