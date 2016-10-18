/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "vtkSQHemisphereSourceConfigurationWriter.h"
#include "vtkSQHemisphereSourceConfigurationFileInfo.h"

#include "vtkObjectFactory.h"
#include "vtkSMNamedPropertyIterator.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkStringList.h"

vtkStandardNewMacro(vtkSQHemisphereSourceConfigurationWriter);

//-----------------------------------------------------------------------------
vtkSQHemisphereSourceConfigurationWriter::vtkSQHemisphereSourceConfigurationWriter()
{
  vtkStringList* propNames = vtkStringList::New();
  propNames->AddString("Center");
  propNames->AddString("Radius");
  propNames->AddString("Resolution");

  vtkSMNamedPropertyIterator* propIt = vtkSMNamedPropertyIterator::New();
  propIt->SetPropertyNames(propNames);
  propNames->Delete();
  this->SetPropertyIterator(propIt);
  propIt->Delete();

  vtkSQHemisphereSourceConfigurationFileInfo info;
  this->SetFileIdentifier(info.FileIdentifier);
  this->SetFileDescription(info.FileDescription);
  this->SetFileExtension(info.FileExtension);
}

//-----------------------------------------------------------------------------
vtkSQHemisphereSourceConfigurationWriter::~vtkSQHemisphereSourceConfigurationWriter()
{
}

//-----------------------------------------------------------------------------
void vtkSQHemisphereSourceConfigurationWriter::SetProxy(vtkSMProxy* proxy)
{
  this->vtkSMProxyConfigurationWriter::SetProxy(proxy);
  this->GetPropertyIterator()->SetProxy(proxy);
}

//-----------------------------------------------------------------------------
void vtkSQHemisphereSourceConfigurationWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
