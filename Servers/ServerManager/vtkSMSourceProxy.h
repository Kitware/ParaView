/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSourceProxy -
// .SECTION Description

#ifndef __vtkSMSourceProxy_h
#define __vtkSMSourceProxy_h

#include "vtkSMProxy.h"
#include "vtkClientServerID.h" // Needed for ClientServerID

//BTX
struct vtkSMSourceProxyInternals;
//ETX
class vtkSMPart;

class VTK_EXPORT vtkSMSourceProxy : public vtkSMProxy
{
public:
  static vtkSMSourceProxy* New();
  vtkTypeRevisionMacro(vtkSMSourceProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  void SetInput(int idx, vtkSMSourceProxy* input);

  // Description:
  void SetInput(int idx, vtkSMSourceProxy* input, const char* method);

  // Description:
  void UpdateInformation();

  // Description:
  void Update();

  // Description:
  vtkSetMacro(HasMultipleInputs, int);
  vtkGetMacro(HasMultipleInputs, int);

protected:
  vtkSMSourceProxy();
  ~vtkSMSourceProxy();

  // Description:
  void CreateParts();

  // Description:
  int GetNumberOfParts();

  // Description:
  vtkSMPart* GetPart(int idx);

  void AddConsumer(vtkSMSourceProxy *c);
  void RemoveConsumer(vtkSMSourceProxy *c);
  int IsConsumer(vtkSMSourceProxy *c);
  vtkSMSourceProxy *GetConsumer(int i);
  void SetNumberOfInputs(int num);
  void SetNthInput(int idx, vtkSMSourceProxy *sp);

  int PartsCreated;

  vtkSMSourceProxy **Inputs;
  int NumberOfInputs; 

  vtkSMSourceProxy **Consumers;
  int NumberOfConsumers;

  int HasMultipleInputs;

private:
  vtkSMSourceProxyInternals* PInternals;

  vtkSMSourceProxy(const vtkSMSourceProxy&); // Not implemented
  void operator=(const vtkSMSourceProxy&); // Not implemented
};

#endif
