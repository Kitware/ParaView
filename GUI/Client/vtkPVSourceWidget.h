/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSourceWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSourceWidget - superclass for widgets which contain a vtk source
// .SECTION Description
// The subclasses of this widget contain a vtkSource. This source (which
// is created on all processes) can be used as input or source to 
// filters.

#ifndef __vtkPVSourceWidget_h
#define __vtkPVSourceWidget_h

#include "vtkPVObjectWidget.h"

class VTK_EXPORT vtkPVSourceWidget : public vtkPVObjectWidget
{
public:
  vtkTypeRevisionMacro(vtkPVSourceWidget, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The Tcl name of the contained VTK source.
  vtkGetMacro(SourceID,vtkClientServerID);

  // Description:
  // The Tcl name of the output of the contained VTK source.
  vtkGetMacro(OutputID,vtkClientServerID);

  // Description:
  // A method for saving a widget into a VTK Tcl script.
  virtual void SaveInBatchScript(ofstream *file);
  // Description:
  // Return SourceID for Source, and OutputID for Output.
  virtual vtkClientServerID GetObjectByName(const char*);

protected:
  vtkPVSourceWidget();
  ~vtkPVSourceWidget();

  vtkClientServerID SourceID;
  vtkClientServerID OutputID;

  // Description:
  // A method for saving a widget into a VTK Tcl script.
  virtual void SaveInBatchScriptForPart(ofstream *file, vtkClientServerID);

  vtkPVSourceWidget(const vtkPVSourceWidget&); // Not implemented
  void operator=(const vtkPVSourceWidget&); // Not implemented

};

#endif
