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

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWStateMachineDOTWriter);
vtkCxxRevisionMacro(vtkKWStateMachineDOTWriter, "1.1");

//----------------------------------------------------------------------------
vtkKWStateMachineDOTWriter::vtkKWStateMachineDOTWriter()
{
  this->GraphLabel = NULL;

  this->GraphFontName = NULL;
  this->SetGraphFontName("Helvetica");
  this->GraphFontSize = 14;
  this->GraphFontColor[0] = 0.0;
  this->GraphFontColor[1] = 0.0;
  this->GraphFontColor[2] = 0.0;
  this->GraphDirection = vtkKWStateMachineDOTWriter::GraphDirectionLeftToRight;

  this->StateFontName = NULL;
  this->SetStateFontName("Helvetica");
  this->StateFontSize = 12;
  this->StateFontColor[0] = 0.0;
  this->StateFontColor[1] = 0.0;
  this->StateFontColor[2] = 0.0;

  this->InputFontName = NULL;
  this->SetInputFontName("Helvetica");
  this->InputFontSize = 9;
  this->InputFontColor[0] = 0.0;
  this->InputFontColor[1] = 0.0;
  this->InputFontColor[2] = 1.0;

  this->ClusterFontName = NULL;
  this->SetClusterFontName("Helvetica");
  this->ClusterFontSize = 10;
  this->ClusterFontColor[0] = 0.0;
  this->ClusterFontColor[1] = 0.0;
  this->ClusterFontColor[2] = 0.0;
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
    os << "\"";
    if (state == this->Input->GetInitialState())
      {
      os << ", peripheries=2";
      }
    os << "];" << endl;
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
    os << next_indent << "fontcolor=\"" << cluster_color << "\";" << endl;
    os << next_indent << "fontsize=" << this->ClusterFontSize << ";" << endl;
    os << next_indent << "style=dashed;" << endl;
    if (this->ClusterFontName)
      {
      os << next_indent << "fontname=\"" 
         << this->ClusterFontName << "\";" << endl;
      }
    nb_states = cluster->GetNumberOfStates();
    for (j = 0; j < nb_states; j++)
      {
      vtkKWStateMachineState *state = cluster->GetNthState(j);
      os << next_indent << state->GetId() << ";" << endl;
      }
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
}
