/*=========================================================================

   Program: ParaView
   Module:    pqCurrentTimeToolbar.cxx

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
#include "pqCurrentTimeToolbar.h"

// Server Manager Includes.
#include "vtkSMProxy.h"
#include "vtkSMProperty.h"

// Qt Includes.
#include <QAction>
#include <QLabel>
#include <QLineEdit>

// ParaView Includes.
#include "pqSpinBox.h"
#include "pqAnimationScene.h"
#include "pqTimeKeeper.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
pqCurrentTimeToolbar::pqCurrentTimeToolbar(
  const QString &mytitle, QWidget *myparent): Superclass(mytitle, myparent)
{
  this->constructor();
}

//-----------------------------------------------------------------------------
pqCurrentTimeToolbar::pqCurrentTimeToolbar(QWidget *myparent) :
  Superclass(myparent)
{
  this->constructor();
}

//-----------------------------------------------------------------------------
void pqCurrentTimeToolbar::constructor()
{
  QLabel* label = new QLabel(this);
  label->setText("Time: ");
  this->TimeLabel = label;

  QLineEdit* timeedit = new QLineEdit(this);
  timeedit->setSizePolicy(
    QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
  timeedit->setObjectName("CurrentTime");
  timeedit->setValidator(new QDoubleValidator(this));
  this->TimeLineEdit = timeedit;

  QSpinBox* sbtimeedit = new pqSpinBox(this);
  sbtimeedit->setObjectName("CurrentTimeIndex");
  sbtimeedit->setMaximum(10000); // ensure that the widget is atleast 10000 wide.
  this->TimeSpinBox = sbtimeedit;

  //QObject::connect(
  //  this->TimeSpinBox, SIGNAL(valueChanged(int)),
  //  this, SLOT(currentTimeIndexChanged()));

  QObject::connect(
    this->TimeSpinBox, SIGNAL(editingFinished()),
    this, SLOT(currentTimeIndexChanged()));

  QObject::connect(
    this->TimeLineEdit, SIGNAL(editingFinished()),
    this, SLOT(currentTimeEdited()));

  this->addWidget(label);
  this->addWidget(timeedit);
  this->addWidget(sbtimeedit);

  QLabel* countLabel = new QLabel(this);
  countLabel->setText(" of (TBD)");
  this->CountLabelAction = this->addWidget(countLabel);
  this->CountLabel = countLabel;

  // need to use CountLabelAction to show/hide label.
  this->CountLabelAction->setVisible(false);

  this->ShowFrameCount = true;
}

//-----------------------------------------------------------------------------
pqCurrentTimeToolbar::~pqCurrentTimeToolbar()
{
}

//-----------------------------------------------------------------------------
void pqCurrentTimeToolbar::setShowFrameCount(bool val)
{
  this->ShowFrameCount = val;
  if (this->TimeSpinBox->isEnabled() && val)
    {
    this->CountLabelAction->setVisible(true);
    }
  else
    {
    this->CountLabelAction->setVisible(false);
    }
}

//-----------------------------------------------------------------------------
void pqCurrentTimeToolbar::setAnimationScene(pqAnimationScene* scene)
{
  if (this->Scene == scene)
    {
    return;
    }

  if (this->Scene)
    {
    QObject::disconnect(this->Scene, 0, this, 0);
    QObject::disconnect(this, 0, this->Scene, 0);
    }

  this->Scene = scene;
  if (this->Scene)
    {
    QObject::connect(this->Scene, SIGNAL(animationTime(double)),
      this, SLOT(sceneTimeChanged(double)));
    QObject::connect(this->Scene, SIGNAL(playModeChanged()),
      this, SLOT(onPlayModeChanged()));
    QObject::connect(this, SIGNAL(changeSceneTime(double)),
      this->Scene, SLOT(setAnimationTime(double)));
    QObject::connect(this->Scene, SIGNAL(timeStepsChanged()),
      this, SLOT(onTimeStepsChanged()));
    QObject::connect(this->Scene, SIGNAL(timeLabelChanged()),
      this, SLOT(onTimeLabelChanged()));

    this->sceneTimeChanged(this->Scene->getAnimationTime());
    }
}

//-----------------------------------------------------------------------------
void pqCurrentTimeToolbar::onTimeStepsChanged()
{
  bool prev = this->TimeSpinBox->blockSignals(true);
  pqTimeKeeper* timekeeper = this->Scene->getServer()->getTimeKeeper();
  int time_steps = timekeeper->getNumberOfTimeStepValues();
  if (time_steps > 0)
    {
    this->TimeSpinBox->setMaximum(time_steps -1);
    this->CountLabel->setText(QString(" of %1").arg(time_steps));
    }
  else
    {
    this->TimeSpinBox->setMaximum(0);
    this->CountLabel->setText("");
    }
  this->TimeSpinBox->blockSignals(prev);
}

//-----------------------------------------------------------------------------
void pqCurrentTimeToolbar::onTimeLabelChanged()
{
  pqTimeKeeper* timekeeper = this->Scene->getServer()->getTimeKeeper();
  this->TimeLabel->setText(
        pqSMAdaptor::getElementProperty(
          timekeeper->getProxy()->GetProperty("TimeLabel")).toString());

  this->TimeLabel->setText(this->TimeLabel->text() + ": ");
}

//-----------------------------------------------------------------------------
void pqCurrentTimeToolbar::onPlayModeChanged()
{
  if (this->Scene)
    {
    this->sceneTimeChanged(this->Scene->getAnimationTime());
    }
}

//-----------------------------------------------------------------------------
// When user edits the spin-box
void pqCurrentTimeToolbar::currentTimeIndexChanged()
{
  if (this->Scene)
    {
    pqTimeKeeper* timekeeper = this->Scene->getServer()->getTimeKeeper();
    emit this->changeSceneTime(
      timekeeper->getTimeStepValue(this->TimeSpinBox->value()));
    }
}

//-----------------------------------------------------------------------------
// When user edits the line-edit.
void pqCurrentTimeToolbar::currentTimeEdited()
{
  emit this->changeSceneTime(this->TimeLineEdit->text().toDouble());
}

//-----------------------------------------------------------------------------
// update GUI based on time.
void pqCurrentTimeToolbar::sceneTimeChanged(double time)
{
  if (!this->Scene)
    {
    return;
    }

  bool prev = this->blockSignals(true);

  pqTimeKeeper* timekeeper = this->Scene->getServer()->getTimeKeeper();
  vtkSMProxy* proxy = this->Scene->getProxy();
  QString playmode = pqSMAdaptor::getEnumerationProperty(
    proxy->GetProperty("PlayMode")).toString();
  if (playmode == "Snap To TimeSteps")
    {
    int index = timekeeper->getTimeStepValueIndex(time);
    this->TimeSpinBox->setValue(index);
    this->TimeSpinBox->setEnabled(true);
    if (this->ShowFrameCount)
      {
      this->CountLabelAction->setVisible(true);
      }
    this->TimeLineEdit->setEnabled(false);
    }
  else
    {
    this->TimeSpinBox->setEnabled(false);
    this->CountLabelAction->setVisible(false);
    this->TimeLineEdit->setEnabled(true);
    }

  this->TimeLineEdit->setText(QString::number(time));
  this->blockSignals(prev);
}
