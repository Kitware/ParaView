/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDatasetSpreadRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDatasetSpreadRepresentationProxy
// .SECTION Description
// This is a representation proxy for spreadsheet view which can be used to show
// output of algorithms  producing vtkDataSet or subclasses.

#ifndef __vtkSMDatasetSpreadRepresentationProxy_h
#define __vtkSMDatasetSpreadRepresentationProxy_h

#include "vtkSMDataRepresentationProxy.h"

class VTK_EXPORT vtkSMDatasetSpreadRepresentationProxy : 
  public vtkSMDataRepresentationProxy
{
public:
  static vtkSMDatasetSpreadRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMDatasetSpreadRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the client side dataset for the active window.
  // It must be noted that the connectivity information in the dataset is bogus. 
  // This representation merely delivers the point data, cell data and field
  // data arrays to the client. 
  virtual vtkDataSet* GetOutput();

//BTX
protected:
  vtkSMDatasetSpreadRepresentationProxy();
  ~vtkSMDatasetSpreadRepresentationProxy();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();


private:
  vtkSMDatasetSpreadRepresentationProxy(const vtkSMDatasetSpreadRepresentationProxy&); // Not implemented
  void operator=(const vtkSMDatasetSpreadRepresentationProxy&); // Not implemented
//ETX
};

#endif

