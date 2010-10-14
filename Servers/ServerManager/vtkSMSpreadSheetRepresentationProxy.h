/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSpreadSheetRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSpreadSheetRepresentationProxy
// .SECTION Description
// vtkSMSpreadSheetRepresentationProxy is a representation proxy used for
// spreadsheet view. This class overrides vtkSMRepresentationProxy to ensure
// that the selection inputs are setup correctly.

#ifndef __vtkSMSpreadSheetRepresentationProxy_h
#define __vtkSMSpreadSheetRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class VTK_EXPORT vtkSMSpreadSheetRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMSpreadSheetRepresentationProxy* New();
  vtkTypeMacro(vtkSMSpreadSheetRepresentationProxy,
    vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // vtkSpreadSheetRepresentation has three input ports one of the data, and
  // the others for the extracted selections. The data-input is exposed from the
  // proxy, while the extracted selection input is hidden and implicitly defined
  // based on the data input. We override this method to handle that.
  virtual void AddInput(unsigned int inputPort,
    vtkSMSourceProxy* input, unsigned int outputPort, const char* method);
  virtual void AddInput(vtkSMSourceProxy* input, const char* method)
    { this->Superclass::AddInput(input, method); }

//BTX
protected:
  vtkSMSpreadSheetRepresentationProxy();
  ~vtkSMSpreadSheetRepresentationProxy();


private:
  vtkSMSpreadSheetRepresentationProxy(const vtkSMSpreadSheetRepresentationProxy&); // Not implemented
  void operator=(const vtkSMSpreadSheetRepresentationProxy&); // Not implemented
//ETX
};

#endif

