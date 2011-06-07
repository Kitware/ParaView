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
// .NAME vtkMultiClientMPIMoveData - a helper class that enables using
// vtkMPIMoveData for delivering data to multiple clients.
// .SECTION Description
// vtkMultiClientMPIMoveData is a helper class that makes it possible to deliver
// data to multiple clients. It uses vtkMPIMoveData filters internally. Note
// that this is not an algorithm, that's because we wanted to avoid repeating
// unnecessary actions e.g. is data was delivered to client A, we must have
// gathered the data to root already, we don't need gather the data again to
// deliver to client B.
//
// .SECTION Implementation Details
// This class uses 3 vtkMPIMoveData instances.
// \li PassThroughMoveData :- used in pass-through modes i.e. when delivering
// data to render-server nodes.
// \li ServerMoveData :- used to collect data to data-server root node
// \li ClientMoveData :- used to take data from data-server root node to client.

#ifndef __vtkMultiClientMPIMoveData_h
#define __vtkMultiClientMPIMoveData_h

#include "vtkObject.h"

class vtkDataObject;
class vtkInformation;
class vtkMPIMoveData;

class VTK_EXPORT vtkMultiClientMPIMoveData : public vtkObject
{
public:
  static vtkMultiClientMPIMoveData* New();
  vtkTypeMacro(vtkMultiClientMPIMoveData, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Sets the output data type on all vtkMPIMoveData instances.
  void SetOutputDataType(int);

  // Description:
  // Clears all state about data location. Next time we request Delivery(), this
  // helper will operate from scratch. Typically called when the data has
  // changed.
  void Reset();

  // Description:
  // Process the view request.
  // Note that this may affect the MTime of internal delivery filters.
  void ProcessViewRequest(vtkInformation*);

  // Description:
  // Request the actual delivery. Note that this may not result in any real data
  // transfer.
  void Deliver(vtkDataObject* input, vtkDataObject* output);

  // Description:
  // Get MTime.
  virtual unsigned long GetMTime();

  // Description:
  // Get/Set LOD Mode. This identifies is this class is used in a LOD pipeline.
  vtkSetMacro(LODMode, bool);
  vtkGetMacro(LODMode, bool);
//BTX
protected:
  vtkMultiClientMPIMoveData();
  ~vtkMultiClientMPIMoveData();

  vtkMPIMoveData* PassThroughMoveData;
  vtkMPIMoveData* ServerMoveData;
  vtkMPIMoveData* ClientMoveData;

  bool LODMode;
  bool UsePassThrough;

  // flag that keeps track of where the data has been "moved" since the last
  // Modified.
  int DataMoveState;
  enum
    {
    NO_WHERE = 0,
    PASS_THROUGH = 0x01,
    COLLECT_TO_ROOT = 0x02,
    };

private:
  vtkMultiClientMPIMoveData(const vtkMultiClientMPIMoveData&); // Not implemented
  void operator=(const vtkMultiClientMPIMoveData&); // Not implemented

  class VoidPtrSet;
  VoidPtrSet* HandledClients;
//ETX
};

#endif
