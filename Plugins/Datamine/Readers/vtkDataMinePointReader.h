// .NAME DataMinePointReader from vtkDataMinePointReader
// .SECTION Description
// vtkDataMinePointReader is a subclass of vtkPolyDataAlgorithm
// to read DataMine binary Files (point, perimeter, wframe<points/triangle>)

#ifndef vtkDataMinePointReader_h
#define vtkDataMinePointReader_h

#include "vtkDataMineReader.h"

class VTKDATAMINEREADERS_EXPORT vtkDataMinePointReader : public vtkDataMineReader
{
public:
  static vtkDataMinePointReader* New();
  vtkTypeMacro(vtkDataMinePointReader, vtkDataMineReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Determine if the file can be readed with this reader.
  int CanReadFile(const char* fname);

protected:
  vtkDataMinePointReader();
  ~vtkDataMinePointReader();

  void Read(vtkPoints* points, vtkCellArray* cells) override;
  // submethods depending on file type
  void ParsePoints(vtkPoints* points, vtkCellArray* cells, TDMFile* file, const int& XID,
    const int& YID, const int& ZID);

private:
  vtkDataMinePointReader(const vtkDataMinePointReader&) = delete;
  void operator=(const vtkDataMinePointReader&) = delete;
};
#endif
