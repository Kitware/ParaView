/* Copyright 2023 NVIDIA Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// SPDX-FileCopyrightText: Copyright 2023 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtknvindex_global_settings.h"

#include "vtkCallbackCommand.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMSettings.h"

#include <iostream>

vtkSmartPointer<vtknvindex_global_settings> vtknvindex_global_settings::Instance;

//-------------------------------------------------------------------------------------------------
vtknvindex_global_settings* vtknvindex_global_settings::New()
{
  vtknvindex_global_settings* instance = vtknvindex_global_settings::GetInstance();
  instance->Register(nullptr);
  return instance;
}

//-------------------------------------------------------------------------------------------------
vtknvindex_global_settings* vtknvindex_global_settings::GetInstance()
{
  if (!vtknvindex_global_settings::Instance)
  {
    vtknvindex_global_settings* instance = new vtknvindex_global_settings();
    instance->InitializeObjectBase();
    vtknvindex_global_settings::Instance.TakeReference(instance);
  }
  return vtknvindex_global_settings::Instance;
}

//-------------------------------------------------------------------------------------------------
vtknvindex_global_settings::vtknvindex_global_settings()
  : ObserverTag(0)
{
  vtkObject* obj = vtkPVGeneralSettings::GetInstance();

  this->SettingsObserver->SetCallback(&vtknvindex_global_settings::InitializeSettings);
  this->SettingsObserver->SetClientData(this);
  this->ObserverTag = obj->AddObserver(vtkCommand::ModifiedEvent, this->SettingsObserver.Get());
}

//-------------------------------------------------------------------------------------------------
vtknvindex_global_settings::~vtknvindex_global_settings()
{
  vtkObject* obj = vtknvindex_global_settings::GetInstance();
  if (obj && ObserverTag != 0)
    obj->RemoveObserver(this->ObserverTag);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_global_settings::InitializeSettings(vtkObject*, unsigned long, void*, void*) {}

//-------------------------------------------------------------------------------------------------
void vtknvindex_global_settings::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_global_settings::SetNumberOfExtraConfigurations(int n)
{
  this->ExtraConfigurations.resize(static_cast<size_t>(n));
}

//-------------------------------------------------------------------------------------------------
int vtknvindex_global_settings::GetNumberOfExtraConfigurations()
{
  return static_cast<int>(this->ExtraConfigurations.size());
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_global_settings::SetExtraConfiguration(int i, const char* expression)
{
  if (i >= 0 && i < this->GetNumberOfExtraConfigurations())
  {
    if (strcmp(this->ExtraConfigurations[i].c_str(), expression) != 0)
    {
      this->ExtraConfigurations[i] = expression;
    }
  }
}

//-------------------------------------------------------------------------------------------------
const char* vtknvindex_global_settings::GetExtraConfiguration(int i)
{
  if (i >= 0 && i < this->GetNumberOfExtraConfigurations())
  {
    return this->ExtraConfigurations[i].c_str();
  }

  return nullptr;
}
