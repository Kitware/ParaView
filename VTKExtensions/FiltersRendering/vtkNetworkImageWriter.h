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
 * @brief  This ImageWriter lets you write images either in the client-side
 *         or in the server-side, as specified in the OutputDestination field.
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
  enum FileDestination
  {
    CLIENT = 0x0,
    SERVER = 0x1,
  };

  static vtkNetworkImageWriter* New();
  vtkTypeMacro(vtkNetworkImageWriter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
    OutputDestination, int, vtkNetworkImageWriter::CLIENT, vtkNetworkImageWriter::SERVER);
  vtkGetMacro(OutputDestination, int);
  //@}

  //@{
  /**
   * Set the interpreter to use to call methods on the writer.
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
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  /**
   * Internal method that setup the file name and delegates `Write` in this->Writer
   */
  void WriteLocally(vtkDataObject* input);

  int OutputDestination = CLIENT;

  vtkAlgorithm* Writer = nullptr;
  vtkClientServerInterpreter* Interpreter = nullptr;
};

#endif /* end of include guard: VTKNETWORKIMAGEWRITER_H_3BVHUERT */
