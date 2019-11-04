// .NAME vtkDataMineDrillHoleReader from vtkDataMineDrillHoleReader
// .SECTION Description
// vtkDataMineDrillHoleReader is a subclass of vtkPolyDataAlgorithm
// to read DataMine binary Files (point, perimeter, wframe<points/triangle>)

#ifndef vtkDataMineDrillHoleReader_h
#define vtkDataMineDrillHoleReader_h

#include "vtkDataMineReader.h"

class VTKDATAMINEREADERS_EXPORT vtkDataMineDrillHoleReader : public vtkDataMineReader
{
public:
  static vtkDataMineDrillHoleReader* New();
  vtkTypeMacro(vtkDataMineDrillHoleReader, vtkDataMineReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Determine if the file can be readed with this reader.
  int CanReadFile(const char* fname);

protected:
  vtkDataMineDrillHoleReader();
  ~vtkDataMineDrillHoleReader();

  void Read(vtkPoints* points, vtkCellArray* cells) override;
  // submethods depending on file type
  void ParsePoints(vtkPoints* points, vtkCellArray* cells, TDMFile* file, const int& XID,
    const int& YID, const int& ZID, const int& BHID, const int& BHIDSize);

private:
  vtkDataMineDrillHoleReader(const vtkDataMineDrillHoleReader&) = delete;
  void operator=(const vtkDataMineDrillHoleReader&) = delete;
};
#endif
