/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerMoveData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClientServerMoveData - Moves data from the server root node
// to the client.
// .SECTION Description
// This class moves all the input data available at the input on the root
// server node to the client node. If not in server-client mode,
// this filter behaves as a simple pass-through filter. 
// This can work with any data type, the application does not need to set
// the output type before hand.
// .SECTION Warning
// This filter may change the output in RequestData().

#include "vtkDataSetAlgorithm.h"

class vtkProcessModuleConnection;
class vtkSocketController;

class VTK_EXPORT vtkClientServerMoveData : public vtkDataSetAlgorithm
{
public:
  static vtkClientServerMoveData* New();
  vtkTypeRevisionMacro(vtkClientServerMoveData, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Set the connection on which we are moving the data.
  // Thus must not be set on the render server only nodes at all.
  void SetProcessModuleConnection(vtkProcessModuleConnection*);
protected:
  vtkClientServerMoveData();
  ~vtkClientServerMoveData();
 
  // Overridden to mark input as optional, since input data may
  // not be available on all processes that this filter is instantiated.
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // Overridden to create output of datatype obtained from the server.
  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);


  int SendData(vtkSocketController* controller, vtkDataObject* data);
  vtkDataObject* ReceiveData(vtkSocketController* controller);

  vtkProcessModuleConnection* ProcessModuleConnection;
  //BTX
  enum Tags {
    COMMUNICATION_TAG = 23483,
    COMMUNICATION_CLASSNAME_LENGTH = 23484,
    COMMUNICATION_CLASSNAME = 23485,
    COMMUNICATION_DATA = 23486,
    COMMUNICATION_DATA_LENGTH = 23487
  };
  //ETX
private:
  vtkClientServerMoveData(const vtkClientServerMoveData&); // Not implemented.
  void operator=(const vtkClientServerMoveData&); // Not implemented.

};
