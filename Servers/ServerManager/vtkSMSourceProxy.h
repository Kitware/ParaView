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

class vtkPVArrayInformation;
class vtkPVDataInformation;
class vtkPVDataSetAttributesInformation;
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
  // Calls UpdateInformation() on all sources.
  virtual void UpdateInformation();

  // Description:
  // Calls Update() on all sources. 
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

  // Description:
  void UpdateDataInformation();

  // Description:
  unsigned int GetNumberOfParts();

  // Description:
  vtkSMPart* GetPart(unsigned int idx);

  // Description:
  // Create n parts where n is the number of filters. Each part
  // correspond to one output of one filter.
  void CreateParts();

//BTX
  // Description:
  vtkPVDataInformation* GetDataInformation();
//ETX

  // Description:
  virtual void MarkConsumersAsModified();

  // Description:
  vtkSMProperty* GetProperty(const char* name) 
    {
      return this->Superclass::GetProperty(name);
    }
  
protected:
  vtkSMSourceProxy();
  ~vtkSMSourceProxy();

//BTX
  friend class vtkSMInputProperty;
//ETX

  // Description:
  void ConvertDataInformationToProperty(
    vtkPVDataInformation* info, vtkSMProperty* prop);
  void ConvertFieldDataInformationToProperty(
    vtkPVDataSetAttributesInformation* info, vtkSMProperty* prop);
  void ConvertArrayInformationToProperty(
    vtkPVArrayInformation* info, vtkSMProperty* prop);


  int PartsCreated;


  // Description:
  void GatherDataInformation();

  // Description:
  void InvalidateDataInformation();

  // Description:
  virtual vtkSMProperty* GetProperty(const char* name, int selfOnly);

  vtkPVDataInformation *DataInformation;
  int DataInformationValid;

private:
  vtkSMSourceProxyInternals* PInternals;

  vtkSMSourceProxy(const vtkSMSourceProxy&); // Not implemented
  void operator=(const vtkSMSourceProxy&); // Not implemented
};

#endif
