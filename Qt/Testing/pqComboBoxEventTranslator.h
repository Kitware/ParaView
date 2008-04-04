/*=========================================================================

   Program: ParaView
   Module:    pqComboBoxEventTranslator.h

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

#ifndef _pqComboBoxEventTranslator_h
#define _pqComboBoxEventTranslator_h

#include "pqWidgetEventTranslator.h"

/**
Translates low-level Qt events into high-level ParaView events that can be recorded as test cases.

\sa pqEventTranslator
*/

class pqComboBoxEventTranslator :
  public pqWidgetEventTranslator
{
  Q_OBJECT
  
public:
  pqComboBoxEventTranslator(QObject* p=0);
  
  virtual bool translateEvent(QObject* Object, QEvent* Event, bool& Error);

private:
  pqComboBoxEventTranslator(const pqComboBoxEventTranslator&);
  pqComboBoxEventTranslator& operator=(const pqComboBoxEventTranslator&);

  QObject* CurrentObject;
  
private slots:
  void onDestroyed(QObject*);
  void onStateChanged(const QString&);  
};

#endif // !_pqComboBoxEventTranslator_h

