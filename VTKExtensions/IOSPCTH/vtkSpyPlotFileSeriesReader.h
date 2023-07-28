// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright 2014 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkSpyPlotFileSeriesReader_h
#define vtkSpyPlotFileSeriesReader_h

#include "vtkFileSeriesReader.h"
#include "vtkPVVTKExtensionsIOSPCTHModule.h" //needed for exports

/// vtkSpyPlotFileSeriesReader extends vtkFileSeriesReader to change the number
/// of output ports on this reader. Based on whether markers support was
/// enabled, this reader will have 2 or 3 output ports.
class VTKPVVTKEXTENSIONSIOSPCTH_EXPORT vtkSpyPlotFileSeriesReader : public vtkFileSeriesReader
{
public:
  static vtkSpyPlotFileSeriesReader* New();
  vtkTypeMacro(vtkSpyPlotFileSeriesReader, vtkFileSeriesReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSpyPlotFileSeriesReader();
  ~vtkSpyPlotFileSeriesReader() override;

private:
  vtkSpyPlotFileSeriesReader(const vtkSpyPlotFileSeriesReader&) = delete;
  void operator=(const vtkSpyPlotFileSeriesReader&) = delete;
};

#endif /* vtkSpyPlotFileSeriesReader_h */
