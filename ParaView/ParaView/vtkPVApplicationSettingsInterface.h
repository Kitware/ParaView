/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVApplicationSettingsInterface.h
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
// .NAME vtkPVApplicationSettingsInterface - a user interface panel.
// .SECTION Description
// A concrete implementation of a user interface panel. It extends the
// user interface defined in vtkKWApplicationSettingsInterface.
// See vtkKWUserInterfacePanel for a more detailed description.
// .SECTION See Also
// vtkKWApplicationSettingsInterface vtkKWUserInterfacePanel vtkKWUserInterfaceManager vtkKWUserInterfaceNotebookManager

#ifndef __vtkPVApplicationSettingsInterface_h
#define __vtkPVApplicationSettingsInterface_h

#include "vtkKWApplicationSettingsInterface.h"

//------------------------------------------------------------------------------

class vtkKWCheckButton;
class vtkKWLabeledFrame;

class VTK_EXPORT vtkPVApplicationSettingsInterface : public vtkKWApplicationSettingsInterface
{
public:
  static vtkPVApplicationSettingsInterface* New();
  vtkTypeRevisionMacro(vtkPVApplicationSettingsInterface,vtkKWApplicationSettingsInterface);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the interface objects.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Refresh the interface given the current value of the Window and its
  // views/composites/widgets.
  virtual void Update();

  // Description:
  // Callback used when interaction has been performed.
  virtual void ShowSourcesDescriptionCallback();
  virtual void ShowSourcesNameCallback();

protected:
  vtkPVApplicationSettingsInterface();
  ~vtkPVApplicationSettingsInterface();

  vtkKWCheckButton *ShowSourcesDescriptionCheckButton;
  vtkKWCheckButton *ShowSourcesNameCheckButton;

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

private:
  vtkPVApplicationSettingsInterface(const vtkPVApplicationSettingsInterface&); // Not implemented
  void operator=(const vtkPVApplicationSettingsInterface&); // Not Implemented
};

#endif
