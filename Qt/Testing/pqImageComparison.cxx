/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

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
