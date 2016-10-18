/*=========================================================================

   Program: ParaView
   Module:    pqOutputWindowAdapter.cxx

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

#include "pqOutputWindowAdapter.h"

#include <vtkObjectFactory.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(pqOutputWindowAdapter);

//-----------------------------------------------------------------------------
pqOutputWindowAdapter::pqOutputWindowAdapter()
{
  this->Active = true;
}

//-----------------------------------------------------------------------------
pqOutputWindowAdapter::~pqOutputWindowAdapter()
{
}

//-----------------------------------------------------------------------------
void pqOutputWindowAdapter::setActive(bool active)
{
  this->Active = active;
}

//-----------------------------------------------------------------------------
void pqOutputWindowAdapter::DisplayTextInWindow(const QString& text)
{
  if (this->Active)
  {
    emit displayTextInWindow(text);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindowAdapter::DisplayErrorTextInWindow(const QString& text)
{
  if (this->Active)
  {
    emit displayErrorTextInWindow(text);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindowAdapter::DisplayText(const char* text)
{
  if (this->Active)
  {
    emit displayText(text);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindowAdapter::DisplayErrorText(const char* text)
{
  if (this->Active)
  {
    emit displayErrorText(text);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindowAdapter::DisplayWarningText(const char* text)
{
  if (this->Active)
  {
    emit displayWarningText(text);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindowAdapter::DisplayGenericWarningText(const char* text)
{
  if (this->Active)
  {
    emit displayGenericWarningText(text);
  }
}
