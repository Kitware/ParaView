/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLineSourceWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLineSourceWidget - a LineWidget which contains a separate line source
// .SECTION Description
// This widget adds vtkLineSource to vtkPVLineWidget.
// This vtkLineSource (which is created on all processes) can be used as 
// input or source to filters (for example as streamline seed).

#ifndef __vtkPVLineSourceWidget_h
#define __vtkPVLineSourceWidget_h

#include "vtkPVLineWidget.h"
class vtkPVInputMenu;
class vtkSMSourceProxy;

class VTK_EXPORT vtkPVLineSourceWidget : public vtkPVLineWidget
{
public:

  static vtkPVLineSourceWidget* New();
  vtkTypeRevisionMacro(vtkPVLineSourceWidget, vtkPVLineWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates common widgets.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Saves the value of this widget into a VTK Tcl script.
  // This creates the line source (one for all parts).
  virtual void SaveInBatchScript(ofstream *file);

  //BTX
  // Description:
  // The methods get called when the Accept button is pressed. 
  // It sets the VTK objects value using this widgets value.
  virtual void Accept();
  //ETX

  // Description:
  // Initialize place after creation
  virtual void Initialize();

  // Description:
  // The methods get called when the Reset button is pressed. 
  // It sets this widgets value using the VTK objects value.
  virtual void ResetInternal();

  // Description:
  // This is called if the input menu changes.
  virtual void Update();

  void SetInputMenu(vtkPVInputMenu *im);

  virtual vtkSMProxy* GetProxyByName(const char*) { return reinterpret_cast<vtkSMProxy*>(this->SourceProxy); }
  
protected:
  vtkPVLineSourceWidget();
  ~vtkPVLineSourceWidget();
  
  vtkSMSourceProxy *SourceProxy;

  vtkPVLineSourceWidget(const vtkPVLineSourceWidget&); // Not implemented
  void operator=(const vtkPVLineSourceWidget&); // Not implemented

  virtual int ReadXMLAttributes(vtkPVXMLElement *element,
                                vtkPVXMLPackageParser *parser);
//BTX
  virtual void CopyProperties(vtkPVWidget *clone, vtkPVSource *pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  vtkPVInputMenu *InputMenu;

};

#endif
