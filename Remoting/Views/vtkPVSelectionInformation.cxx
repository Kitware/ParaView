/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectionInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSelectionInformation.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include <sstream>

vtkStandardNewMacro(vtkPVSelectionInformation);

//----------------------------------------------------------------------------
vtkPVSelectionInformation::vtkPVSelectionInformation()
{
  this->Selection = vtkSelection::New();
}

//----------------------------------------------------------------------------
vtkPVSelectionInformation::~vtkPVSelectionInformation()
{
  if (this->Selection)
  {
    this->Selection->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkPVSelectionInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Selection: ";
  this->Selection->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void vtkPVSelectionInformation::Initialize()
{
  this->Selection->Initialize();
}

//----------------------------------------------------------------------------
void vtkPVSelectionInformation::CopyFromObject(vtkObject* obj)
{
  this->Initialize();
  vtkAlgorithm* alg = vtkAlgorithm::SafeDownCast(obj);
  if (alg)
  {
    vtkSelection* output = vtkSelection::SafeDownCast(alg->GetOutputDataObject(0));
    if (output)
    {
      this->Selection->DeepCopy(output);
    }
  }

  vtkSelection* sel = vtkSelection::SafeDownCast(obj);
  if (sel)
  {
    this->Selection->DeepCopy(sel);
  }
}

//----------------------------------------------------------------------------
void vtkPVSelectionInformation::AddInformation(vtkPVInformation* info)
{
  if (!info)
  {
    return;
  }

  vtkPVSelectionInformation* sInfo = vtkPVSelectionInformation::SafeDownCast(info);
  if (!sInfo)
  {
    vtkErrorMacro("Could not downcast info to array info.");
    return;
  }

  for (unsigned int i = 0; i < sInfo->Selection->GetNumberOfNodes(); ++i)
  {
    vtkSelectionNode* node = sInfo->Selection->GetNode(i);
    vtkSmartPointer<vtkSelectionNode> newNode = vtkSmartPointer<vtkSelectionNode>::New();
    newNode->ShallowCopy(node);
    this->Selection->AddNode(node);
  }
}

//----------------------------------------------------------------------------
void vtkPVSelectionInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;

  std::ostringstream res;
  vtkSelectionSerializer::PrintXML(res, vtkIndent(), 1, this->Selection);
  res << ends;
  *css << res.str().c_str();

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVSelectionInformation::CopyFromStream(const vtkClientServerStream* css)
{
  this->Initialize();

  const char* xml = nullptr;
  if (!css->GetArgument(0, 0, &xml))
  {
    vtkErrorMacro("Error parsing selection xml from message.");
    return;
  }
  vtkSelectionSerializer::Parse(xml, this->Selection);
}
