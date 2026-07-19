// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVRMLSource
 * @brief   Wrapper around the vtkVRMLImporter that makes it availalable as a data source.
 *
 * This is a simple wrapper around the vtkVRMLImporter that makes it available as a data
 * source in ParaView.
 */

#ifndef vtkVRMLSource_h
#define vtkVRMLSource_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsIOGeneralModule.h" //needed for exports

class vtkMultiBlockDataSet;
class vtkVRMLImporter;

class VTKPVVTKEXTENSIONSIOGENERAL_EXPORT vtkVRMLSource : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkVRMLSource, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkVRMLSource* New();

  ///@{
  /**
   * Set/get the VRML file name.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  ///@{
  /**
   * Decide whether to generate color arrays or not. By default, this
   * flag is set to on.
   */
  vtkSetMacro(Color, vtkTypeBool);
  vtkGetMacro(Color, vtkTypeBool);
  vtkBooleanMacro(Color, vtkTypeBool);
  ///@}

  ///@{
  /**
   * This method allows all parts to be put into a single output.
   * By default this flag is on.
   */
  vtkSetMacro(Append, vtkTypeBool);
  vtkGetMacro(Append, vtkTypeBool);
  vtkBooleanMacro(Append, vtkTypeBool);
  ///@}

  static int CanReadFile(const char* filename);

protected:
  vtkVRMLSource();
  ~vtkVRMLSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Forwards the file name to the vtkVRMLImporter and calls Update() on it.
  void InitializeImporter();

  // Copies the output of the vtkVRMLImporter to the output of this source. This is
  // where the bulk of the work is done in this class.
  void CopyImporterToOutputs(vtkMultiBlockDataSet*);

private:
  vtkVRMLSource(const vtkVRMLSource&) = delete;
  void operator=(const vtkVRMLSource&) = delete;

  char* FileName = nullptr;
  vtkTypeBool Color = 1;
  vtkTypeBool Append = 1;

  vtkNew<vtkVRMLImporter> Importer;
};

#endif
