/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPart.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPart - proxy for a data object
// .SECTION Description
// This object manages one output port of an algorithm. It is used
// internally by vtkSMSourceProxy to manage all of it's outputs.

#ifndef __vtkSMPart_h
#define __vtkSMPart_h

#include "vtkSMProxy.h"
#include "vtkClientServerStream.h" // needed for SetupUpdateExtent
class vtkPVClassNameInformation;
class vtkPVDataInformation;
class vtkSMPartDisplay;
class vtkSMDisplay;
class vtkCollection;

class VTK_EXPORT vtkSMPart : public vtkSMProxy
{
public:
  static vtkSMPart* New();
  vtkTypeRevisionMacro(vtkSMPart, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns data information. If data information is marked
  // invalid, calls GatherDataInformation.
  vtkPVDataInformation* GetDataInformation();

  // Description:
  // Returns classname information.
  vtkPVClassNameInformation* GetClassNameInformation();

  // Description:
  // Get the classname of the dataset from server.
  void GatherClassNameInformation();

  // Description:
  // Get information about dataset from server.
  void GatherDataInformation(int doUpdate=1);

  // Description:
  // Mark data information as invalid.
  void InvalidateDataInformation();

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
  vtkSMProxy* GetDataObjectProxy(int recheck);

//BTX
  // Description:
  // Returns the client/server id of the output pointed by this part.
  vtkClientServerID GetAlgorithmOutputID()
    {
      return this->GetID(0);
    }

  // Description:
  // Returns the client/server id of the producer that has the output
  // of this part.
  vtkClientServerID GetProducerID()
    {
      return this->GetID(1);
    }

  // Description:
  // Returns the client/server id of the executive that has the output
  // of this part.
  vtkClientServerID GetExecutiveID()
    {
      return this->GetID(2);
    }
//ETX

protected:
  vtkSMPart();
  ~vtkSMPart();

  vtkSMProxy* DataObjectProxy;

private:
  vtkSMPart(const vtkSMPart&); // Not implemented
  void operator=(const vtkSMPart&); // Not implemented

//BTX
  friend class vtkSMSourceProxy;
  void UpdatePipeline();

  // The index of the port the output is obtained from.
  vtkSetMacro(PortIndex, int);
  int PortIndex;

  void SetAlgorithmOutputID(vtkClientServerID id)
    {
      this->SetID(0, id);
    }
  void SetProducerID(vtkClientServerID id)
    {
      this->SetID(1, id);
    }
  void SetExecutiveID(vtkClientServerID id)
    {
      this->SetID(2, id);
    }

  // Description:
  // Insert a filter to extract (and redistribute) unstructured
  // pieces if the source cannot generate pieces.
  void InsertExtractPiecesIfNecessary();

  // Description:
  // Replace the default extent translator with vtkPVExtentTranslator.
  void CreateTranslatorIfNecessary();

  vtkPVClassNameInformation* ClassNameInformation;
  int ClassNameInformationValid;
  vtkPVDataInformation* DataInformation;
  int DataInformationValid;
//ETX

};

#endif
