/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataSetFileEntry.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDataSetFileEntry - File entry that checks type.
// .SECTION Description
// This file entry will accept VTK files.  First time any VTK file
// will do.  Once a file is picked, then any following file has to have 
// the same data type.

#ifndef __vtkPVDataSetFileEntry_h
#define __vtkPVDataSetFileEntry_h

#include "vtkPVFileEntry.h"

class vtkPDataSetReader;

class VTK_EXPORT vtkPVDataSetFileEntry : public vtkPVFileEntry
{
public:
  static vtkPVDataSetFileEntry* New();
  vtkTypeRevisionMacro(vtkPVDataSetFileEntry, vtkPVFileEntry);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVDataSetFileEntry* ClonePrototype(vtkPVSource* pvSource,
                                        vtkArrayMap<vtkPVWidget*, 
                                        vtkPVWidget*>* map);
//ETX

protected:
  vtkPVDataSetFileEntry();
  ~vtkPVDataSetFileEntry();
  
  // Called when accept button is pushed.  
  // Sets objects variable to the widgets value.
  // Adds a trace entry.  Side effect is to turn modified flag off.
  virtual void AcceptInternal(vtkClientServerID);

  vtkPDataSetReader *TypeReader;
  int Type;

  vtkPVDataSetFileEntry(const vtkPVDataSetFileEntry&); // Not implemented
  void operator=(const vtkPVDataSetFileEntry&); // Not implemented
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};

#endif
