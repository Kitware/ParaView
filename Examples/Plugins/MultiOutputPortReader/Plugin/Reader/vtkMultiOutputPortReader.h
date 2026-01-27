#ifndef vtkMultiOutputPortReader_h
#define vtkMultiOutputPortReader_h

#include "MultiOutputPortReaderCoreModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

#include <string>

/**
 * @class vtkMultiOutputPortReader
 * @brief Example reader with 2 output ports to demonstrate vtkFileSeriesReader
 *
 * This reader demonstrates multi-output-port support with vtkFileSeriesReader.
 * It reads simple .mopr files containing a single time value, and
 * generates two meshes:
 *   - Port 0: Square mesh with f(t,x,y) = sin(2*pi*(x + y + t))
 *   - Port 1: Circle mesh with f(t,x,y) = cos(2*pi*(r + t)) where r = sqrt(x^2+y^2)
 *
 * File format: Single line containing a floating-point time value.
 *
 * This example demonstrates how vtkFileSeriesReader automatically detects
 * the number of output ports from the internal reader and configures itself
 * to match, enabling file series support for multi-output-port readers.
 */
class MULTIOUTPUTPORTREADERCORE_EXPORT vtkMultiOutputPortReader
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkMultiOutputPortReader* New();
  vtkTypeMacro(vtkMultiOutputPortReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the file name.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  /**
   * Test whether the file can be read.
   */
  int CanReadFile(const char* filename);

protected:
  vtkMultiOutputPortReader();
  ~vtkMultiOutputPortReader() override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMultiOutputPortReader(const vtkMultiOutputPortReader&) = delete;
  void operator=(const vtkMultiOutputPortReader&) = delete;

  char* FileName;

  // Read the time value from file
  bool ReadTimeValue(const char* filename, double& time);

  // Generate the square mesh for port 0
  vtkSmartPointer<vtkDataObject> GenerateSquareMesh(double time);

  // Generate the circle mesh for port 1
  vtkSmartPointer<vtkDataObject> GenerateCircleMesh(double time);
};

#endif
