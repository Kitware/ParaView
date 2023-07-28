// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkCartisoReader_h
#define vtkCartisoReader_h

#include "vtkAlgorithm.h"
#include "vtkPVAdiosReaderStagingModule.h" // for export macro

class vtkMultiProcessController;

class VTKPVADIOSREADERSTAGING_EXPORT vtkCartisoReader : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkCartisoReader, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkCartisoReader* New();

  ///@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Get/Set the path of the input stream.
   */
  vtkSetStringMacro(StreamName);
  vtkGetStringMacro(StreamName);
  ///@}

  ///@{
  /**
   * Get/Set the time in seconds to wait for input stream to become available.
   * Values < 0 will result in wait forever. Default is 300 seconds.
   */
  vtkSetMacro(TimeOut, double);
  vtkGetMacro(TimeOut, double);
  ///@}

  /**
   * Get the current step in the stream
   */
  vtkGetMacro(Step, int);

  /**
   * True if the stream has ended
   */
  vtkGetMacro(StreamEnded, bool);

  /**
   * Advance to the next available time step
   */
  void AdvanceStep();

protected:
  vtkCartisoReader();
  ~vtkCartisoReader();

  // Usual data generation methods
  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int FillOutputPortInformation(int, vtkInformation*) override;

  void Initialize();
  void Finalize();

  vtkMultiProcessController* Controller;

  char* StreamName;
  float TimeOut;

  int Step;
  bool StreamEnded;

  struct Internals;
  Internals* Internal;

private:
  vtkCartisoReader(const vtkCartisoReader&) = delete;
  void operator=(const vtkCartisoReader&) = delete;
};

#endif // vtkCartisoReader_h
