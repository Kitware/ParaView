/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkPVWindow
// .SECTION Description
// This class represents a top level window with menu bar and status
// line. It is designed to hold one or more vtkPVViews in it.

#ifndef __vtkPVWindow_h
#define __vtkPVWindow_h

#include "vtkKWWindow.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVSourceList.h"

class vtkKWNotebook;
class vtkKWToolbar;
class vtkKWScale;
class vtkKWPushButton;

class VTK_EXPORT vtkPVWindow : public vtkKWWindow
{
public:
  static vtkPVWindow* New();
  vtkTypeMacro(vtkPVWindow,vtkKWWindow);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, char *args);

  // Description:
  // Open or close a polygonal dataset for viewing.
  virtual void NewWindow();
  virtual void Save();
  virtual void NewCone();
  virtual void NewVolume();
  
  // Description:
  // Chaining method to serialize an object and its superclasses.
  virtual void SerializeSelf(ostream& os, vtkIndent indent);
  virtual void SerializeToken(istream& is,const char token[1024]);
    
  // Description:
  // Access to the RenderView.
  vtkGetObjectMacro(MainView, vtkPVRenderView);

  void SetCurrentSource(vtkPVSource *comp);
  vtkPVSource *GetCurrentSource();
  vtkPVSource *GetNextSource();
  vtkPVSource *GetPreviousSource();
  
  vtkKWCompositeCollection *GetSources();
  
  //tcl callbacks that use GetNextComposite and GetPreviousComposite
  void NextSource();
  void PreviousSource();
  
  vtkGetObjectMacro(SourceList, vtkPVSourceList);

  // Description:
  // Callback from the reset camera button.
  void ResetCameraCallback();
  
  // Description:
  // Callback to display window proerties
  void ShowWindowProperties();
  
protected:
  vtkPVWindow();
  ~vtkPVWindow();
  vtkPVWindow(const vtkPVWindow&) {};
  void operator=(const vtkPVWindow&) {};

  void SetupCone();
  
  vtkPVRenderView *MainView;
  vtkKWMenu *CreateMenu;
  
  vtkKWToolbar *Toolbar;
  vtkKWPushButton *ResetCameraButton;
  vtkKWPushButton *PreviousSourceButton;
  vtkKWPushButton *NextSourceButton;
  vtkKWPushButton *SourceListButton;

  vtkKWCompositeCollection *Sources;
  vtkKWLabeledFrame *ApplicationAreaFrame;
  vtkPVSourceList *SourceList;
};


#endif
