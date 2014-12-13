/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPANLHaloFinder.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#ifndef VTKPANLHALOFINDER_H
#define VTKPANLHALOFINDER_H

// .NAME vtkPANLHaloFinder.h -- Compute halos (clusters of particles)
//
// .SECTION Description
// Given an input a vtkUnstructuredGrid of points with arrays vx, vy, vz, and
// id, finds clumps of points (halos) using the cosmotools halo finder.  The
// first output is a vtkUnstructuredGrid similar to the input but with the data
// array fof_halo_tag appended to indicate which halo a point lies in.  The
// value will be -1 for points in no halo.  Optionally, subhalo finding can be
// turned on which will find subhalos of all halos over a certain size.  The
// subhalo id a point is in will be appended as the data array subhalo_tag.
//
// The second output is a summary of halo properties such as mass, center of mass,
// center (computed via one of several algorithms), and net velocity.  This
// vtkUnstructuredGrid has one point per halo.
//
// The third output is empty unless subhalo finding is turned on.  If subhalo
// finding is on, this output is similar to the second output except with data
// for each subhalo rather than each halo.  It contains one point per subhalo.

#include "vtkPVVTKExtensionsCosmoToolsModule.h"
#include "vtkUnstructuredGridAlgorithm.h"

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPANLHaloFinder : public vtkUnstructuredGridAlgorithm
{
  vtkTypeMacro(vtkPANLHaloFinder, vtkUnstructuredGridAlgorithm)
public:
  static vtkPANLHaloFinder* New();
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Turns on/off the subhalo finder part of the algorithm
  // Default: Off
  vtkSetMacro(RunSubHaloFinder,bool)
  vtkGetMacro(RunSubHaloFinder,bool)
  vtkBooleanMacro(RunSubHaloFinder,bool)

  // Description:
  // Gets/Sets RL, the physical coordinate box size
  // Default: 256.0
  vtkSetMacro(RL,double)
  vtkGetMacro(RL,double)

  // Description:
  // Gets/Sets the distance conversion factor.  This is multiplied into all position
  // coordinates before the halo finder is run.
  // Default: 1.0
  vtkSetMacro(DistanceConvertFactor,double)
  vtkGetMacro(DistanceConvertFactor,double)

  // Description:
  // Gets/Sets the mass conversion factor.  This is multiplied into the particle mass
  // before the halo finder is run.
  // Default: 1.0
  vtkSetMacro(MassConvertFactor,double)
  vtkGetMacro(MassConvertFactor,double)

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
  // Gets/Sets NP
  // Default: 1024
  vtkSetMacro(NP,int)
  vtkGetMacro(NP,int)

  // Description:
  // Gets/Sets the minimum number of close neighbors for a halo candidate to
  // include a particle.
  // Default: 1
  vtkSetMacro(NMin,int)
  vtkGetMacro(NMin,int)

  // Description:
  // Gets/Sets the minimum number of particles required for a halo candidate to
  // be considered a halo and output
  // Default: 10000
  vtkSetMacro(PMin,int)
  vtkGetMacro(PMin,int)

  // Description:
  // Gets/Sets the minimum halo size to run the subhalo finder on.
  // Default: 10000
  vtkSetMacro(MinFOFSubhaloSize,long)
  vtkGetMacro(MinFOFSubhaloSize,long)

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

  enum CenterFindingType {
    NONE,
    MOST_BOUND_PARTICLE,
    MOST_CONNECTED_PARTICLE,
    HIST_CENTER_FINDING
  };

protected:
  vtkPANLHaloFinder();
  virtual ~vtkPANLHaloFinder();

  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double RL;
  double DistanceConvertFactor;
  double MassConvertFactor;
  double DeadSize;
  float ParticleMass;
  double BB;
  double AlphaFactor;
  double BetaFactor;
  int NP;
  int NMin;
  int PMin;
  long MinFOFSubhaloSize;
  int MinCandidateSize;
  int NumSPHNeighbors;
  int NumNeighbors;

  bool RunSubHaloFinder;

  // Center finding parameters
  CenterFindingType CenterFindingMode;
  double SmoothingLength;
  double OmegaMatter;
  double OmegaCB;
  double Hubble;
  double RedShift;

  vtkMultiProcessController* Controller;

  class vtkInternals;
  vtkInternals* Internal;
private:
  void DistributeInput(vtkUnstructuredGrid* input);
  void CreateGhostParticles();
  void ExecuteHaloFinder(vtkUnstructuredGrid* allParticles, vtkUnstructuredGrid* fofProperties);
  void ExecuteSubHaloFinder(vtkUnstructuredGrid* allParticles, vtkUnstructuredGrid* subFofProperties);
  void FindCenters(vtkUnstructuredGrid* fofProperties);
};

#endif // VTKPANLHALOFINDER_H
