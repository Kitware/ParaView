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
// .NAME vtkSMOutputPort - proxy for an output port of a vtkAlgorithm.
// .SECTION Description
// This object manages one output port of an algorithm. It is used
// internally by vtkSMSourceProxy to manage all of it's outputs.
// .SECTION Notes
// This class was called vtkSMPart in earlier versions.

#ifndef __vtkSMOutputPort_h
#define __vtkSMOutputPort_h

#include "vtkSMProxy.h"
#include "vtkClientServerStream.h" // needed for SetupUpdateExtent
class vtkPVClassNameInformation;
class vtkPVDataInformation;
class vtkCollection;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMOutputPort : public vtkSMProxy
{
public:
  static vtkSMOutputPort* New();
  vtkTypeRevisionMacro(vtkSMOutputPort, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns data information. If data information is marked
  // invalid, calls GatherDataInformation.
  // If data information is gathered then this fires the 
  // vtkCommand::UpdateInformationEvent event.
  virtual vtkPVDataInformation* GetDataInformation();

  // Description:
  // Returns the classname of the data object on this output port.
  virtual const char* GetDataClassName();

  // Description:
  // Returns classname information.
  virtual vtkPVClassNameInformation* GetClassNameInformation();

  // Description:
  // Get the classname of the dataset from server.
  virtual void GatherClassNameInformation();

  // Description:
  // Get information about dataset from server.
  // Fires the vtkCommand::UpdateInformationEvent event.
  virtual void GatherDataInformation(int doUpdate=1);

  // Description:
  // Mark data information as invalid.
  virtual void InvalidateDataInformation();

  // Description:
  // Returns the index of the port the output is obtained from.
  vtkGetMacro(PortIndex, int);

  // Description:
  // Returns the proxy pointing to a data object produced by the port
  // represented by the part. The first time this method is called, an id
  // is assigned to the current output.  After this first call, the server
  // is not contacted again unless recheck is true. If recheck is false,
  // the proxy may point to the wrong data object (if the output of the
  // algorithm changed for some reason). Be very careful using this proxy,
  // if the output of the algorithm changed for some reason, this proxy
  // will be pointing to the old output until GetDataObjectProxy() is
  // called with recheck=1. This has two consequences: 1. a reference to
  // the old data object is hold and the data object is not released 2.
  // GetDataObjectProxy(0) may return the wrong data object.
  virtual vtkSMProxy* GetDataObjectProxy(int recheck);

//BTX
  // Description:
  // Returns the client/server id of the producer that has the output
  // of this part.
  vtkClientServerID GetProducerID()
    {
      return this->ProducerID;
    }

  // Description:
  // Returns the client/server id of the executive that has the output
  // of this part.
  vtkClientServerID GetExecutiveID()
    {
      return this->ExecutiveID;
    }
//ETX

  // Description:
  // This method saves state information about the proxy
  // which can be used to revive the proxy using server side objects
  // already present. This includes the entire state saved by calling 
  // SaveState() as well additional information such as server side
  // object IDs.
  // Overridden to save information pertinant to reviving the output ports.
  virtual vtkPVXMLElement* SaveRevivalState(vtkPVXMLElement* root);
  virtual int LoadRevivalState(vtkPVXMLElement* revivalElement);

  // Description:
  // Provides access to the source proxy to which the output port belongs.
  vtkSMSourceProxy* GetSourceProxy()
    { return this->SourceProxy; }

protected:
  vtkSMOutputPort();
  ~vtkSMOutputPort();

  void SetSourceProxy(vtkSMSourceProxy* src)
    { this->SourceProxy = src; }

  // Description:
  // An internal update pipeline method that subclasses may override.
  virtual void UpdatePipelineInternal(double time, bool doTime);

  vtkSMProxy* DataObjectProxy;
  vtkClientServerID ProducerID;
  vtkClientServerID ExecutiveID;

  // The index of the port the output is obtained from.
  vtkSetMacro(PortIndex, int);
  int PortIndex;

  vtkSMSourceProxy* SourceProxy;

  vtkPVClassNameInformation* ClassNameInformation;
  int ClassNameInformationValid;
  vtkPVDataInformation* DataInformation;
  bool DataInformationValid;

private:
  vtkSMOutputPort(const vtkSMOutputPort&); // Not implemented
  void operator=(const vtkSMOutputPort&); // Not implemented

//BTX
  friend class vtkSMSourceProxy;
  void UpdatePipeline();
  
  // Update Pipeline with the given timestep request.
  void UpdatePipeline(double time);

  void InitializeWithIDs(vtkClientServerID outputID, 
                         vtkClientServerID producerID, 
                         vtkClientServerID executiveID);

  // Description:
  // Insert a filter to extract (and redistribute) unstructured
  // pieces if the source cannot generate pieces.
  void InsertExtractPiecesIfNecessary();

  // Description:
  // Replace the default extent translator with vtkPVExtentTranslator.
  void CreateTranslatorIfNecessary();
//ETX

};

#endif
