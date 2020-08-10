/*=========================================================================

  Program:   ParaView
  Module:    vtkNetworkImageWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class  vtkNetworkImageWriter
 * @brief  Helper to write data on client or data-server-root.
 *
 * vtkNetworkImageWriter is designed to be used to write data that is being
 * generated on the client-side either directly on the client or on the
 * data-server root node. This is useful for saving data such as screenshots,
 * state files, etc. that are generated on the client-side but sometimes needs
 * to be saved remotely on the data-server root node instead.
 *
 * To use, simply create the ("misc", "NetworkImageWriter") proxy. The proxy
 * creates this class on client and data-server. Next, using the client-side
 * object, pass the input data to the client-instance and then set
 * `OutputDestination` property as needed. The actual writer to use must be set
 * using the "Writer" property. Now, when you call UpdatePipeline,
 * the vtkNetworkImageWriter will transfer data if needed and then invoke the
 * writer's Write method on the target process.
*/

#ifndef vtkNetworkImageWriter_h
#define vtkNetworkImageWriter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" //needed for exports

class vtkAlgorithm;
class vtkClientServerInterpreter;
class vtkDataObject;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkNetworkImageWriter
  : public vtkDataObjectAlgorithm
{
public:
  static vtkNetworkImageWriter* New();
  vtkTypeMacro(vtkNetworkImageWriter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum FileDestination
  {
    CLIENT = 0x0,
    DATA_SERVER_ROOT = 0x1,
  };

  //@{
  /**
   * Get/Set the Image writer
   */
  void SetWriter(vtkAlgorithm* writer);
  vtkGetObjectMacro(Writer, vtkAlgorithm);
  //@}

  //@{
  /**
   * Get/Set the destination of the image
   */
  vtkSetClampMacro(
    OutputDestination, int, vtkNetworkImageWriter::CLIENT, vtkNetworkImageWriter::DATA_SERVER_ROOT);
  vtkGetMacro(OutputDestination, int);
  //@}

  //@{
  /**
   * Set the interpreter to use to call methods on the writer. Initialized to
   * `vtkClientServerInterpreterInitializer::GetGlobalInterpreter()` in the
   * constructor.
   */
  void SetInterpreter(vtkClientServerInterpreter* interp);
  vtkGetObjectMacro(Interpreter, vtkClientServerInterpreter);
  //@}

protected:
  vtkNetworkImageWriter();
  ~vtkNetworkImageWriter() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Internal method that setup the file name and delegates `Write` in this->Writer
   */
  void WriteLocally(vtkDataObject* input);

  int OutputDestination = CLIENT;
  vtkAlgorithm* Writer = nullptr;
  vtkClientServerInterpreter* Interpreter = nullptr;
};

#endif /* end of include guard: VTKNETWORKIMAGEWRITER_H_3BVHUERT */
