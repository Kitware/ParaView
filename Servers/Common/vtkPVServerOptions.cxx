/*=========================================================================

  Module:    vtkPVServerOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerOptions.h"
#include "vtkObjectFactory.h"
#include <vtkstd/vector>
#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVServerOptions);
vtkCxxRevisionMacro(vtkPVServerOptions, "1.2");
class vtkPVServerOptionsInternals
{
public:
  // The MachineInformation struct is used
  // to store information about each machine.
  // These are stored in a vector one for each node in the process.
  struct MachineInformation
  {
  public:
    MachineInformation()
      {
        for(int i =0; i < 3; ++i)
          {
          this->LowerLeft[i] = 0.0;
          this->LowerRight[i] = 0.0;
          this->UpperLeft[i] = 0.0;
          }
        this->CaveBoundsSet = 0;
      }
    
    vtkstd::string Name;  // what is the name of the machine
    vtkstd::string Environment; // what environment variables should be set
    int CaveBoundsSet;  // have the cave bounds been set
    // store the cave bounds  all 0.0 if not set
    double LowerLeft[3];
    double LowerRight[3];
    double UpperLeft[3];
  };
  void PrintSelf(ostream& os, vtkIndent indent)
    {
      os << indent << "Machine Information :\n";
      vtkIndent ind = indent.GetNextIndent();
      for(unsigned int i =0; i < this->MachineInformationVector.size(); ++i)
        {
        MachineInformation& minfo = this->MachineInformationVector[i];
        os << ind << "Node: " << i << "\n";
        vtkIndent ind2 = ind.GetNextIndent();
        os << ind2 << "Name: " << minfo.Name.c_str() << "\n";
        os << ind2 << "Environment: " << minfo.Environment.c_str() << "\n";
        if(minfo.CaveBoundsSet)
          {
          int j;
          os << ind2 << "LowerLeft: ";
          for(j=0; j < 3; ++j)
            {
            os << minfo.LowerLeft[j] << " ";
            }
          os << "\n" << ind2 << "LowerRight: ";
          for(j=0; j < 3; ++j)
            {
            os << minfo.LowerRight[j] << " ";
            }
          os << "\n" << ind2 << "UpperLeft: ";
          for(j=0; j < 3; ++j)
            {
            os << minfo.UpperLeft[j] << " ";
            }
          os << "\n";
          }
        else
          {
          os << ind2 << "No Cave Options\n";
          }
        }
    }
  vtkstd::vector<MachineInformation> MachineInformationVector; // store the vector of machines
};

//----------------------------------------------------------------------------
vtkPVServerOptions::vtkPVServerOptions()
{
  this->Internals = new vtkPVServerOptionsInternals;
}

//----------------------------------------------------------------------------
vtkPVServerOptions::~vtkPVServerOptions()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPVServerOptions::Initialize()
{
  Superclass::Initialize();
}

//----------------------------------------------------------------------------
int vtkPVServerOptions::AddMachineInformation(const char** atts)
{
  vtkPVServerOptionsInternals::MachineInformation info;
  int caveBounds = 0;
  for (int i = 0; atts[i] && atts[i + 1]; i += 2)
    {
    vtkstd::string key = atts[i];
    vtkstd::string value = atts[i + 1];
    if(key == "Name")
      {
      info.Name = value;
      }
    else if(key == "Environment")
      {
      info.Environment = value;
      }
    else if(key == "LowerLeft")
      {
      caveBounds++;
      istrstream str(const_cast<char *>(value.c_str()));
      for(int j =0; j < 3; j++)
        {
        str >> info.LowerLeft[j];
        }
      }
    else if(key == "LowerRight")
      {
      caveBounds++;
      istrstream str(const_cast<char *>(value.c_str()));
      for(int j =0; j < 3; j++)
        {
        str >> info.LowerRight[j];
        }
      }
    else if(key == "UpperLeft")
      {
      caveBounds++;
      istrstream str(const_cast<char *>(value.c_str()));
      for(int j =0; j < 3; j++)
        {
        str >> info.UpperLeft[j];
        }
      }
    }
  if(caveBounds && caveBounds != 3)
    {
    vtkErrorMacro("LowerRight LowerLeft and UpperLeft must all be present, if one is present");
    return 0;
    }
  if(caveBounds)
    {
    // if there are cave bounds then set the render module to CaveRenderModule
    this->SetRenderModuleName("CaveRenderModule");
    info.CaveBoundsSet = 1;
    }
  this->Internals->MachineInformationVector.push_back(info);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVServerOptions::ParseExtraXMLTag(const char* name, const char** atts)
{
  // handle the Machine tag
  if(strcmp(name, "Machine") == 0)
    {
    return this->AddMachineInformation(atts);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVServerOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Internals->PrintSelf(os, indent);
}

