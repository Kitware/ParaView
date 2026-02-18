#ifndef vtkMultiOutputPortReader_h
#define vtkMultiOutputPortReader_h

#include "MultiOutputPortReaderCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

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
class MULTIOUTPUTPORTREADERCORE_EXPORT vtkMultiOutputPortReader : public vtkPolyDataAlgorithm
{
public:
  static vtkMultiOutputPortReader* New();
  vtkTypeMacro(vtkMultiOutputPortReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Specify the file name.
   */
  void SetFileName(const std::string& fname);

protected:
  vtkMultiOutputPortReader();
  ~vtkMultiOutputPortReader() override = default;

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMultiOutputPortReader(const vtkMultiOutputPortReader&) = delete;
  void operator=(const vtkMultiOutputPortReader&) = delete;

  bool ReadTimeValue(const char* filename, double& time);
  vtkSmartPointer<vtkDataObject> GenerateSquareMesh(double time);
  vtkSmartPointer<vtkDataObject> GenerateCircleMesh(double time);

  std::string FileName;
};

#endif
