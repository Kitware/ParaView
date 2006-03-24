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

bool pqImageComparison::CompareImage(vtkRenderWindow* RenderWindow, const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory)
{
  // Verify the reference image exists
  if(!QFileInfo(ReferenceImage).exists())
    {
    Output << "<DartMeasurement name=\"ImageNotFound\" type=\"text/string\">";
    Output << ReferenceImage.toAscii().data();
    Output << "</DartMeasurement>" << endl;

    return false;
    }

  // Load the reference image
  vtkSmartPointer<vtkPNGReader> reference_image = vtkSmartPointer<vtkPNGReader>::New();
  reference_image->SetFileName(ReferenceImage.toAscii().data()); 
  reference_image->Update();
  
  // Get a screenshot
  vtkSmartPointer<vtkWindowToImageFilter> screenshot = vtkSmartPointer<vtkWindowToImageFilter>::New();
  screenshot->SetInput(RenderWindow);
  screenshot->Update();

  // Compute the difference between the reference image and the screenshot
  vtkSmartPointer<vtkImageDifference> difference = vtkSmartPointer<vtkImageDifference>::New();
  difference->SetInput(screenshot->GetOutput());
  difference->SetImage(reference_image->GetOutput());
  difference->Update();

  Output << "<DartMeasurement name=\"ImageError\" type=\"numeric/double\">";
  Output << difference->GetThresholdedError();
  Output << "</DartMeasurement>" << endl;

  Output << "<DartMeasurement name=\"ImageThreshold\" type=\"numeric/double\">";
  Output << Threshold;
  Output << "</DartMeasurement>" << endl;

  // If the difference didn't exceed the threshold, we're done
  if(difference->GetThresholdedError() <= Threshold)
    return true;

  // Write the reference image to a file
  const QString reference_file = QDir(TempDirectory).filePath(QFileInfo(ReferenceImage).baseName() + ".reference.png");
  vtkSmartPointer<vtkPNGWriter> reference_writer = vtkSmartPointer<vtkPNGWriter>::New();
  reference_writer->SetInput(reference_image->GetOutput());
  reference_writer->SetFileName(reference_file.toAscii().data());
  reference_writer->Write();

  // Write the screenshot to a file
  const QString screenshot_file = QDir(TempDirectory).filePath(QFileInfo(ReferenceImage).baseName() + ".screenshot.png");
  vtkSmartPointer<vtkPNGWriter> screenshot_writer = vtkSmartPointer<vtkPNGWriter>::New();
  screenshot_writer->SetInput(screenshot->GetOutput());
  screenshot_writer->SetFileName(screenshot_file.toAscii().data());
  screenshot_writer->Write();

  // Write the difference to a file, increasing the contrast to make discrepancies stand out
  vtkSmartPointer<vtkImageShiftScale> scale_image = vtkSmartPointer<vtkImageShiftScale>::New();
  scale_image->SetShift(0);
  scale_image->SetScale(10);
  scale_image->SetInput(difference->GetOutput());
  
  const QString difference_file = QDir(TempDirectory).filePath(QFileInfo(ReferenceImage).baseName() + ".difference.png");
  vtkSmartPointer<vtkPNGWriter> difference_writer = vtkSmartPointer<vtkPNGWriter>::New();
  difference_writer->SetInput(scale_image->GetOutput());
  difference_writer->SetFileName(difference_file.toAscii().data());
  difference_writer->Write();

  Output << "<DartMeasurementFile name=\"ValidImage\" type=\"image/jpeg\">";
  Output << reference_file.toAscii().data();
  Output << "</DartMeasurementFile>" << endl;
 
  Output << "<DartMeasurementFile name=\"TestImage\" type=\"image/jpeg\">";
  Output << screenshot_file.toAscii().data();
  Output << "</DartMeasurementFile>" << endl;
  
  Output << "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/jpeg\">";
  Output << difference_file.toAscii().data();
  Output << "</DartMeasurementFile>" << endl;

  return false;
}
