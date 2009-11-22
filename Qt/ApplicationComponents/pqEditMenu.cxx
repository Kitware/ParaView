/*=========================================================================

   Program: ParaView
   Module:    pqEditMenu.cxx

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
#include "pqEditMenu.h"
#include "ui_pqEditMenu.h"

class pqEditMenu::pqInternals : public Ui::pqEditMenu
{
};

//-----------------------------------------------------------------------------
pqEditMenu::pqEditMenu(QWidget* parentObject)
  : Superclass(parentObject), Internals(0)
{
  this->constructor();
}

//-----------------------------------------------------------------------------
pqEditMenu::pqEditMenu(const QString& title, QWidget* parentObject)
  : Superclass(title, parentObject), Internals(0)
{
  this->constructor();
  this->setTitle(title);
}

//-----------------------------------------------------------------------------
pqEditMenu::~pqEditMenu()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqEditMenu::constructor()
{
  this->Internals = new pqEditMenu::pqInternals();
  this->Internals->setupUi(this);

  // TODO: Connect to events of interest from the pqClientCore 
  // (replacement for pqMainWindowCore) or pqApplicationCore.
}

//-----------------------------------------------------------------------------
void pqEditMenu::updateEnableState()
{
}
