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
// .NAME vtkStreamingExtentTranslator
// .SECTION Description
//

#ifndef __vtkStreamingExtentTranslator_h
#define __vtkStreamingExtentTranslator_h

#include "vtkObject.h"

class vtkInformation;
class VTK_EXPORT vtkStreamingExtentTranslator : public vtkObject
{
public:
  vtkTypeMacro(vtkStreamingExtentTranslator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the method that is responsible to convert the pass request to
  // appropriate pipeline request.
  virtual int PassToRequest(int pass, vtkInformation* info)=0;

//BTX
protected:
  vtkStreamingExtentTranslator();
  ~vtkStreamingExtentTranslator();

private:
  vtkStreamingExtentTranslator(const vtkStreamingExtentTranslator&); // Not implemented
  void operator=(const vtkStreamingExtentTranslator&); // Not implemented
//ETX
};

#endif
