// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) 2006 University Corporation for Atmospheric Research
// SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-EULA-VAPOR
// .NAME vtkVDFReader - class for reader Vapor Data Format files
// .Section Description
// vtkVDFReader uses the Vapor library to read wavelet compressed VDF files
// and produce VTK data image data. The resolution of the image data is
// controlled by the refinement parameter.
//

#ifndef vtkVDFReader_h
#define vtkVDFReader_h

#include "vtkImageAlgorithm.h"
#include "vtkVaporReadersModule.h" // for export macro

#include "vapor/DataMgr.h"                    //needed for vapor datastructures
#include "vapor/WaveletBlock3DRegionReader.h" //needed for vapor datastructures

#include <map>    //needed for protected ivars
#include <string> //needed for protected ivars
#include <vector> //needed for protected ivars

// using namespace std;
using namespace VAPoR;

class VTKVAPORREADERS_EXPORT vtkVDFReader : public vtkImageAlgorithm
{
public:
  static vtkVDFReader* New();
  vtkTypeMacro(vtkVDFReader, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Choose file to read
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Check file for suitability to this reader
  int CanReadFile(const char* fname);

  // Description:
  // Set resolution within range provided by data.
  void SetRefinement(int);
  vtkGetMacro(Refinement, int);
  vtkGetVector2Macro(RefinementRange, int);

  // Description:
  void SetVariableType(int);
  vtkGetMacro(VariableType, int);

  // Description:
  void SetCacheSize(int);
  vtkGetMacro(CacheSize, int);

  // Description:
  // Choose which arrays to load
  int GetPointArrayStatus(const char*);
  void SetPointArrayStatus(const char*, int);
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int);

  vtkVDFReader(const vtkVDFReader&) = delete;
  void operator=(const vtkVDFReader&) = delete;

protected:
  vtkVDFReader();
  ~vtkVDFReader();
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int FillTimeSteps();

  char* FileName;

  Metadata* vdc_md;
  DataMgr* data_mgr;
  VDFIOBase* vdfiobase;

  bool is_layered;
  int height_factor;
  int num_levels;
  size_t ext_p[3];
  int Refinement;
  int VariableType;
  int CacheSize;
  double* TimeSteps;
  int nTimeSteps;
  int TimeStep;
  int RefinementRange[2];

  std::map<std::string, int> data;
  std::vector<double> uExt;
  std::vector<std::string> current_var_list;

private:
  vtkVDFReader(const vtkVDFReader&);
  void operator=(const vtkVDFReader&);
  bool extentsMatch(int*, int*);
  void GetVarDims(size_t*, size_t*);
  void SetExtents(vtkInformation*);
};
#endif
