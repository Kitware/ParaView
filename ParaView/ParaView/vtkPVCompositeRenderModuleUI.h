/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeRenderModuleUI.h
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
// .NAME vtkPVCompositeRenderModuleUI - UI for composite options..
// .SECTION Description
// This render module user interface control compositing.


#ifndef __vtkPVCompositeRenderModuleUI_h
#define __vtkPVCompositeRenderModuleUI_h

#include "vtkPVLODRenderModuleUI.h"

class vtkPVCompositeRenderModule;

class VTK_EXPORT vtkPVCompositeRenderModuleUI : public vtkPVLODRenderModuleUI
{
public:
  static vtkPVCompositeRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVCompositeRenderModuleUI,vtkPVLODRenderModuleUI);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the TK widgets associated with the view.
  virtual void Create(vtkKWApplication *app, const char *);
      
  // Description:
  // This pointer gets down cast to a vtkPVCompositeRenderModule.
  // This UI uses the pointer to access composite specific methods.
  void SetRenderModule(vtkPVRenderModule* rm);

  // Description:
  // Callback for the use char check button.  
  // These are only public because they are callbacks.
  // Cannot be used from a script because they do not 
  // change the state of the check.
  void CompositeWithFloatCallback();
  void CompositeWithFloatCallback(int val);
  void CompositeWithRGBACallback();
  void CompositeWithRGBACallback(int val);
  void CompositeCompressionCallback();
  void CompositeCompressionCallback(int val);

  // Description:
  // Tracing uses the method with the argument.
  // A reduction value of 1 is equivalent to having the feature
  // disabled.
  void ReductionCheckCallback();
  void ReductionFactorScaleCallback();
  void SetReductionFactor(int val);

  // Description:
  // Squirt compression os a combination of run-length encoding
  // and bit compression.  A level of 0 is the same as disabling
  // squirt compression.
  void SquirtCheckCallback();
  void SquirtLevelScaleCallback();
  void SetSquirtLevel(int val);

  // Description:
  // Threshold for collecting data to a single process (MBytes).
  void CollectCheckCallback();
  void CollectThresholdScaleCallback();
  void SetCollectThreshold(float val);
  vtkGetMacro(CollectThreshold, float);
  // I should really put this in the composite module.
  void SetCollectThresholdInternal(float threshold);

protected:
  vtkPVCompositeRenderModuleUI();
  ~vtkPVCompositeRenderModuleUI();
 
  vtkPVCompositeRenderModule* CompositeRenderModule;

  vtkKWLabeledFrame *ParallelRenderParametersFrame;

  vtkKWCheckButton *CompositeWithFloatCheck;
  vtkKWCheckButton *CompositeWithRGBACheck;
  vtkKWCheckButton *CompositeCompressionCheck;

  vtkKWLabel*       CollectLabel;
  vtkKWCheckButton* CollectCheck;
  vtkKWScale*       CollectThresholdScale;
  vtkKWLabel*       CollectThresholdLabel;
  float             CollectThreshold;

  vtkKWLabel*       ReductionLabel;
  vtkKWCheckButton* ReductionCheck;
  vtkKWScale*       ReductionFactorScale;
  vtkKWLabel*       ReductionFactorLabel;
  float             ReductionFactor;

  vtkKWLabel*       SquirtLabel;
  vtkKWCheckButton* SquirtCheck;
  vtkKWScale*       SquirtLevelScale;      
  vtkKWLabel*       SquirtLevelLabel;
  int               SquirtLevel;

  int CompositeWithFloatFlag;
  int CompositeWithRGBAFlag;
  int CompositeCompressionFlag;

  vtkPVCompositeRenderModuleUI(const vtkPVCompositeRenderModuleUI&); // Not implemented
  void operator=(const vtkPVCompositeRenderModuleUI&); // Not implemented
};


#endif
