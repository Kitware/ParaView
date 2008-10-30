/*=========================================================================

  Program:   Visualization Toolkit
  Module:    HaloClassPanel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    HaloClassPanel.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "pqApplicationCore.h"
#include "pqContourPanel.h"
#include "pqNamedWidgets.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "HaloClassPanel.h"

#include <QLayout>
#include <QLabel>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <vtkSMDoubleVectorProperty.h>

#include "vtkSMProxy.h"
#include "pqProxy.h"

HaloClassPanel::HaloClassPanel(pqProxy* pxy, QWidget* p)
  : pqObjectPanel(pxy, p), SampleScalarWidget(false)
{
  QGridLayout *gridLayout = new QGridLayout(this);
  gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
  gridLayout->setSpacing(6);
  gridLayout->setMargin(9);

  QLabel* label_class = new QLabel(this);
  label_class->setObjectName(QString::fromUtf8("label_class"));
  label_class->setText(
    this->proxy()->GetProperty("SelectInputScalars")->GetXMLLabel());

  gridLayout->addWidget(label_class, 0, 0, 1, 1);

  QComboBox *SelectInputScalars = new QComboBox(this);
  SelectInputScalars->setObjectName(QString::fromUtf8("SelectInputScalars"));
  gridLayout->addWidget(SelectInputScalars, 0, 2, 1, 1);
 
  connect(&this->SampleScalarWidget,
          SIGNAL(samplesChanged()), 
          this->propertyManager(), 
          SLOT(propertyChanged()));

  connect(this->propertyManager(), SIGNAL(accepted()), this, SLOT(onAccepted()));
  connect(this->propertyManager(), SIGNAL(rejected()), this, SLOT(onRejected()));

  this->SampleScalarWidget.setDataSources(this->proxy(),
    vtkSMDoubleVectorProperty::SafeDownCast(
    this->proxy()->GetProperty("BoundValues")), NULL);

  gridLayout->addWidget(&this->SampleScalarWidget, 1, 0, 5, 5);

  pqNamedWidgets::link(this, this->proxy(), this->propertyManager());
}

HaloClassPanel::~HaloClassPanel()
{
}

void HaloClassPanel::onAccepted()
{
  this->SampleScalarWidget.accept();
}

void HaloClassPanel::onRejected()
{
  this->SampleScalarWidget.reset();
}
