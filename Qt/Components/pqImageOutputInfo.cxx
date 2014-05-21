/*=========================================================================

   Program: ParaView
   Module:    pqSGExportStateWizard.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
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

========================================================================*/
#include "pqImageOutputInfo.h"

#include <pqView.h>

#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPNGWriter.h>
#include <vtkSmartPointer.h>
#include <vtkUnsignedCharArray.h>
#include <vtksys/SystemTools.hxx>

#include "ui_pqImageOutputInfo.h"


//-----------------------------------------------------------------------------
pqImageOutputInfo::pqImageOutputInfo(
  QWidget *parentObject, Qt::WindowFlags parentFlags,
  pqView* view, QString& viewName):
  QWidget(parentObject, parentFlags),
  Info(new Ui::ImageOutputInfo()),
  View(view)
{
  this->Info->setupUi(this);

  this->Info->imageFileName->setText(viewName);
  QObject::connect(
    this->Info->imageFileName, SIGNAL(editingFinished()),
    this, SLOT(updateImageFileName()));

  QObject::connect(
    this->Info->imageType, SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(updateImageFileNameExtension(const QString&)));

  this->setupScreenshotInfo();
};

//-----------------------------------------------------------------------------
pqImageOutputInfo::~pqImageOutputInfo()
{

}

//-----------------------------------------------------------------------------
pqView* pqImageOutputInfo::getView()
{
  return this->View;
}

//-----------------------------------------------------------------------------
QString pqImageOutputInfo::getImageFileName()
{
  return this->Info->imageFileName->displayText();
}
//-----------------------------------------------------------------------------
void pqImageOutputInfo::hideFrequencyInput()
{
  this->Info->imageWriteFrequency->hide();
  this->Info->imageWriteFrequencyLabel->hide();
}

//-----------------------------------------------------------------------------
void pqImageOutputInfo::showFrequencyInput()
{
  this->Info->imageWriteFrequency->show();
    this->Info->imageWriteFrequencyLabel->show();
}
//-----------------------------------------------------------------------------
void pqImageOutputInfo::hideFitToScreen()
{
  this->Info->fitToScreen->hide();
}

//-----------------------------------------------------------------------------
void pqImageOutputInfo::showFitToScreen()
{
  this->Info->fitToScreen->show();
}
//-----------------------------------------------------------------------------
int pqImageOutputInfo::getWriteFrequency()
{
  return this->Info->imageWriteFrequency->value();
}

//-----------------------------------------------------------------------------
bool pqImageOutputInfo::fitToScreen()
{
  return this->Info->fitToScreen->isChecked();
}

//-----------------------------------------------------------------------------
int pqImageOutputInfo::getMagnification()
{
  return this->Info->imageMagnification->value();
}

//-----------------------------------------------------------------------------
void pqImageOutputInfo::updateImageFileName()
{
  QString fileName = this->Info->imageFileName->displayText();
  if(fileName.isNull() || fileName.isEmpty())
    {
    fileName = "image";
    }
  QRegExp regExp("\\.(png|bmp|ppm|tif|tiff|jpg|jpeg)$");
  if(fileName.contains(regExp) == 0)
    {
    fileName.append(".");
    fileName.append(this->Info->imageType->currentText());
    }
  else
    {  // update imageType if it is different
    int extensionIndex = fileName.lastIndexOf(".");
    QString anExtension = fileName.right(fileName.size()-extensionIndex-1);
    int index = this->Info->imageType->findText(anExtension);
    this->Info->imageType->setCurrentIndex(index);
    fileName = this->Info->imageFileName->displayText();
    }

  if(fileName.contains("%t") == 0)
    {
    fileName.insert(fileName.lastIndexOf("."), "_%t");
    }

  this->Info->imageFileName->setText(fileName);
}

//-----------------------------------------------------------------------------
void pqImageOutputInfo::updateImageFileNameExtension(
  const QString& fileExtension)
{
  QString displayText = this->Info->imageFileName->text();
  std::string newFileName = vtksys::SystemTools::GetFilenameWithoutExtension(
    displayText.toLocal8Bit().constData());

  newFileName.append(".");
  newFileName.append(fileExtension.toLocal8Bit().constData());
  this->Info->imageFileName->setText(QString::fromStdString(newFileName));
}

//-----------------------------------------------------------------------------
void pqImageOutputInfo::setupScreenshotInfo()
{
  this->Info->thumbnailLabel->setVisible(true);
  if(!this->View)
    {
    cerr << "no view available which seems really weird\n";
    return;
    }

  QSize viewSize = this->View->getSize();
  QSize thumbnailSize;
  if(viewSize.width() > viewSize.height())
    {
    thumbnailSize.setWidth(100);
    thumbnailSize.setHeight(100*viewSize.height()/viewSize.width());
    }
  else
    {
    thumbnailSize.setHeight(100);
    thumbnailSize.setWidth(100*viewSize.width()/viewSize.height());
    }
  if(this->View->getWidget()->isVisible())
    {
    vtkSmartPointer<vtkImageData> image;
    image.TakeReference(this->View->captureImage(thumbnailSize));
    vtkNew<vtkPNGWriter> pngWriter;
    pngWriter->SetInputData(image);
    pngWriter->WriteToMemoryOn();
    pngWriter->Update();
    pngWriter->Write();
    vtkUnsignedCharArray* result = pngWriter->GetResult();
    QPixmap thumbnail;
    thumbnail.loadFromData(
      result->GetPointer(0),
      result->GetNumberOfTuples()*result->GetNumberOfComponents(), "PNG");

    this->Info->thumbnailLabel->setPixmap(thumbnail);
    }
}
