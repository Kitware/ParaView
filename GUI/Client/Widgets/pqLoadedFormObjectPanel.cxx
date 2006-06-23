/*=========================================================================

   Program: ParaView
   Module:    pqLoadedFormObjectPanel.cxx

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

// this include
#include "pqLoadedFormObjectPanel.h"

// Qt includes
#include <QVBoxLayout>
#include <QFormBuilder>

// VTK includes

// Paraview Server Manager includes

// ParaView includes


/// constructor
pqLoadedFormObjectPanel::pqLoadedFormObjectPanel(QString filename, QWidget* p)
  : pqNamedObjectPanel(p)
{
  QBoxLayout* mainlayout = new QVBoxLayout(this);
  mainlayout->setMargin(0);

  QFile file(filename);
  
  if(file.open(QFile::ReadOnly))
    {
    QFormBuilder builder;
    QWidget* customForm = builder.load(&file, NULL);
    file.close();
    mainlayout->addWidget(customForm);
    }
}

/// destructor
pqLoadedFormObjectPanel::~pqLoadedFormObjectPanel()
{
  if(this->Proxy)
    {
    this->unlinkServerManagerProperties();
    }
  this->Proxy = NULL;
}

bool pqLoadedFormObjectPanel::isValid()
{
  return this->layout()->count() == 1;
}

/// set the proxy to display properties for
void pqLoadedFormObjectPanel::setProxyInternal(pqSMProxy p)
{
  if(this->Proxy)
    {
    this->unlinkServerManagerProperties();
    }

  this->pqNamedObjectPanel::setProxyInternal(p);

  if(this->Proxy)
    {
    this->linkServerManagerProperties();
    }
}

