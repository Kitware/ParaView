// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkGmshWriter
 *
 * Writer for the Gmsh format. Write an unstructured grid as an ASCII msh file.
 * It supports export of data arrays as well as temporal data.
 * The teomporal change of topology is however not supported.
 */

#ifndef vtkGmshWriter_h
#define vtkGmshWriter_h

#include "vtkGmshIOModule.h" // for export macro
#include "vtkWriter.h"

struct GmshWriterInternal;
class vtkUnstructuredGrid;
class vtkDataSetAttributes;

VTK_ABI_NAMESPACE_BEGIN
class VTKGMSHIO_EXPORT vtkGmshWriter : public vtkWriter
{
public:
  static vtkGmshWriter* New();
  vtkTypeMacro(vtkGmshWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the file name to be written.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify if we write data arrays starting by "gmsh" (usually created by the Gmsh reader).
   */
  vtkSetMacro(WriteGmshSpecificArray, bool);
  vtkGetMacro(WriteGmshSpecificArray, bool);
  ///@}

  ///@{
  /**
   * Specify if all timesteps have to be saved for temporal data.
   */
  vtkSetMacro(WriteAllTimeSteps, bool);
  vtkGetMacro(WriteAllTimeSteps, bool);
  ///@}

  ///@{
  /**
   * Get the input of this writer as an unstructured grid.
   */
  vtkUnstructuredGrid* GetInput();
  vtkUnstructuredGrid* GetInput(int port);
  ///@}

  ///@{
  /**
   * Get/Set the name of the elementary entity ID cell field if there is one
   */
  vtkGetStringMacro(ElementaryEntityIDFieldName);
  vtkSetStringMacro(ElementaryEntityIDFieldName);
  ///@}

  ///@{
  /**
   * Get/Set the name of the physical group ID cell field if there is one
   */
  vtkGetStringMacro(PhysicalGroupIDFieldName);
  vtkSetStringMacro(PhysicalGroupIDFieldName);
  ///@}

protected:
  vtkGmshWriter();
  ~vtkGmshWriter() override;

  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void WriteData() override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  char* FileName = nullptr;
  bool WriteAllTimeSteps = false;
  bool WriteGmshSpecificArray = false;

  char* ElementaryEntityIDFieldName = nullptr;
  char* PhysicalGroupIDFieldName = nullptr;

private:
  void SetUpEntities();
  bool SetUpPhysicalGroups();
  void LoadNodes();
  void LoadCells();
  void InitViews();
  void LoadNodeData();
  void LoadCellData();
  void LoadData(vtkDataSetAttributes* data, const char* type);

  GmshWriterInternal* Internal;

  vtkGmshWriter(const vtkGmshWriter&) = delete;
  void operator=(const vtkGmshWriter&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif // vtkGmshWriter_h
