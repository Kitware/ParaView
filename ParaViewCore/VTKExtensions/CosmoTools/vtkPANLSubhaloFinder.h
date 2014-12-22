/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPANLSubhaloFinder.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#ifndef VTKPANLSUBHALOFINDER_H
#define VTKPANLSUBHALOFINDER_H

// .NAME vtkPANLSubhaloFinder -- finds subhalos of requested halos
//
// .SECTION Description
// This filter takes the output of the HACC halo finder and runs the subhalo
// finder on a user-specified list of halos.
//
// The input should be a vtkUnstructuredGrid with the data arrays vx, vy, vz,
// id and fof_halo_tag.  AddHaloToProcess should be used to select which halos
// to find subhalos of.
//
// The first output is a vtkUnstructuredGrid similar to the input but with the
// data array subhalo_tag appended to indicate which subhalo a point lies in.
// The value will be -1 for points in no halo.
//
// The second output is a vtkUnstructuredGrid with one point per subhalo and
// data arrays for summary information such as average velocity, mass, and
// center of mass.

#include "vtkPVVTKExtensionsCosmoToolsModule.h"
#include "vtkUnstructuredGridAlgorithm.h"

class vtkMultiProcessController;
class vtkIdList;

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPANLSubhaloFinder : public vtkUnstructuredGridAlgorithm
{
  vtkTypeMacro(vtkPANLSubhaloFinder, vtkUnstructuredGridAlgorithm)
public:
  static vtkPANLSubhaloFinder* New();
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Adds a halo to the list of halos that will have the subhalo finder run on them
  void AddHaloToProcess(vtkIdType haloId);
  // Description:
  // Clears the list of halos that will have the subhalo finder run on them
  void ClearHalosToProcess();

  // Description:
  // Gets the list of halos that will have the subhalo finder run on them
  vtkGetMacro(HalosToProcess,vtkIdList*)

  // Description:
  // Gets/Sets RL, the physical coordinate box size
  // Default: 256.0
  vtkSetMacro(RL,double)
  vtkGetMacro(RL,double)

  // Description:
  // Gets/Sets the size of the ghost particle region around each process's particles
  // to exchange when creating ghost particles.
  // Default: 8.0
  vtkSetMacro(DeadSize,double)
  vtkGetMacro(DeadSize,double)

  // Description:
  // Gets/Sets the particle mass.  For input datasets that do not have mass information
  // the mass of each particle defaults to this value.
  // Default: 1.307087181e+09
  vtkSetMacro(ParticleMass,float)
  vtkGetMacro(ParticleMass,float)

  // Description:
  // Gets/Sets distance threshold for particles to be considered in the same
  // halo.  This is measured in grid units on a NP x NP x NP grid.
  // Default: 0.1679999998
  vtkSetMacro(BB,double)
  vtkGetMacro(BB,double)

  // Description:
  // Gets/Sets alpha factor.  This controls how aggressively small subhalos
  // are grown.  Alpha factor of 1.0 is the least aggressive
  // Default: 1.0
  vtkSetClampMacro(AlphaFactor,double,0.0,1.0)
  vtkGetMacro(AlphaFactor,double)

  // Description:
  // Gets/Sets beta factor.  This controlls how saddle points between
  // subhalos are treated.  Larger values allow identification of smaller
  // scale structures such as tails.
  // Default: 0.0
  vtkSetClampMacro(BetaFactor,double,0.0,1.0)
  vtkGetMacro(BetaFactor,double)

  // Description:
  // Gets/Sets the minimum size of a subhalo candidate
  // Default: 200
  vtkSetMacro(MinCandidateSize,int)
  vtkGetMacro(MinCandidateSize,int)

  // Description:
  // Gets/Sets NumSPHNeighbors
  // Default: 64
  vtkSetMacro(NumSPHNeighbors,int)
  vtkGetMacro(NumSPHNeighbors,int)

  // Description:
  // Gets/Sets the number of neighbors that are examined by the subhalo finder
  // to determine local clumps near the each particle
  // Default: 20
  vtkSetMacro(NumNeighbors,int)
  vtkGetMacro(NumNeighbors,int)

protected:
  vtkPANLSubhaloFinder();
  virtual ~vtkPANLSubhaloFinder();

  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double RL;
  double DeadSize;
  double BB;
  double ParticleMass;
  double AlphaFactor;
  double BetaFactor;
  int MinCandidateSize;
  int NumSPHNeighbors;
  int NumNeighbors;

  vtkIdList* HalosToProcess;
  vtkMultiProcessController* Controller;

  class vtkInternals;
  vtkInternals* Internal;
private:
  void ExecuteSubHaloFinder(vtkUnstructuredGrid* input,
                            vtkUnstructuredGrid* allParticles,
                            vtkUnstructuredGrid* subFofProperties);
};

#endif
