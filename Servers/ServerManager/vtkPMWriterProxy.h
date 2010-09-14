/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMWriterProxy
// .SECTION Description
//

#ifndef __vtkPMWriterProxy_h
#define __vtkPMWriterProxy_h

#include "vtkPMSourceProxy.h"

class VTK_EXPORT vtkPMWriterProxy : public vtkPMSourceProxy
{
public:
  static vtkPMWriterProxy* New();
  vtkTypeMacro(vtkPMWriterProxy, vtkPMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMWriterProxy();
  ~vtkPMWriterProxy();

  // Description:
  // Creates the VTKObjects. Overridden to add post-filters to the pipeline.
  virtual bool CreateVTKObjects(vtkSMMessage* message);

  // Description:
  // Read xml-attributes.
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element);

  char* FileNameMethod;
  vtkSetStringMacro(FileNameMethod);

private:
  vtkPMWriterProxy(const vtkPMWriterProxy&); // Not implemented
  void operator=(const vtkPMWriterProxy&); // Not implemented
//ETX
};

#endif
