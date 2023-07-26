// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

// Hide PARAVIEW_DEPRECATED_IN_5_12_0() warnings for this class.
#define PARAVIEW_DEPRECATION_LEVEL 0

#include "vtkPVOptions.h"

#include "vtkLegacy.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptionsXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConfiguration.h"
#include "vtkRemotingCoreConfiguration.h"

#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVOptions);

//----------------------------------------------------------------------------
vtkPVOptions::vtkPVOptions()
{
  this->SetProcessType(ALLPROCESS);
  if (this->XMLParser)
  {
    this->XMLParser->Delete();
    this->XMLParser = nullptr;
  }
  this->XMLParser = vtkPVOptionsXMLParser::New();
  this->XMLParser->SetPVOptions(this);
}

//----------------------------------------------------------------------------
vtkPVOptions::~vtkPVOptions() = default;

//----------------------------------------------------------------------------
void vtkPVOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
