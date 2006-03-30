/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqImageComparison.h"

#include <vtkWindowToImageFilter.h>
#include <vtkBMPWriter.h>
#include <vtkTIFFWriter.h>
#include <vtkPNMWriter.h>
#include <vtkPNGWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkErrorCode.h>
#include <vtkSmartPointer.h>
#include <vtkPNGReader.h>
#include <vtkImageDifference.h>
#include <vtkImageShiftScale.h>
#include <vtkRenderWindow.h>
#include <vtkTesting.h>

#include <QDir>
#include <QFileInfo>

template<typename WriterT>
bool saveImage(vtkWindowToImageFilter* Capture, const QFileInfo& File)
{
  WriterT* const writer = WriterT::New();
  writer->SetInput(Capture->GetOutput());
  writer->SetFileName(File.filePath().toAscii().data());
  writer->Write();
  const bool result = writer->GetErrorCode() == vtkErrorCode::NoError;
  writer->Delete();
  
  return result;
}

bool pqImageComparison::SaveScreenshot(vtkRenderWindow* RenderWindow, const QString& File)
{
  vtkWindowToImageFilter* const capture = vtkWindowToImageFilter::New();
  capture->SetInput(RenderWindow);
  capture->Update();

  bool success = false;
  
  const QFileInfo file(File);
  if(file.completeSuffix() == "bmp")
    success = saveImage<vtkBMPWriter>(capture, file);
  else if(file.completeSuffix() == "tif")
    success = saveImage<vtkTIFFWriter>(capture, file);
  else if(file.completeSuffix() == "ppm")
    success = saveImage<vtkPNMWriter>(capture, file);
  else if(file.completeSuffix() == "png")
    success = saveImage<vtkPNGWriter>(capture, file);
  else if(file.completeSuffix() == "jpg")
    success = saveImage<vtkJPEGWriter>(capture, file);
    
  capture->Delete();
  
  return success;
}

bool pqImageComparison::CompareImage(vtkRenderWindow* RenderWindow, 
  const QString& ReferenceImage, double Threshold, 
  ostream& vtkNotUsed(Output), const QString& TempDirectory)
{
  vtkSmartPointer<vtkTesting> testing = vtkSmartPointer<vtkTesting>::New();
  testing->AddArgument("-T");
  testing->AddArgument(TempDirectory.toAscii().data());
  testing->AddArgument("-V");
  testing->AddArgument(ReferenceImage.toAscii().data());
  testing->SetRenderWindow(RenderWindow);
  if (testing->RegressionTest(Threshold) == vtkTesting::PASSED)
    {
    return true;
    }
  return false;
}
