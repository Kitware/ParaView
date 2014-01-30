/*=========================================================================

  Program:   ParaView
  Module:    vtkSMOutputPort.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMOutputPort - reference for an output port of a vtkAlgorithm.
// .SECTION Description
// This object manages one output port of an algorithm. It is used
// internally by vtkSMSourceProxy to manage all of it's outputs.
// .SECTION Notes
// Previously, vtkSMOutputPort used to be a vtkSMProxy subclass since it was
// indeed a Proxy for the output port. However that has now changed. This merely
// sits as a datastructure to manage ports specific things like
// data-information. However for backwards compatibility, to keep the impact
// minimal, we leave this as a sub-class of a Proxy with GlobalID=0 and
// Session=NULL.

#ifndef __vtkSMOutputPort_h
#define __vtkSMOutputPort_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMProxy.h"
#include "vtkWeakPointer.h" // needed by SourceProxy pointer

class vtkCollection;
class vtkPVClassNameInformation;
class vtkPVDataInformation;
class vtkPVTemporalDataInformation;
class vtkSMCompoundSourceProxy;
class vtkSMSourceProxy;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMOutputPort : public vtkSMProxy
{
public:
  static vtkSMOutputPort* New();
  vtkTypeMacro(vtkSMOutputPort, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns data information. If data information is marked
  // invalid, calls GatherDataInformation.
  // If data information is gathered then this fires the
  // vtkCommand::UpdateInformationEvent event.
  virtual vtkPVDataInformation* GetDataInformation();

  // Description:
  // Returns data information collected over all timesteps provided by the
  // pipeline. If the data information is not valid, this results iterating over
  // the pipeline and hence can be slow. Use with caution.
  virtual vtkPVTemporalDataInformation* GetTemporalDataInformation();

  // Description:
  // Returns the classname of the data object on this output port.
  virtual const char* GetDataClassName();

  // Description:
  // Returns classname information.
  virtual vtkPVClassNameInformation* GetClassNameInformation();

  // Description:
  // Mark data information as invalid.
  virtual void InvalidateDataInformation();

  // Description:
  // Returns the index of the port the output is obtained from.
  vtkGetMacro(PortIndex, int);

  // Description:
  // Provides access to the source proxy to which the output port belongs.
  vtkSMSourceProxy* GetSourceProxy();

//BTX
protected:
  vtkSMOutputPort();
  ~vtkSMOutputPort();

  // Description:
  // Get the classname of the dataset from server.
  virtual void GatherClassNameInformation();

  // Description:
  // Get information about dataset from server.
  // Fires the vtkCommand::UpdateInformationEvent event.
  virtual void GatherDataInformation();

  // Description:
  // Get temporal information from the server.
  virtual void GatherTemporalDataInformation();

  void SetSourceProxy(vtkSMSourceProxy* src);

  // When set to non-null, GetSourceProxy() returns this rather than the real
  // source-proxy set using SetSourceProxy(). This provides a mechanism for
  // vtkSMCompoundSourceProxy to take ownership of ports that don't really
  // belong to it.
  void SetCompoundSourceProxy(vtkSMCompoundSourceProxy* src);

  // Description:
  // An internal update pipeline method that subclasses may override.
  virtual void UpdatePipelineInternal(double time, bool doTime);

  // The index of the port the output is obtained from.
  vtkSetMacro(PortIndex, int);
  int PortIndex;

  vtkWeakPointer<vtkSMSourceProxy> SourceProxy;
  vtkWeakPointer<vtkSMCompoundSourceProxy> CompoundSourceProxy;

  vtkPVClassNameInformation* ClassNameInformation;
  int ClassNameInformationValid;
  vtkPVDataInformation* DataInformation;
  bool DataInformationValid;

  vtkPVTemporalDataInformation* TemporalDataInformation;
  bool TemporalDataInformationValid;

private:
  vtkSMOutputPort(const vtkSMOutputPort&); // Not implemented
  void operator=(const vtkSMOutputPort&); // Not implemented

  friend class vtkSMSourceProxy;
  friend class vtkSMCompoundSourceProxy;
  void UpdatePipeline();

  // Update Pipeline with the given timestep request.
  void UpdatePipeline(double time);
//ETX
};

#endif
