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

#include "vtkDataObjectAlgorithm.h"

class vtkProcessModuleConnection;
class vtkSocketController;

class VTK_EXPORT vtkClientServerMoveData : public vtkDataObjectAlgorithm
{
public:
  static vtkClientServerMoveData* New();
  vtkTypeMacro(vtkClientServerMoveData, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Set the connection on which we are moving the data.
  // If not set, the active remote connection will be used.
  void SetProcessModuleConnection(vtkProcessModuleConnection*);

  // Description:
  // Controls the output type. This is required because processes receiving
  // data cannot know their output type in RequestDataObject without
  // communicating with other processes. Since communicating with other
  // processes in RequestDataObject is dangerous (can cause deadlock because
  // it may happen out-of-sync), the application has to set the output
  // type. The default is VTK_POLY_DATA. Make sure to call this before any
  // pipeline updates occur.
  vtkSetMacro(OutputDataType, int);
  vtkGetMacro(OutputDataType, int);

  // Description:
  // Controls the output WHOLE_EXTENT.  This is required because processes
  // receiving data cannot know their WHOLE_EXTENT in RequestInformation
  // without communicating with other processes. Since communicating with
  // other processes in RequestInformation is dangerous (can cause deadlock
  // because it may happen out-of-sync), the application has to set the
  // output type. Make sure to call this before any pipeline updates occur.
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);
 
  // Description:
  // Optionally, set the process type. If set to AUTO, then the process type is
  // tried to be determined using the active connection.
  vtkSetMacro(ProcessType, int);
  vtkGetMacro(ProcessType, int);
//BTX
  enum ProcessTypes
    {
    AUTO=0,
    SERVER=1,
    CLIENT=2
    };

protected:
  vtkClientServerMoveData();
  ~vtkClientServerMoveData();
 
  // Overridden to mark input as optional, since input data may
  // not be available on all processes that this filter is instantiated.
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Create an output of the type defined by OutputDataType
  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);

  // If there is an input call superclass' RequestInformation
  // otherwise set the output WHOLE_EXTENT() to be WholeExtent
  virtual int RequestInformation(vtkInformation* request, 
                                 vtkInformationVector** inputVector, 
                                 vtkInformationVector* outputVector);

  virtual int SendData(vtkDataObject*, vtkSocketController*);
  virtual vtkDataObject* ReceiveData(vtkSocketController*);


  vtkProcessModuleConnection* ProcessModuleConnection;
  enum Tags {
    TRANSMIT_DATA_OBJECT = 23483
  };

  int OutputDataType;
  int WholeExtent[6];
  int ProcessType;

private:
  vtkClientServerMoveData(const vtkClientServerMoveData&); // Not implemented.
  void operator=(const vtkClientServerMoveData&); // Not implemented.
//ETX
};
