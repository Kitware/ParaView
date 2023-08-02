// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
