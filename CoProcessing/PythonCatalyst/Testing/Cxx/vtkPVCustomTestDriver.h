/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCustomTestDriver.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVCustomTestDriver
 * @brief   A custom test driver code that uses ParaView and python.
 *
 * A custom test driver that creates a vtkUniformGrid with a single
 * scalar point field named "Pressure".  It runs a python script
 * using ParaView.
*/

#ifndef vtkPVCustomTestDriver_h
#define vtkPVCustomTestDriver_h

#include "vtkCPTestDriver.h"

class vtkCPProcessor;

class VTK_EXPORT vtkPVCustomTestDriver : public vtkCPTestDriver
{
public:
  static vtkPVCustomTestDriver* New();
  vtkTypeMacro(vtkPVCustomTestDriver, vtkCPTestDriver);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Run the test driver with the coprocessor.
   * Returns 0 if there were no errors.
   */
  virtual int Run() override;

  /**
   * Initialize the driver with the coprocessor.  fileName is the
   * name of the python script.  Returns 0 on failure.
   */
  virtual int Initialize(const char* fileName);

  /**
   * Finalize the driver with the coprocessor.
   */
  virtual int Finalize();

protected:
  vtkPVCustomTestDriver();
  ~vtkPVCustomTestDriver();

private:
  vtkPVCustomTestDriver(const vtkPVCustomTestDriver&) = delete;
  void operator=(const vtkPVCustomTestDriver&) = delete;

  //@{
  /**
   * The coprocessor to be called by this custom test.
   */
  vtkCPProcessor* Processor;
};
//@}

#endif
