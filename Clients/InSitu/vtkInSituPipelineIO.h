/*=========================================================================

  Program:   ParaView
  Module:    vtkInSituPipelineIO.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkInSituPipelineIO
 * @brief insitu pipeline for IO
 *
 * vtkInSituPipelineIO is a hard-coded pipeline that can be used to save out data using writers
 * supported by ParaView.
 *
 */

#ifndef vtkInSituPipelineIO_h
#define vtkInSituPipelineIO_h

#include "vtkInSituPipeline.h"
#include "vtkPVInSituModule.h" // for exports
#include "vtkSmartPointer.h"   // for vtkSmartPointer

#include <string> // for std::string

class vtkSMSourceProxy;

class VTKPVINSITU_EXPORT vtkInSituPipelineIO : public vtkInSituPipeline
{
public:
  static vtkInSituPipelineIO* New();
  vtkTypeMacro(vtkInSituPipelineIO, vtkInSituPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the filename.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get/Set the channel name.
   */
  vtkSetStringMacro(ChannelName);
  vtkGetStringMacro(ChannelName);
  //@}

  //@{
  /**
   * vtkInSituPipeline API implementaton
   */
  bool Initialize() override;
  bool Execute(int timestep, double time) override;
  bool Finalize() override;
  //@}

  /**
   * Helper function to format a filename using current timestep and
   * time.
   */
  virtual std::string GetCurrentFileName(const char* fname, int timestep, double time);

protected:
  vtkInSituPipelineIO();
  ~vtkInSituPipelineIO();

private:
  vtkInSituPipelineIO(const vtkInSituPipelineIO&) = delete;
  void operator=(const vtkInSituPipelineIO&) = delete;

  char* FileName;
  char* ChannelName;
  vtkSmartPointer<vtkSMSourceProxy> Writer;
};

#endif
