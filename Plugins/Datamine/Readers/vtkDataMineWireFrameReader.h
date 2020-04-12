// .NAME vtkDataMineWireFrameReader from vtkDataMineWireFrameReader
// .SECTION Description
// vtkDataMineWireFrameReader is a subclass of vtkPolyDataAlgorithm
// to read DataMine binary Files (point, perimeter, wframe<points/triangle>)

#ifndef vtkDataMineWireFrameReader_h
#define vtkDataMineWireFrameReader_h

#include "vtkDataMineReader.h"

class VTKDATAMINEREADERS_EXPORT vtkDataMineWireFrameReader : public vtkDataMineReader
{
public:
  static vtkDataMineWireFrameReader* New();
  vtkTypeMacro(vtkDataMineWireFrameReader, vtkDataMineReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // have to define both for compiler, on basic version call parent
  void SetCellArrayStatus(const char* name, int status) override;

  void SetFileName(const char* filename) override { this->SetFileName(filename, true, invalid); };
  void SetFileName(const char* filename, const bool& update, FileTypes filetype);

  vtkSetStringMacro(PointFileName);
  vtkGetStringMacro(PointFileName);

  void SetTopoFileName(const char* filename);
  vtkGetStringMacro(TopoFileName);

  void SetStopeSummaryFileName(const char* filename);
  vtkGetStringMacro(StopeSummaryFileName);

  vtkSetMacro(UseStopeSummary, int);
  vtkGetMacro(UseStopeSummary, int);

  // Description:
  // Determine if the file can be readed with this reader.
  int CanReadFile(const char* fname);

protected:
  vtkDataMineWireFrameReader();
  ~vtkDataMineWireFrameReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void UpdateDataSelection() override;
  virtual void SetupDataSelection(TDMFile* dmFile, vtkDataArraySelection* old);

  // check of the files
  int TopoFileBad();
  int PointFileBad();
  int StopeFileBad();

  // finds a file path based on the input. Used to find the other file for topo/triangle
  // was code in RI, but moved to a method to reduce duplication
  bool FindAndSetFilePath(std::string& dmExt, const bool& update, FileTypes type);

  void Read(vtkPoints* points, vtkCellArray* cells) override;
  void ReadPoints(vtkPoints* points);
  void ReadCells(vtkCellArray* cells);

  void ParsePoints(vtkPoints* points, TDMFile* file, const int& PID, const int& XID, const int& YID,
    const int& ZID);
  void ParseCells(vtkCellArray* cells, TDMFile* file, const int& P1, const int& P2, const int& P3);
  void ParseCellsWithStopes(vtkCellArray* cells, TDMFile* file, TDMFile* stopeFile, const int& P1,
    const int& P2, const int& P3, const int& stopeId);

  /// read stope file, and create mapping
  bool PopulateStopeMap();

  // user defined filenames
  char* PointFileName;
  char* TopoFileName;

  // special extra file info for Stope Summary Support
  char* StopeSummaryFileName;
  int UseStopeSummary;

  // needed for stop summary files to determine the index to stope id map
  PointMap* StopeFileMap;

private:
  vtkDataMineWireFrameReader(const vtkDataMineWireFrameReader&) = delete;
  void operator=(const vtkDataMineWireFrameReader&) = delete;
};
#endif
