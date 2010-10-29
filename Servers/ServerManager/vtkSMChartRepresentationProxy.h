/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMChartRepresentationProxy
// .SECTION Description
//

#ifndef __vtkSMChartRepresentationProxy_h
#define __vtkSMChartRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class vtkChartRepresentation;

class VTK_EXPORT vtkSMChartRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMChartRepresentationProxy* New();
  vtkTypeMacro(vtkSMChartRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns client side representation object.
  vtkChartRepresentation* GetRepresentation();

  // Description:
  // vtkChartRepresentation has two input ports one of the data, and
  // the other for the extracted selections. The data-input is exposed from the
  // proxy, while the extracted selection input is hidden and implicitly defined
  // based on the data input. We override this method to handle that.
  virtual void AddInput(unsigned int inputPort,
    vtkSMSourceProxy* input, unsigned int outputPort, const char* method);
  virtual void AddInput(vtkSMSourceProxy* input, const char* method)
    { this->Superclass::AddInput(input, method); }

//BTX
protected:
  vtkSMChartRepresentationProxy();
  ~vtkSMChartRepresentationProxy();

  virtual void CreateVTKObjects();

private:
  vtkSMChartRepresentationProxy(const vtkSMChartRepresentationProxy&); // Not implemented
  void operator=(const vtkSMChartRepresentationProxy&); // Not implemented
//ETX
};

#endif
