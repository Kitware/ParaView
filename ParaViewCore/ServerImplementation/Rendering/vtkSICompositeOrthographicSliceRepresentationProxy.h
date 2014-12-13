/*=========================================================================

  Program:   ParaView
  Module:    vtkSICompositeOrthographicSliceRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSICompositeOrthographicSliceRepresentationProxy
// .SECTION Description
//

#ifndef __vtkSICompositeOrthographicSliceRepresentationProxy_h
#define __vtkSICompositeOrthographicSliceRepresentationProxy_h

#include "vtkSIPVRepresentationProxy.h"

class VTKPVSERVERIMPLEMENTATIONRENDERING_EXPORT vtkSICompositeOrthographicSliceRepresentationProxy : public vtkSIPVRepresentationProxy
{
public:
  static vtkSICompositeOrthographicSliceRepresentationProxy* New();
  vtkTypeMacro(vtkSICompositeOrthographicSliceRepresentationProxy, vtkSIPVRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSICompositeOrthographicSliceRepresentationProxy();
  ~vtkSICompositeOrthographicSliceRepresentationProxy();

  // Description:
  // Parses the XML to create property/subproxy helpers.
  // Overridden to process the GeometrySliceRepresentation[X|Y|Z]
  // representations.
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element);

private:
  vtkSICompositeOrthographicSliceRepresentationProxy(const vtkSICompositeOrthographicSliceRepresentationProxy&); // Not implemented
  void operator=(const vtkSICompositeOrthographicSliceRepresentationProxy&); // Not implemented
//ETX
};

#endif
