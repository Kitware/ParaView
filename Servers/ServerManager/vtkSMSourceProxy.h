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
// .NAME vtkSMSourceProxy - proxy for a VTK source on a server
// .SECTION Description
// vtkSMSourceProxy manages VTK source(s) that are created on a server
// using the proxy pattern. In addition to functionality provided
// by vtkSMProxy, vtkSMSourceProxy provides method to connect and
// update sources.
// .SECTION See Also
// vtkSMProxy vtkSMPart

#ifndef __vtkSMSourceProxy_h
#define __vtkSMSourceProxy_h

#include "vtkSMProxy.h"
#include "vtkClientServerID.h" // Needed for ClientServerID

class vtkPVDataInformation;
//BTX
struct vtkSMSourceProxyInternals;
//ETX
class vtkSMPart;
class vtkSMProperty;

class VTK_EXPORT vtkSMSourceProxy : public vtkSMProxy
{
public:
  static vtkSMSourceProxy* New();
  vtkTypeRevisionMacro(vtkSMSourceProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Calls UpdateInformation() on all sources. In future, this
  // will also populate some information properties.
  void UpdateInformation();

  // Description:
  // Calls Update() on all sources.  In future, this will also 
  // populate some information properties.
  void UpdatePipeline();

  // Description:
  // Connects filters/sinks to an input. If the filter(s) is not
  // created, this will create it. If hasMultipleInputs is
  // true,  only one filter is created, even if the input has
  // multiple parts. All the inputs are added using the method
  // name provided. If hasMultipleInputs is not true, one filter
  // is created for each input. NOTE: The filter(s) is created
  // when SetInput is called the first and if the it wasn't already
  // created. If the filter has two inputs and one is multi-block
  // whereas the other one is not, SetInput() should be called with
  // the multi-block input first. Otherwise, it will create only
  // on filter and can not apply to the multi-block input.
  void AddInput(vtkSMSourceProxy* input, 
                const char* method,
                int hasMultipleInputs);

  // Description:
  void CleanInputs(const char* method);

  // Description:
  virtual void UpdateSelfAndAllInputs();

protected:
  vtkSMSourceProxy();
  ~vtkSMSourceProxy();

//BTX
  friend class vtkSMInputProperty;
//ETX


  // Description:
  // Create n parts where n is the number of filters. Each part
  // correspond to one output of one filter.
  void CreateParts();
  
  // Description:
  void ConvertDataInformationToProperty(
    vtkPVDataInformation* info, vtkSMProperty* prop);

  int GetNumberOfParts();
  vtkSMPart* GetPart(int idx);

//   // Description:
//   // Connectivity methods. Manage inputs and consumers. These methods
//   // do not invoke anything on the managed sources.
//   // Consumer info is not used yet.
//   void AddConsumer(vtkSMSourceProxy *c);
//   void RemoveConsumer(vtkSMSourceProxy *c);
//   int IsConsumer(vtkSMSourceProxy *c);
//   vtkSMSourceProxy *GetConsumer(int i);

  int PartsCreated;

private:
  vtkSMSourceProxyInternals* PInternals;

  vtkSMSourceProxy(const vtkSMSourceProxy&); // Not implemented
  void operator=(const vtkSMSourceProxy&); // Not implemented
};

#endif
