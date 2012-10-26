/*=========================================================================

  Program:   ParaView
  Module:    vtkPVQuadViewInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVQuadViewInformation
// .SECTION Description
// Information class used to retreive annotation information from the server side

#ifndef __vtkPVQuadViewInformation_h
#define __vtkPVQuadViewInformation_h

#include "vtkPVInformation.h"

class vtkClientServerStream;
class vtkMultiProcessStream;

class vtkPVQuadViewInformation : public vtkPVInformation
{
public:
  static vtkPVQuadViewInformation* New();
  vtkTypeMacro(vtkPVQuadViewInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  virtual void Initialize();

  //BTX
  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // Serialize/Deserialize the parameters that control how/what information is
  // gathered. This are different from the ivars that constitute the gathered
  // information itself. For example, PortNumber on vtkPVDataInformation
  // controls what output port the data-information is gathered from.
  virtual void CopyParametersToStream(vtkMultiProcessStream&);
  virtual void CopyParametersFromStream(vtkMultiProcessStream&);
  //ETX

  vtkGetStringMacro(XLabel);
  vtkGetStringMacro(YLabel);
  vtkGetStringMacro(ZLabel);
  vtkGetStringMacro(ScalarLabel);
  vtkGetVector4Macro(Values, double);

protected:
  vtkPVQuadViewInformation();
  ~vtkPVQuadViewInformation();

  vtkSetStringMacro(XLabel);
  vtkSetStringMacro(YLabel);
  vtkSetStringMacro(ZLabel);
  vtkSetStringMacro(ScalarLabel);

  char* XLabel;
  char* YLabel;
  char* ZLabel;
  char* ScalarLabel;
  double Values[4];

  vtkPVQuadViewInformation(const vtkPVQuadViewInformation&); // Not implemented
  void operator=(const vtkPVQuadViewInformation&); // Not implemented
};

#endif
