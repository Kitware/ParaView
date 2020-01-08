/*=========================================================================

  Program:   ParaView
  Module:    vtkCPXMLPWriterPipeline.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCPXMLPWriterPipeline_h
#define vtkCPXMLPWriterPipeline_h

#include "vtkPVCatalystModule.h" // For windows import/export of shared libraries
#include <string>                // For Path member variable
#include <vtkCPPipeline.h>

/// @ingroup CoProcessing
/// Generic PXML writer pipeline to write out the full Catalyst
/// input datasets. The filename will correspond to the input
/// name/channel identifier with time step and file extension
/// (e.g. "input_0.pvtu" for an unstructured dataset with no
/// padding).
class VTKPVCATALYST_EXPORT vtkCPXMLPWriterPipeline : public vtkCPPipeline
{
public:
  static vtkCPXMLPWriterPipeline* New();
  vtkTypeMacro(vtkCPXMLPWriterPipeline, vtkCPPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int RequestDataDescription(vtkCPDataDescription* dataDescription) override;

  int CoProcess(vtkCPDataDescription* dataDescription) override;

  /// Set the output frequency for this pipeline. The default is 1.
  vtkSetClampMacro(OutputFrequency, int, 1, VTK_INT_MAX);
  vtkGetMacro(OutputFrequency, int);

  /// Set the padding amount for the time index portion of the generated files.
  /// The default is 0.
  vtkSetClampMacro(PaddingAmount, int, 1, 10);
  vtkGetMacro(PaddingAmount, int);

  /// Set the path to the generated files.
  vtkSetMacro(Path, std::string);
  vtkGetMacro(Path, std::string);

protected:
  vtkCPXMLPWriterPipeline();
  virtual ~vtkCPXMLPWriterPipeline();

private:
  vtkCPXMLPWriterPipeline(const vtkCPXMLPWriterPipeline&) = delete;
  void operator=(const vtkCPXMLPWriterPipeline&) = delete;

  int OutputFrequency;
  int PaddingAmount;
  std::string Path;
};
#endif
