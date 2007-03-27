/*=========================================================================

   Program: ParaView
   Module:    pqThresholdPanel.cxx

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

#include "pqThresholdPanel.h"

// Qt includes
#include "pqDoubleRangeWidget.h"

pqThresholdPanel::pqThresholdPanel(pqProxy* pxy, QWidget* p) :
  pqLoadedFormObjectPanel(":/pqWidgets/UI/pqThresholdPanel.ui", pxy, p)
{
  this->Lower = this->findChild<pqDoubleRangeWidget*>("ThresholdBetween_0");
  this->Upper = this->findChild<pqDoubleRangeWidget*>("ThresholdBetween_1");

  QObject::connect(this->Lower, SIGNAL(valueChanged(double)),
                   this, SLOT(lowerChanged(double)));
  QObject::connect(this->Upper, SIGNAL(valueChanged(double)),
                   this, SLOT(upperChanged(double)));

  this->linkServerManagerProperties();
}

pqThresholdPanel::~pqThresholdPanel()
{
}

void pqThresholdPanel::accept()
{
  // accept widgets controlled by the parent class
  pqLoadedFormObjectPanel::accept();
}

void pqThresholdPanel::reset()
{
  // reset widgets controlled by the parent class
  pqLoadedFormObjectPanel::reset();
}

void pqThresholdPanel::linkServerManagerProperties()
{
  // parent class hooks up some of our widgets in the ui
  pqLoadedFormObjectPanel::linkServerManagerProperties();
}

void pqThresholdPanel::lowerChanged(double val)
{
  // clamp the lower threshold if we need to
  if(this->Upper->value() < val)
    {
    this->Upper->setValue(val);
    }
}

void pqThresholdPanel::upperChanged(double val)
{
  // clamp the lower threshold if we need to
  if(this->Lower->value() > val)
    {
    this->Lower->setValue(val);
    }
}

