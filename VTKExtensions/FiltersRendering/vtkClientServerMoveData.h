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
/**
 * @class   vtkClientServerMoveData
 * @brief   Moves data from the server root node
 * to the client.
 *
 * This class moves all the input data available at the input on the root
 * server node to the client node. If not in server-client mode,
 * this filter behaves as a simple pass-through filter.
 * This can work with any data type, the application does not need to set
 * the output type before hand.
 * @warning
 * This filter may change the output in RequestData().
*/

#ifndef vtkClientServerMoveData_h
#define vtkClientServerMoveData_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" //needed for exports

class vtkMultiProcessController;
class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkClientServerMoveData
  : public vtkDataObjectAlgorithm
{
public:
  static vtkClientServerMoveData* New();
  vtkTypeMacro(vtkClientServerMoveData, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Controls the output type. This is required because processes receiving
   * data cannot know their output type in RequestDataObject without
   * communicating with other processes. Since communicating with other
   * processes in RequestDataObject is dangerous (can cause deadlock because
   * it may happen out-of-sync), the application has to set the output
   * type. The default is VTK_POLY_DATA. Make sure to call this before any
   * pipeline updates occur.
   */
  vtkSetMacro(OutputDataType, int);
  vtkGetMacro(OutputDataType, int);
  //@}

  //@{
  /**
   * Controls the output WHOLE_EXTENT.  This is required because processes
   * receiving data cannot know their WHOLE_EXTENT in RequestInformation
   * without communicating with other processes. Since communicating with
   * other processes in RequestInformation is dangerous (can cause deadlock
   * because it may happen out-of-sync), the application has to set the
   * output type. Make sure to call this before any pipeline updates occur.
   */
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);
  //@}

  //@{
  /**
   * Optionally, set the process type. If set to AUTO, then the process type is
   * tried to be determined using the active connection.
   */
  vtkSetMacro(ProcessType, int);
  vtkGetMacro(ProcessType, int);
  //@}

  //@{
  /**
   * Get/Set the controller to use. This is optional and needed only when
   * ProcessType is set to something other than AUTO. If AUTO, then the
   * controller is obtained from the active session.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  enum ProcessTypes
  {
    AUTO = 0,
    SERVER = 1,
    CLIENT = 2
  };

protected:
  vtkClientServerMoveData();
  ~vtkClientServerMoveData() override;

  // Overridden to mark input as optional, since input data may
  // not be available on all processes that this filter is instantiated.
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Create an output of the type defined by OutputDataType
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // If there is an input call superclass' RequestInformation
  // otherwise set the output WHOLE_EXTENT() to be WholeExtent
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  virtual int SendData(vtkDataObject*, vtkMultiProcessController*);
  virtual vtkDataObject* ReceiveData(vtkMultiProcessController*);

  enum Tags
  {
    TRANSMIT_DATA_OBJECT = 23483
  };

  int OutputDataType;
  int WholeExtent[6];
  int ProcessType;
  vtkMultiProcessController* Controller;

private:
  vtkClientServerMoveData(const vtkClientServerMoveData&) = delete;
  void operator=(const vtkClientServerMoveData&) = delete;
};

#endif
