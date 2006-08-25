/*=========================================================================

   Program: ParaView
   Module:    pqTestUtility.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqFileDialogEventPlayer.h"
#include "pqFileDialogEventTranslator.h"
#include "pqOptions.h"
#include "pqProcessModuleGUIHelper.h"
#include "pqQVTKWidgetEventPlayer.h"
#include "pqQVTKWidgetEventTranslator.h"
#include "pqTestUtility.h"

#include <pqEventDispatcher.h>
#include <pqEventPlayer.h>
#include <pqEventTranslator.h>
#include <pqXMLEventSource.h>

#include <vtkProcessModule.h>
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
#include <vtkPQConfig.h>
#include <vtkRenderWindow.h>
#include <vtkTesting.h>

#include <QDir>
#include <QFileInfo>
#include <QApplication>

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

/////////////////////////////////////////////////////////////////////////////
// pqTestUtility::pqImplementation

class pqTestUtility::pqImplementation
{
public:
  pqImplementation(pqProcessModuleGUIHelper& helper) :
    GUIHelper(helper)
  {
  }

  pqProcessModuleGUIHelper& GUIHelper;
  pqXMLEventSource EventSource;
  pqEventPlayer EventPlayer;
  pqEventDispatcher EventDispatcher;
};

/////////////////////////////////////////////////////////////////////////////
// pqTestUtility

pqTestUtility::pqTestUtility(pqProcessModuleGUIHelper& helper, QObject* p) :
  QObject(p),
  Implementation(new pqImplementation(helper))
{
  this->Setup(this->Implementation->EventPlayer);
  
  QObject::connect(
    &this->Implementation->EventDispatcher,
    SIGNAL(succeeded()),
    this,
    SLOT(testSucceeded()));
    
  QObject::connect(
    &this->Implementation->EventDispatcher,
    SIGNAL(failed()),
    this,
    SLOT(testFailed()));
}

pqTestUtility::~pqTestUtility()
{
  delete this->Implementation;
}

void pqTestUtility::Setup(pqEventTranslator& translator)
{
  translator.addWidgetEventTranslator(new pqQVTKWidgetEventTranslator());
  translator.addWidgetEventTranslator(new pqFileDialogEventTranslator());
  translator.addDefaultWidgetEventTranslators();
}

void pqTestUtility::Setup(pqEventPlayer& player)
{
  player.addWidgetEventPlayer(new pqQVTKWidgetEventPlayer());
  player.addWidgetEventPlayer(new pqFileDialogEventPlayer());
  player.addDefaultWidgetEventPlayers();
}

QString pqTestUtility::DataRoot()
{
  // Let the user override the defaults by setting an environment variable ...
  QString result = getenv("PARAVIEW_DATA_ROOT");
  
  // Otherwise, go with the compiled-in default ...
  if(result.isEmpty())
    {
    result = PARAVIEW_DATA_ROOT;
    }
  
  // Ensure all slashes face forward ...
  result.replace('\\', '/');
  
  // Remove any trailing slashes ...
  if(result.size() && result.at(result.size()-1) == '/')
    {
    result.chop(1);
    }
    
  // Trim excess whitespace ...
  result = result.trimmed();
    
  return result;
}

bool pqTestUtility::SaveScreenshot(vtkRenderWindow* RenderWindow, const QString& File)
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

bool pqTestUtility::CompareImage(vtkRenderWindow* RenderWindow, 
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
  
void pqTestUtility::runTests()
{
  if(pqOptions* const options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions()))
    {
    if(options->GetTestFileName())
      {
      this->Implementation->EventSource.setContent(options->GetTestFileName());
      this->Implementation->EventDispatcher.playEvents(
        this->Implementation->EventSource,
        this->Implementation->EventPlayer);
      }
    }
}

void pqTestUtility::testSucceeded()
{
  if(pqOptions* const options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions()))
    {
    // TODO: image comparisons probably ought to be done the same
    //       way widget validation is done (when that gets implemented)
    //       That is, check that the text of a QLineEdit is a certain value
    //       Referencing a QVTKWidget can then be done the same way as referencing
    //       any other widget, instead of relying on the "active" view.
    bool comparison_succeeded = true;
    if(options->GetBaselineImage())
      {
      comparison_succeeded = this->Implementation->GUIHelper.compareView(options->GetBaselineImage(),
        options->GetImageThreshold(), cout, options->GetTestDirectory());
      }
      
    if(options->GetExitAppWhenTestsDone())
      {
      QApplication::instance()->exit(comparison_succeeded ? 0 : 1);
      }
    }
}

void pqTestUtility::testFailed()
{
  if(pqOptions* const options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions()))
    {
    if(options->GetExitAppWhenTestsDone())
      {
      QApplication::instance()->exit(1);
      }
    }
}
