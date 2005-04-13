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
// update sources. Each source proxy has one or more parts (vtkSMPart).
// Each part represents one output of one filter. These are created
// automatically (when CreateParts() is called) by the source.
// Each vtkSMSourceProxy creates a property called DataInformation.
// This property is a composite property that provides information
// about the output(s) of the VTK sources (obtained from the server)
// .SECTION See Also
// vtkSMProxy vtkSMPart vtkSMInputProperty

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
  // Calls UpdateInformation() on all sources (chains to superclass).
  virtual void UpdateInformation();

  // Description:
  // Calls Update() on all sources. It also creates parts if
  // they are not already created.
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
  // one filter and can not apply to the multi-block input.
  void AddInput(vtkSMSourceProxy* input, 
                const char* method,
                int portIdx,
                int hasMultipleInputs);

  // Description:
  // Calls method on all VTK sources. Used by the input property 
  // to remove inputs. Made public to allow access by wrappers. Do
  // not use.
  void CleanInputs(const char* method);

  // Description:
  // Chains to superclass and calls UpdateInformation()
  virtual void UpdateSelfAndAllInputs();

  // Description:
  // Updates data information if necessary and copies information to 
  // the DataInformation property.
  void UpdateDataInformation();

  // Description:
  // Return the number of parts (output proxies).
  unsigned int GetNumberOfParts();

  // Description:
  // Return a part (output proxy).
  vtkSMPart* GetPart(unsigned int idx);

  // Description:
  // Create n parts where n is the number of filters. Each part
  // correspond to one output of one filter.
  void CreateParts();

  // Description:
  // DataInformation is used by the source proxy to obtain information
  // on the output(s) from the server. The information contained in
  // this object is also copied to the DataInformation automatically.
  // The direct use of the data information object is low level and
  // should be avoided if possible.
  vtkPVDataInformation* GetDataInformation();

  // Description:
  // Chains to superclass as well as mark the data information as
  // invalid (next time data information is requested, it will be
  // re-created).
  virtual void MarkConsumersAsModified();

  // Description:
  // Return a property (see superclass documentation). Overwritten
  // to allow DataInformation property specially.
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
  // Internal helper methods used to populate the information property
  // using the information object.
  void ConvertDataInformationToProperty(
    vtkPVDataInformation* info, vtkSMProperty* prop);
  void ConvertFieldDataInformationToProperty(
    vtkPVDataSetAttributesInformation* info, vtkSMProperty* prop);
  void ConvertArrayInformationToProperty(
    vtkPVArrayInformation* info, vtkSMProperty* prop);


  int PartsCreated;


  // Description:
  // Obtain data information from server (does not check if the
  // data information is valid)
  void GatherDataInformation();

  // Description:
  // Mark the data information as invalid.
  void InvalidateDataInformation();

  // Description:
  // Return a property (see superclass documentation). Overwritten
  // to allow DataInformation property specially.
  virtual vtkSMProperty* GetProperty(const char* name, int selfOnly);

  vtkPVDataInformation *DataInformation;
  int DataInformationValid;

  // Description:
  // Call superclass' and then assigns a new executive 
  // (vtkCompositeDataPipeline)
  virtual void CreateVTKObjects(int numObjects);

  char *ExecutiveName;
  vtkSetStringMacro(ExecutiveName);

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

private:
  vtkSMSourceProxyInternals* PInternals;

  vtkSMSourceProxy(const vtkSMSourceProxy&); // Not implemented
  void operator=(const vtkSMSourceProxy&); // Not implemented
};

#endif
