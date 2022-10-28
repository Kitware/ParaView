/*=========================================================================

  Program:   ParaView
  Module:    vtkPrismSESAMEFileSeriesReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPrismSESAMEFileSeriesReader
 * @brief   read a series of files in the SESAME format
 *
 * vtkPrismSESAMEFileSeriesReader is a subclass of vtkFileSeriesReader that
 * sets the number of output ports to 2.
 */
#ifndef vtkPrismSESAMEFileSeriesReader_h
#define vtkPrismSESAMEFileSeriesReader_h

#include "vtkFileSeriesReader.h"
#include "vtkPrismReadersModule.h" // needed for exports

class VTKPRISMREADERS_EXPORT vtkPrismSESAMEFileSeriesReader : public vtkFileSeriesReader
{
public:
  static vtkPrismSESAMEFileSeriesReader* New();
  vtkTypeMacro(vtkPrismSESAMEFileSeriesReader, vtkFileSeriesReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPrismSESAMEFileSeriesReader();
  ~vtkPrismSESAMEFileSeriesReader() override;

private:
  vtkPrismSESAMEFileSeriesReader(const vtkPrismSESAMEFileSeriesReader&) = delete;
  void operator=(const vtkPrismSESAMEFileSeriesReader&) = delete;
};

#endif // vtkPrismSESAMEFileSeriesReader_h
