/*=========================================================================

   Program:   ParaQ
   Module:    pqFileDialogEventPlayer.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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
#include "pqTestUtility.h"

#include <pqFileDialog.h>
#include <pqTesting.h>

#include <QApplication>
#include <QtDebug>

pqFileDialogEventPlayer::pqFileDialogEventPlayer()
{
}

bool pqFileDialogEventPlayer::playEvent(QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  // Handle playback for pqFileDialog and all its children ...
  pqFileDialog* object = 0;
  for(QObject* o = Object; o; o = o->parent())
    {
    object = qobject_cast<pqFileDialog*>(o);
    if(object)
      break;
    }
  if(!object)
    return false;

  if(Command == "filesSelected")
    {
    const QString data_directory = pqTestUtility::DataRoot();
    if(data_directory.isEmpty())
      {
      qCritical() << "You must set the PARAQ_DATA_ROOT environment variable to play-back file selections.";
      Error = true;
      return true;
      }
    
    /** \todo Handle multiple files */
    QString file = Arguments;
    file.replace("$PARAQ_DATA_ROOT", data_directory);

    QStringList files;
    files.append(file);

    object->emitFilesSelected(files);
    QApplication::processEvents();
        
    return true;
    }
    
  if(Command == "cancelled")
    {
    object->reject();
    return true;
    }

  qCritical() << "Unknown pqFileDialog command: " << Object << " " << Command << " " << Arguments;
  Error = true;
  return true;
}
