/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPart.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPart - proxy for a data object
// .SECTION Description
// This object manages one vtk data set. It is used internally
// by vtkSMSourceProxy to manage all of it's outputs.

#ifndef __vtkSMPart_h
#define __vtkSMPart_h

#include "vtkSMProxy.h"

class vtkPVClassNameInformation;
class vtkPVDataInformation;
class vtkPVPartDisplay;
class vtkPVDisplay;
class vtkCollection;

class VTK_EXPORT vtkSMPart : public vtkSMProxy
{
public:
  static vtkSMPart* New();
  vtkTypeRevisionMacro(vtkSMPart, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  vtkPVDataInformation* GetDataInformation();
  //ETX
  
  // Description:
  void GatherDataInformation();

  // Description:
  void InvalidateDataInformation();

  // Description:
  void InsertExtractPiecesIfNecessary();

  // Description:
  void CreateTranslatorIfNecessary();

  // These methods/ivars are just copied from PVPart.
  // Cleanup later.

  // Description:
  // Temporary access to the display object.
  // Render modules may eleimnate the need for this access.
//BTX
  vtkPVPartDisplay* GetPartDisplay() { return this->PartDisplay;}
  void SetPartDisplay(vtkPVPartDisplay* pDisp);

  // Description:
  // We are starting to support multiple types of displays (plot).
  // I am keeping the PartDisplay pointer and methods around
  // until we come up with a better API (maybe proxy/properties).
  // The method SetPartDisplay also adds the display to this collection.
  void AddDisplay(vtkPVDisplay* disp);
//ETX
  // Description:
  // Update the data and geometry.
  void Update();

  // Description:
  // Modified propagated forward to eliminate extra network update calls.
  void MarkForUpdate();

protected:
  vtkSMPart();
  ~vtkSMPart();

  vtkSMPart(const vtkSMPart&); // Not implemented
  void operator=(const vtkSMPart&); // Not implemented

  vtkPVDataInformation* DataInformation;
  vtkPVClassNameInformation* ClassNameInformation;
  int DataInformationValid;

  // We are starting to support multiple types of displays (plot).
  // I am keeping the PartDisplay pointer and methods around
  // until we come up with a better API (maybe proxy/properties).
  // The part display is also in the collection.
  vtkCollection* Displays;
  vtkPVPartDisplay* PartDisplay;
};

#endif
