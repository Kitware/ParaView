/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqPluginDocumentationBehavior.h"

#include "vtkPVPlugin.h"
#include "vtkPVPluginTracker.h"
#include <vtksys/Base64.h>

#include <QHelpEngine>
#include <QTemporaryFile>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqPluginDocumentationBehavior::pqPluginDocumentationBehavior(
  QHelpEngine* parentObject)
  : Superclass(parentObject)
{
  Q_ASSERT(parentObject != NULL);
  vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();
  for (unsigned int cc=0; cc < tracker->GetNumberOfPlugins(); cc++)
    {
    if (tracker->GetPluginLoaded(cc))
      {
      this->updatePlugin(tracker->GetPlugin(cc));
      }
    }
}

//-----------------------------------------------------------------------------
pqPluginDocumentationBehavior::~pqPluginDocumentationBehavior()
{
}

//-----------------------------------------------------------------------------
void pqPluginDocumentationBehavior::updatePlugin(vtkPVPlugin* plugin)
{
  vtkstd::vector<vtkstd::string> resources;
  if (!plugin)
    {
    return;
    }
  plugin->GetBinaryResources(resources);

  QHelpEngine* engine = qobject_cast<QHelpEngine*>(this->parent());
  Q_ASSERT(engine);

  for (size_t cc=0; cc < resources.size(); cc++)
    {
    const vtkstd::string& str = resources[cc];
    unsigned char* decoded_stream = new unsigned char[str.size()];
    unsigned long length = vtksysBase64_Decode(
      reinterpret_cast<const unsigned char*>(str.c_str()),
      static_cast<unsigned long>(str.size()),
      decoded_stream,
      0);

    QTemporaryFile* file = new QTemporaryFile(this);
    if (!file->open())
      {
      qCritical() << "Failed to create temporary files." << endl;
      continue;
      }
    qint64 written = file->write(reinterpret_cast<char*>(decoded_stream), length);
    Q_ASSERT(written == (qint64)length);
    (void)written;
    engine->registerDocumentation(file->fileName());

    delete [] decoded_stream;
    }
}
