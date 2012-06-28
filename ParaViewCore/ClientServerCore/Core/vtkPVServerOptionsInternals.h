/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerOptionsInternals.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkPVServerOptionsInternals_h
#define __vtkPVServerOptionsInternals_h

#include <vector>
#include <string>

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
          this->UpperRight[i] = 0.0;
          }
        this->CaveBoundsSet = 0;
      }

    std::string Name;  // what is the name of the machine
    std::string Environment; // what environment variables should be set
    int CaveBoundsSet;  // have the cave bounds been set
    // store the cave bounds  all 0.0 if not set
    double LowerLeft[3];
    double LowerRight[3];
    double UpperRight[3];
  };
  void PrintSelf(ostream& os, vtkIndent indent)
    {
      os << indent << "Eye Separation: " << this->EyeSeparation << "\n";
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
          os << "\n" << ind2 << "UpperRight: ";
          for(j=0; j < 3; ++j)
            {
            os << minfo.UpperRight[j] << " ";
            }
          os << "\n";
          }
        else
          {
          os << ind2 << "No Cave Options\n";
          }
        }
    }
  std::vector<MachineInformation> MachineInformationVector; // store the vector of machines
  double EyeSeparation;		// Store Eye Separation information required for VR module
};

#endif
