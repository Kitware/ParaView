/*=========================================================================

  Module:    vtkKWStateMachineDOTWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWStateMachineDOTWriter.h"

#include "vtkObjectFactory.h"

#include "vtkKWStateMachine.h"
#include "vtkKWStateMachineState.h"
#include "vtkKWStateMachineInput.h"
#include "vtkKWStateMachineTransition.h"
#include "vtkKWStateMachineCluster.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWStateMachineDOTWriter);
vtkCxxRevisionMacro(vtkKWStateMachineDOTWriter, "1.2");

//----------------------------------------------------------------------------
vtkKWStateMachineDOTWriter::vtkKWStateMachineDOTWriter()
{
  this->GraphLabel = NULL;

  this->GraphFontName = NULL;
  this->SetGraphFontName("Helvetica");
  this->GraphFontSize = 12;
  this->GraphFontColor[0] = 0.0;
  this->GraphFontColor[1] = 0.0;
  this->GraphFontColor[2] = 0.0;
  this->GraphDirection = vtkKWStateMachineDOTWriter::GraphDirectionLeftToRight;

  this->StateFontName = NULL;
  this->SetStateFontName("Helvetica");
  this->StateFontSize = 9;
  this->StateFontColor[0] = 0.0;
  this->StateFontColor[1] = 0.0;
  this->StateFontColor[2] = 0.0;

  this->InputFontName = NULL;
  this->SetInputFontName("Helvetica");
  this->InputFontSize = 8;
  this->InputFontColor[0] = 0.0;
  this->InputFontColor[1] = 0.0;
  this->InputFontColor[2] = 1.0;

  this->ClusterFontName = NULL;
  this->SetClusterFontName("Helvetica");
  this->ClusterFontSize = 10;
  this->ClusterFontColor[0] = 0.0;
  this->ClusterFontColor[1] = 0.0;
  this->ClusterFontColor[2] = 0.0;

  this->PutStatesAtSameRank = 0;
  this->CommandVisibility = 1;
}

//----------------------------------------------------------------------------
vtkKWStateMachineDOTWriter::~vtkKWStateMachineDOTWriter()
{
  this->SetGraphLabel(NULL);
  this->SetGraphFontName(NULL);
  this->SetStateFontName(NULL);
  this->SetInputFontName(NULL);
  this->SetClusterFontName(NULL);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineDOTWriter::SetGraphDirectionToTopToBottom() 
{
  this->SetGraphDirection(
    vtkKWStateMachineDOTWriter::GraphDirectionTopToBottom);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineDOTWriter::SetGraphDirectionToLeftToRight() 
{
  this->SetGraphDirection(
    vtkKWStateMachineDOTWriter::GraphDirectionLeftToRight);
}

//----------------------------------------------------------------------------
int vtkKWStateMachineDOTWriter::WriteToStream(ostream& os)
{
  if (!this->Input)
    {
    vtkErrorMacro("Input is not set!");
    return 0;
    }

  vtkIndent indent;
  indent = indent.GetNextIndent();

  int i, j;

  // Open graph

  os << "digraph G {" << endl;

  if (this->GraphDirection ==
      vtkKWStateMachineDOTWriter::GraphDirectionLeftToRight)
    {
    os << indent << "rankdir=LR;" << endl;
    }
  else if (this->GraphDirection ==
           vtkKWStateMachineDOTWriter::GraphDirectionTopToBottom)
    {
    os << indent << "rankdir=TB;" << endl;
    }

  char state_color[10];
  sprintf(state_color, "#%02x%02x%02x", 
          (int)(this->StateFontColor[0] * 255.0), 
          (int)(this->StateFontColor[1] * 255.0), 
          (int)(this->StateFontColor[2] * 255.0));

  os << indent << "node [fontcolor=\"" << state_color 
     << "\", fontsize=" << this->StateFontSize;
  if (this->StateFontName)
    {
    os << ", fontname=\"" << this->StateFontName << "\"";
    }
  os << "];" << endl;

  char input_color[10];
  sprintf(input_color, "#%02x%02x%02x", 
          (int)(this->InputFontColor[0] * 255.0), 
          (int)(this->InputFontColor[1] * 255.0), 
          (int)(this->InputFontColor[2] * 255.0));

  os << indent << "edge [fontcolor=\"" << input_color 
     << "\", fontsize=" << this->InputFontSize;
  if (this->InputFontName)
    {
    os << ", fontname=\"" << this->InputFontName << "\"";
    }
  os << "];" << endl;

  // Write all states

  os << endl;

  ostrstream all_states;

  int nb_states = this->Input->GetNumberOfStates();
  for (i = 0; i < nb_states; i++)
    {
    vtkKWStateMachineState *state = this->Input->GetNthState(i);
    os << indent << state->GetId();
    os << " [label=\"";
    if (state->GetName())
      {
      os << state->GetName();
      }
    else
      {
      os << state->GetId();
      }
    if (this->CommandVisibility)
      {
      int has_enter = state->HasEnterCommand() || 
        state->HasObserver(vtkKWStateMachineState::EnterEvent);
      int has_leave = state->HasLeaveCommand() || 
        state->HasObserver(vtkKWStateMachineState::LeaveEvent);
      if (has_enter || has_leave)
        {
        os << "\\n[";
        if (has_enter)
          {
          os << "e";
          }
        if (has_leave)
          {
          if (has_enter)
            {
            os << "/";
            }
          os << "l";
          }
        os << "]";
        }
      }
    os << "\"";
    if (state->GetAccepting())
      {
      os << ", peripheries=2";
      }
    os << "];" << endl;
    if (this->PutStatesAtSameRank)
      {
      all_states << state->GetId() << "; ";
      }
    }

  // Write all transitions

  os << endl;

  int nb_transitions = this->Input->GetNumberOfTransitions();
  for (i = 0; i < nb_transitions; i++)
    {
    vtkKWStateMachineTransition *transition = this->Input->GetNthTransition(i);
    if (!this->WriteSelfLoop && 
        transition->GetOriginState() == transition->GetDestinationState())
      {
      continue;
      }
    os << indent 
       << transition->GetOriginState()->GetId() 
       << " -> " 
       << transition->GetDestinationState()->GetId();
    vtkKWStateMachineInput *input = transition->GetInput();
    os << " [label=\"";
    if (input->GetName())
      {
      os << input->GetName();
      }
    else
      {
      os << input->GetId();
      }
    if (this->CommandVisibility)
      {
      int has_end = transition->HasEndCommand() || 
        transition->HasObserver(vtkKWStateMachineTransition::EndEvent);
      int has_start = transition->HasStartCommand() || 
        transition->HasObserver(vtkKWStateMachineTransition::StartEvent);
      if (has_end || has_start)
        {
        os << " [";
        if (has_start)
          {
          os << "s";
          }
        if (has_end)
          {
          if (has_start)
            {
            os << "/";
            }
          os << "e";
          }
        os << "]";
        }
      }
      os << "\"];" << endl;
    }

  // Write all clusters

  char cluster_color[10];
  sprintf(cluster_color, "#%02x%02x%02x", 
          (int)(this->ClusterFontColor[0] * 255.0), 
          (int)(this->ClusterFontColor[1] * 255.0), 
          (int)(this->ClusterFontColor[2] * 255.0));
  
  int nb_clusters = this->Input->GetNumberOfClusters();
  for (i = 0; i < nb_clusters; i++)
    {
    os << endl;
    vtkKWStateMachineCluster *cluster = this->Input->GetNthCluster(i);
    os << indent << "subgraph cluster" << cluster->GetId() << " {" << endl;
    vtkIndent next_indent = indent.GetNextIndent();
    os << next_indent << "fontcolor=\"" << cluster_color << "\"; " 
       << "fontsize=" << this->ClusterFontSize << "; ";
    if (this->ClusterFontName)
      {
      os << "fontname=\"" << this->ClusterFontName << "\"; ";
      }
    os << endl;
    os << next_indent << "style=dashed;" << endl;
    os << next_indent << "label=\"";
    if (cluster->GetName())
      {
      os << cluster->GetName();
      }
    else
      {
      os << cluster->GetId();
      }
    os << "\"" << endl;
    nb_states = cluster->GetNumberOfStates();
    os << next_indent;
    for (j = 0; j < nb_states; j++)
      {
      vtkKWStateMachineState *state = cluster->GetNthState(j);
      os << state->GetId() << "; ";
      }
    os << endl;
    os << indent << "}" << endl;
    }

  // Put all states at same rank?

  if (this->PutStatesAtSameRank)
    {
    all_states << ends;
    os << endl;
    os << indent << "{" << endl;
    vtkIndent next_indent = indent.GetNextIndent();
    os << next_indent << "rank=same;" << endl;
    os << next_indent << all_states.str() << endl;
    os << indent << "}" << endl;
    }

  // Label has to be at the end in order to be inherited by subgraphs

  if (this->GraphLabel)
    {
    os << endl;
    char graph_color[10];
    sprintf(graph_color, "#%02x%02x%02x", 
            (int)(this->GraphFontColor[0] * 255.0), 
            (int)(this->GraphFontColor[1] * 255.0), 
            (int)(this->GraphFontColor[2] * 255.0));
    os << indent << "fontcolor=\"" << graph_color << "\";" << endl;
    os << indent << "fontsize=" << this->GraphFontSize << ";" << endl;
    if (this->GraphFontName)
      {
      os << indent << "fontname=\"" << this->GraphFontName << "\";" << endl;
      }
    os << indent << "label=\"" << this->GraphLabel << "\";" << endl;
    }

  // Close graph

  os << "}" << endl;

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWStateMachineDOTWriter::WriteToFile(const char *filename)
{
  ofstream os(filename, ios::out);
  int ret = this->WriteToStream(os);
  
  if (!ret)
    {
    os.close();
    unlink(filename);
    }
  
  return ret;
}

//----------------------------------------------------------------------------
void vtkKWStateMachineDOTWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "GraphLabel: " 
     << (this->GraphLabel ? this->GraphLabel : "None") << endl;
  os << indent << "GraphDirection: " << this->GraphDirection << endl;

  os << indent << "GraphFontName: " 
     << (this->GraphFontName ? this->GraphFontName : "None") << endl;

  os << indent << "GraphFontSize: " << this->GraphFontSize << endl;

  os << indent << "GraphFontColor: (" << this->GraphFontColor[0] << ", " 
    << this->GraphFontColor[1] << ", " << this->GraphFontColor[2] << ")\n";

  os << indent << "StateFontName: " 
     << (this->StateFontName ? this->StateFontName : "None") << endl;

  os << indent << "StateFontSize: " << this->StateFontSize << endl;

  os << indent << "StateFontColor: (" << this->StateFontColor[0] << ", " 
    << this->StateFontColor[1] << ", " << this->StateFontColor[2] << ")\n";

  os << indent << "InputFontName: " 
     << (this->InputFontName ? this->InputFontName : "None") << endl;

  os << indent << "InputFontSize: " << this->InputFontSize << endl;

  os << indent << "InputFontColor: (" << this->InputFontColor[0] << ", " 
    << this->InputFontColor[1] << ", " << this->InputFontColor[2] << ")\n";

  os << indent << "GraphFontName: " 
     << (this->GraphFontName ? this->GraphFontName : "None") << endl;

  os << indent << "GraphFontSize: " << this->GraphFontSize << endl;

  os << indent << "GraphFontColor: (" << this->GraphFontColor[0] << ", " 
    << this->GraphFontColor[1] << ", " << this->GraphFontColor[2] << ")\n";

  os << indent << "PutStatesAtSameRank: " 
     << (this->PutStatesAtSameRank ? "On" : "Off") << endl;

  os << indent << "CommandVisibility: " 
     << (this->CommandVisibility ? "On" : "Off") << endl;
}
