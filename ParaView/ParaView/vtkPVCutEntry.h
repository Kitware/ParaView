/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCutEntry.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCutEntry maintains a list of floats for cutting.
// .SECTION Description
// This widget lets the user add or delete floats from a list.
// It is used for cut plane offsets.

#ifndef __vtkPVCutEntry_h
#define __vtkPVCutEntry_h

#include "vtkPVContourEntry.h"

class vtkPVInputMenu;

class VTK_EXPORT vtkPVCutEntry : public vtkPVContourEntry
{
public:
  static vtkPVCutEntry* New();
  vtkTypeRevisionMacro(vtkPVCutEntry, vtkPVContourEntry);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This input menu supplies the data set.
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);

protected:
  vtkPVCutEntry();
  ~vtkPVCutEntry();
  
  virtual int ComputeWidgetRange();

  vtkPVCutEntry(const vtkPVCutEntry&); // Not implemented
  void operator=(const vtkPVCutEntry&); // Not implemented

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,      
                        vtkPVXMLPackageParser* parser);

  vtkPVInputMenu *InputMenu;

};

#endif
