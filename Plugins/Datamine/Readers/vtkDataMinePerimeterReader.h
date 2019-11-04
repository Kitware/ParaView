// .NAME DataMinePointReader from vtkDataMinePerimeterReader
// .SECTION Description
// vtkDataMinePerimeterReader is a subclass of vtkPolyDataAlgorithm
// to read DataMine binary Files (point, perimeter, wframe<points/triangle>)

#ifndef vtkDataMinePerimeterReader_h
#define vtkDataMinePerimeterReader_h

#include "vtkDataMineReader.h"

class VTKDATAMINEREADERS_EXPORT vtkDataMinePerimeterReader : public vtkDataMineReader
{
public:
  static vtkDataMinePerimeterReader* New();
  vtkTypeMacro(vtkDataMinePerimeterReader, vtkDataMineReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Determine if the file can be readed with this reader.
  int CanReadFile(const char* fname);

protected:
  vtkDataMinePerimeterReader();
  ~vtkDataMinePerimeterReader() override;

  void Read(vtkPoints* points, vtkCellArray* cells) override;
  // submethods depending on file type
  void ParsePoints(vtkPoints* points, vtkCellArray* cells, TDMFile* file, const int& XID,
    const int& YID, const int& ZID, const int& PTN, const int& PV);

private:
  vtkDataMinePerimeterReader(const vtkDataMinePerimeterReader&) = delete;
  void operator=(const vtkDataMinePerimeterReader&) = delete;
};
#endif
