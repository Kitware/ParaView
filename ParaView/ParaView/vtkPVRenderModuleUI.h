/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModuleUI.h
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
// .NAME vtkPVRenderModuleUI - User interface for a rendering module.
// .SECTION Description
// This is a superclass for a render module.  We do not create this class ,
// This is created by the vtkPVRenderView.

#ifndef __vtkPVRenderModuleUI_h
#define __vtkPVRenderModuleUI_h

#include "vtkKWWidget.h"

class vtkPVApplication;
class vtkPVRenderModule;

class VTK_EXPORT vtkPVRenderModuleUI : public vtkKWWidget
{
public:
  static vtkPVRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVRenderModuleUI,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
      
  // Description:
  // The subclass should implement this method and 
  // downcast it to the right type.  It can then access
  // any unique methods of the specific render module.
  virtual void SetRenderModule(vtkPVRenderModule *rm);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication* app, const char *args);

  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();
        
protected:
  vtkPVRenderModuleUI();
  ~vtkPVRenderModuleUI();
 
  vtkPVRenderModuleUI(const vtkPVRenderModuleUI&); // Not implemented
  void operator=(const vtkPVRenderModuleUI&); // Not implemented
};


#endif
