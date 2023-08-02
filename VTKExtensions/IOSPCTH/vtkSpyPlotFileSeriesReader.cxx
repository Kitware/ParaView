// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright 2014 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkSpyPlotFileSeriesReader.h"

#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//=============================================================================
vtkStandardNewMacro(vtkSpyPlotFileSeriesReader);

vtkSpyPlotFileSeriesReader::vtkSpyPlotFileSeriesReader()
{
  this->SetNumberOfOutputPorts(2);
#ifdef PARAVIEW_ENABLE_SPYPLOT_MARKERS
  this->SetNumberOfOutputPorts(3);
#endif // PARAVIEW_ENABLE_SPYPLOT_MARKERS
}

vtkSpyPlotFileSeriesReader::~vtkSpyPlotFileSeriesReader() = default;

void vtkSpyPlotFileSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
