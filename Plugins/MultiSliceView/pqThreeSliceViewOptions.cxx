/*=========================================================================

   Program: ParaView
   Module:  pqThreeSliceViewOptions.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#include "pqThreeSliceViewOptions.h"
#include "ui_pqThreeSliceViewOptions.h"

#include "vtkSMRenderViewProxy.h"
#include "vtkSMPropertyHelper.h"

#include <QHBoxLayout>

#include "pqThreeSliceView.h"

#include "pqActiveView.h"

class pqThreeSliceViewOptions::pqInternal
{
public:
  Ui::pqThreeSliceViewOptions ui;
};

//----------------------------------------------------------------------------
pqThreeSliceViewOptions::pqThreeSliceViewOptions(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  this->Internal = new pqInternal();
  this->Internal->ui.setupUi(this);

  // Origin
  QObject::connect( this->Internal->ui.X,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.Y,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.Z,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));

  // Top-Left
  QObject::connect( this->Internal->ui.TopLeftX,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.TopLeftY,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.TopLeftZ,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.TopLeftViewUpX,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.TopLeftViewUpY,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.TopLeftViewUpZ,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));

  // Top-Right
  QObject::connect( this->Internal->ui.TopRightX,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.TopRightY,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.TopRightZ,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.TopRightViewUpX,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.TopRightViewUpY,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.TopRightViewUpZ,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));

  // Bottom-Left
  QObject::connect( this->Internal->ui.BottomLeftX,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.BottomLeftY,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.BottomLeftZ,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.BottomLeftViewUpX,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.BottomLeftViewUpY,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
  QObject::connect( this->Internal->ui.BottomLeftViewUpZ,
                    SIGNAL(textChanged(QString)),
                    this, SIGNAL(changesAvailable()));
}

//----------------------------------------------------------------------------
pqThreeSliceViewOptions::~pqThreeSliceViewOptions()
{
}

//----------------------------------------------------------------------------
void pqThreeSliceViewOptions::setPage(const QString&)
{
}

//----------------------------------------------------------------------------
QStringList pqThreeSliceViewOptions::getPageList()
{
  QStringList ret;
  ret << "Three Slice View";
  return ret;
}

//----------------------------------------------------------------------------
void pqThreeSliceViewOptions::applyChanges()
{
  if(!this->View)
    {
    return;
    }

  // Update View options...
  this->View->setSlicesOrigin( this->Internal->ui.X->text().toDouble(),
                               this->Internal->ui.Y->text().toDouble(),
                               this->Internal->ui.Z->text().toDouble());

  this->View->setTopLeftNormal( this->Internal->ui.TopLeftX->text().toDouble(),
                                this->Internal->ui.TopLeftY->text().toDouble(),
                                this->Internal->ui.TopLeftZ->text().toDouble());
  this->View->setTopLeftViewUp( this->Internal->ui.TopLeftViewUpX->text().toDouble(),
                                this->Internal->ui.TopLeftViewUpY->text().toDouble(),
                                this->Internal->ui.TopLeftViewUpZ->text().toDouble());

  this->View->setTopRightNormal( this->Internal->ui.TopRightX->text().toDouble(),
                                this->Internal->ui.TopRightY->text().toDouble(),
                                this->Internal->ui.TopRightZ->text().toDouble());
  this->View->setTopRightViewUp( this->Internal->ui.TopRightViewUpX->text().toDouble(),
                                this->Internal->ui.TopRightViewUpY->text().toDouble(),
                                this->Internal->ui.TopRightViewUpZ->text().toDouble());

  this->View->setBottomLeftNormal( this->Internal->ui.BottomLeftX->text().toDouble(),
                                this->Internal->ui.BottomLeftY->text().toDouble(),
                                this->Internal->ui.BottomLeftZ->text().toDouble());
  this->View->setBottomLeftViewUp( this->Internal->ui.BottomLeftViewUpX->text().toDouble(),
                                this->Internal->ui.BottomLeftViewUpY->text().toDouble(),
                                this->Internal->ui.BottomLeftViewUpZ->text().toDouble());

  this->View->render();
}

//----------------------------------------------------------------------------
void pqThreeSliceViewOptions::resetChanges()
{
  if(!this->View)
    {
    return;
    }

  this->View->resetDefaultSettings();
  this->setView(this->View);
}

//----------------------------------------------------------------------------
void pqThreeSliceViewOptions::setView(pqView* view)
{
  this->View = qobject_cast<pqThreeSliceView*>(view);

  if(!this->View)
    {
    return;
    }

  // Update UI with the current values
  const double* vector = this->View->getTopLeftNormal();
  this->Internal->ui.TopLeftX->setText(QString::number(vector[0]));
  this->Internal->ui.TopLeftY->setText(QString::number(vector[1]));
  this->Internal->ui.TopLeftZ->setText(QString::number(vector[2]));

  vector = this->View->getTopRightNormal();
  this->Internal->ui.TopRightX->setText(QString::number(vector[0]));
  this->Internal->ui.TopRightY->setText(QString::number(vector[1]));
  this->Internal->ui.TopRightZ->setText(QString::number(vector[2]));

  vector = this->View->getBottomLeftNormal();
  this->Internal->ui.BottomLeftX->setText(QString::number(vector[0]));
  this->Internal->ui.BottomLeftY->setText(QString::number(vector[1]));
  this->Internal->ui.BottomLeftZ->setText(QString::number(vector[2]));

  vector = this->View->getTopLeftViewUp();
  this->Internal->ui.TopLeftViewUpX->setText(QString::number(vector[0]));
  this->Internal->ui.TopLeftViewUpY->setText(QString::number(vector[1]));
  this->Internal->ui.TopLeftViewUpZ->setText(QString::number(vector[2]));

  vector = this->View->getTopRightViewUp();
  this->Internal->ui.TopRightViewUpX->setText(QString::number(vector[0]));
  this->Internal->ui.TopRightViewUpY->setText(QString::number(vector[1]));
  this->Internal->ui.TopRightViewUpZ->setText(QString::number(vector[2]));

  vector = this->View->getBottomLeftViewUp();
  this->Internal->ui.BottomLeftViewUpX->setText(QString::number(vector[0]));
  this->Internal->ui.BottomLeftViewUpY->setText(QString::number(vector[1]));
  this->Internal->ui.BottomLeftViewUpZ->setText(QString::number(vector[2]));

  vector = this->View->getSlicesOrigin();
  this->Internal->ui.X->setText(QString::number(vector[0]));
  this->Internal->ui.Y->setText(QString::number(vector[1]));
  this->Internal->ui.Z->setText(QString::number(vector[2]));
}
