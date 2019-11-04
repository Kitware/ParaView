// .NAME DataMineDummyReader from vtkDataMineDummyReader
// .SECTION Description
// vtkDataMineDummyReader is a subclass of vtkPolyDataAlgorithm
// to read DataMine binary Files that we currently do not support

#ifndef vtkDataMineDummyReader_h
#define vtkDataMineDummyReader_h

#include "vtkDatamineReadersModule.h" // for exports
#include "vtkPolyDataAlgorithm.h"
class VTKDATAMINEREADERS_EXPORT vtkDataMineDummyReader : public vtkPolyDataAlgorithm
{
public:
  static vtkDataMineDummyReader* New();
  vtkTypeMacro(vtkDataMineDummyReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  int CanReadFile(const char* fname);

protected:
  vtkDataMineDummyReader();
  ~vtkDataMineDummyReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  char* FileName;

private:
  vtkDataMineDummyReader(const vtkDataMineDummyReader&) = delete;
  void operator=(const vtkDataMineDummyReader&) = delete;
};
#endif
