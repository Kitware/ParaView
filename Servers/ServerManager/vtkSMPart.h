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
class vtkSMPartDisplay;
class vtkSMDisplay;
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

  //BTX
  // Description:
  vtkPVClassNameInformation* GetClassNameInformation();
  //ETX

  // Description:
  void GatherClassNameInformation();

  // Description:
  void GatherDataInformation();

  // Description:
  void InvalidateDataInformation();

  // Description:
  void InsertExtractPiecesIfNecessary();

  // Description:
  void CreateTranslatorIfNecessary();

//ETX
  // Description:
  // Update the data and geometry.
  void Update();

  // Description:
  // Modified propagated forward to eliminate extra network update calls.
  void MarkForUpdate();
  int UpdateNeeded;

protected:
  vtkSMPart();
  ~vtkSMPart();

  vtkSMPart(const vtkSMPart&); // Not implemented
  void operator=(const vtkSMPart&); // Not implemented

  vtkPVClassNameInformation* ClassNameInformation;
  int ClassNameInformationValid;
  vtkPVDataInformation* DataInformation;
  int DataInformationValid;

};

#endif
