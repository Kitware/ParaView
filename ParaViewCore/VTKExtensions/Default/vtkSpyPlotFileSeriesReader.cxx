/*=========================================================================

  Program:   ParaView
  Module:    vtkSpyPlotFileSeriesReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2014 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkSpyPlotFileSeriesReader.h"

#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//=============================================================================
vtkStandardNewMacro(vtkSpyPlotFileSeriesReader);

vtkSpyPlotFileSeriesReader::vtkSpyPlotFileSeriesReader () 
{
  this->SetNumberOfOutputPorts(2);
#ifdef PARAVIEW_ENABLE_SPYPLOT_MARKERS
  this->SetNumberOfOutputPorts(3);
#endif // PARAVIEW_ENABLE_SPYPLOT_MARKERS
}

vtkSpyPlotFileSeriesReader::~vtkSpyPlotFileSeriesReader () 
{
}

void vtkSpyPlotFileSeriesReader::PrintSelf (ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotFileSeriesReader::RequestInformationForInput(
                                             int index,
                                             vtkInformation *request,
                                             vtkInformationVector *outputVector)
{
  if ((index != this->_FileIndex) || (outputVector != NULL))
    {
    if (this->GetNumberOfFileNames () > 0)
      {
      this->ReaderSetFileName(this->GetFileName(index));
      }
    else
      {
      this->ReaderSetFileName(0);
      }

    this->_FileIndex = index;
    // Need to call RequestInformation on reader to refresh any metadata for the
    // new filename.
    vtkSmartPointer<vtkInformation> tempRequest;
    if (request)
      {
      tempRequest = request;
      }
    else
      {
      tempRequest = vtkSmartPointer<vtkInformation>::New();
      tempRequest->Set(vtkDemandDrivenPipeline::REQUEST_INFORMATION());
      }
    vtkSmartPointer<vtkInformationVector> tempOutputVector;
    if (outputVector)
      {
      tempOutputVector = outputVector;
      }
    else
      {
      tempOutputVector = vtkSmartPointer<vtkInformationVector>::New();
      VTK_CREATE(vtkInformation, tempOutputInfo0);
      VTK_CREATE(vtkInformation, tempOutputInfo1);
      tempOutputVector->Append(tempOutputInfo0);
      tempOutputVector->Append(tempOutputInfo1);
#ifdef PARAVIEW_ENABLE_SPYPLOT_MARKERS
      VTK_CREATE(vtkInformation, tempOutputInfo2);
      tempOutputVector->Append(tempOutputInfo2);
#endif // PARAVIEW_ENABLE_SPYPLOT_MARKERS
      }
    return this->Reader->ProcessRequest(tempRequest,
                                        (vtkInformationVector**)NULL,
                                        tempOutputVector);
    }
  return 1;
}
