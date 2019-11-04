// .NAME DataMinePointReader from vtkDataMineBlockReader
// .SECTION Description
// vtkDataMineBlockReader is a subclass of vtkPolyDataAlgorithm
// to read DataMine binary Files (point, perimeter, wframe<points/triangle>)

#ifndef vtkDataMineBlockReader_h
#define vtkDataMineBlockReader_h

#include "vtkDataMineReader.h"
#include "vtkDatamineReadersModule.h" // for export macro

class VTKDATAMINEREADERS_EXPORT vtkDataMineBlockReader : public vtkDataMineReader
{
public:
  static vtkDataMineBlockReader* New();
  vtkTypeMacro(vtkDataMineBlockReader, vtkDataMineReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Determine if the file can be readed with this reader.
  int CanReadFile(const char* fname);

protected:
  vtkDataMineBlockReader();
  ~vtkDataMineBlockReader() override;

  void Read(vtkPoints* points, vtkCellArray* cells) override;
  // submethods depending on file type
  void ParsePoints(vtkPoints* points, vtkCellArray* cells, TDMFile* file, const int& XID,
    const int& YID, const int& ZID);

private:
  vtkDataMineBlockReader(const vtkDataMineBlockReader&) = delete;
  void operator=(const vtkDataMineBlockReader&) = delete;
};
#endif
