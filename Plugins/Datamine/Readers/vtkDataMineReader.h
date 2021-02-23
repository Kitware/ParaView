// .NAME DataMineReader from vtkDataMineReader
// .SECTION Description
// vtkDataMineReader is a subclass of vtkPolyDataAlgorithm
// to read DataMine binary Files (point, perimeter, wframe<points/triangle>)

#ifndef vtkDataMineReader_h
#define vtkDataMineReader_h

#include "ThirdParty/dmfile.h"        // for dmfile
#include "vtkDataArraySelection.h"    // for vtkDataArraySelection
#include "vtkDatamineReadersModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPolyData;
class vtkCallbackCommand;
class vtkPoints;
class vtkCellArray;
class PropertyStorage;
class PointMap;

class VTKDATAMINEREADERS_EXPORT vtkDataMineReader : public vtkPolyDataAlgorithm
{
public:
  static vtkDataMineReader* New();
  vtkTypeMacro(vtkDataMineReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);
  vtkSetObjectMacro(CellDataArraySelection, vtkDataArraySelection);
  int GetCellArrayStatus(const char* name);

  virtual void SetCellArrayStatus(const char* name, int status);

  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int index);

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkDataMineReader();
  ~vtkDataMineReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // The observer to modify this object when the array selections are
  // modified.
  vtkCallbackCommand* SelectionObserver;
  static void SelectionModifiedCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  vtkDataArraySelection* CellDataArraySelection;
  int SetFieldDataInfo(vtkDataArraySelection* eDSA, int association, int numTuples,
    vtkInformationVector*(&infoVector));
  void SetupOutputInformation(vtkInformation* outInfo);
  virtual void UpdateDataSelection();

  // cleaning output data
  virtual void CleanData(vtkPolyData* preClean, vtkPolyData* output);

  // checks and see if the file is a valid format
  virtual int CanRead(const char* fname, FileTypes type);

  // DM file reading methods
  virtual void Read(vtkPoints* /*points*/, vtkCellArray* /*cells*/){};
  virtual void ParseProperties(Data* values);

  // returns true if the property was created
  virtual bool AddProperty(char* varname, const int& pos, const bool& numeric, int numRecords);
  virtual void SegmentProperties(const int& records);

  // internal storage
  PointMap* PointMapping;
  PropertyStorage* Properties;

  // user defined filenames
  char* FileName;

  // number of possible properties in file
  int PropertyCount;

  // the output mode type;
  VTKCellType CellMode;

private:
  vtkDataMineReader(const vtkDataMineReader&) = delete;
  void operator=(const vtkDataMineReader&) = delete;
};
#endif
