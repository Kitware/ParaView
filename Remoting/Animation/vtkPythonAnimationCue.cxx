/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPythonAnimationCue.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPythonInterpreter.h"

#include <sstream>

vtkStandardNewMacro(vtkPythonAnimationCue);
//----------------------------------------------------------------------------
vtkPythonAnimationCue::vtkPythonAnimationCue()
{
  this->Enabled = true;
  this->Script = 0;
  this->AddObserver(
    vtkCommand::StartAnimationCueEvent, this, &vtkPythonAnimationCue::HandleStartCueEvent);
  this->AddObserver(
    vtkCommand::AnimationCueTickEvent, this, &vtkPythonAnimationCue::HandleTickEvent);
  this->AddObserver(
    vtkCommand::EndAnimationCueEvent, this, &vtkPythonAnimationCue::HandleEndCueEvent);
}

//----------------------------------------------------------------------------
vtkPythonAnimationCue::~vtkPythonAnimationCue()
{
  this->SetScript(NULL);
}

//----------------------------------------------------------------------------
void vtkPythonAnimationCue::HandleStartCueEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  // Set self to point to this
  char addrofthis[1024];
  sprintf(addrofthis, "%p", this);
  char* aplus = addrofthis;
  if ((addrofthis[0] == '0') && ((addrofthis[1] == 'x') || addrofthis[1] == 'X'))
  {
    aplus += 2; // skip over "0x"
  }
  if (this->Script)
  {
    std::ostringstream stream;
    stream << "from paraview import servermanager" << endl;
    stream << "def start_cue(foo): pass" << endl;
    stream << this->Script << endl;
    stream << "_me = servermanager.vtkPythonAnimationCue('" << aplus << "')\n";
    stream << "try:\n";
    stream << "  start_cue(_me)\n";
    stream << "finally:\n"
              "  del _me\n"
              "  import gc\n"
              "  gc.collect()\n";

    // ensure Python is initialized.
    vtkPythonInterpreter::Initialize();
    vtkPythonInterpreter::RunSimpleString(stream.str().c_str());
  }
}

//----------------------------------------------------------------------------
void vtkPythonAnimationCue::HandleTickEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  // Set self to point to this
  char addrofthis[1024];
  sprintf(addrofthis, "%p", this);
  char* aplus = addrofthis;
  if ((addrofthis[0] == '0') && ((addrofthis[1] == 'x') || addrofthis[1] == 'X'))
  {
    aplus += 2; // skip over "0x"
  }
  if (this->Script)
  {
    std::ostringstream stream;
    stream << "from paraview import servermanager" << endl;
    stream << this->Script << endl;
    stream << "_me = servermanager.vtkPythonAnimationCue('" << aplus << "')\n";
    stream << "try:\n";
    stream << "  tick(_me)\n";
    stream << "finally:\n"
              "  del _me\n"
              "  import gc\n"
              "  gc.collect()\n";

    // ensure Python is initialized.
    vtkPythonInterpreter::Initialize();
    vtkPythonInterpreter::RunSimpleString(stream.str().c_str());
  }
}

//----------------------------------------------------------------------------
void vtkPythonAnimationCue::HandleEndCueEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  // Set self to point to this
  char addrofthis[1024];
  sprintf(addrofthis, "%p", this);
  char* aplus = addrofthis;
  if ((addrofthis[0] == '0') && ((addrofthis[1] == 'x') || addrofthis[1] == 'X'))
  {
    aplus += 2; // skip over "0x"
  }
  if (this->Script)
  {
    std::ostringstream stream;
    stream << "from paraview import servermanager" << endl;
    stream << "def end_cue(foo): pass" << endl;
    stream << this->Script << endl;
    stream << "_me = servermanager.vtkPythonAnimationCue('" << aplus << "')\n";
    stream << "try:\n";
    stream << "  end_cue(_me)\n";
    stream << "finally:\n"
              "  del _me\n"
              "  import gc\n"
              "  gc.collect()\n";
    vtkPythonInterpreter::Initialize();
    vtkPythonInterpreter::RunSimpleString(stream.str().c_str());
  }
}

//----------------------------------------------------------------------------
void vtkPythonAnimationCue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "Script: " << this->Script << endl;
}
