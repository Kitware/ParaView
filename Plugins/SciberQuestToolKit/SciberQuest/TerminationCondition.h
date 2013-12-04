/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __TerminationCondition_h
#define __TerminationCondition_h

#include "CartesianBounds.h" // for CartesianBounds
#include "FieldLine.h" // for FieldLine
#include "IntersectionSetColorMapper.h" // for IntersectionSetColorMapper
#include "SQMacros.h" // for sqErrorMacro

#include <vector> // for vector

class vtkPolyData;
class vtkCellLocator;

/// Detect line segment to set of surfaces intersection.
class TerminationCondition
{
public:
  TerminationCondition();
  virtual ~TerminationCondition();

  /**
  Determine if the segment p0->p1 intersects a periodic boundary.
  If so the bc is applied and the face id (1-6) is returned.
  Otherwise return 0.
  */
  int ApplyPeriodicBC(double p0[3], double p1[3]);

  /**
  Test for intersection of segment p0->p1 with the termination surfaces.
  Return the surface id in case an intersection is found and 0 otehrwise.
  The point of intersection is returned in pi and the parametric coordinate
  of the intersection in t.
  */
  int IntersectsTerminationSurface(double p0[3], double p1[3], double *pi);
  /**
  Return boolean indicating if the specified point is outside
  the problem domain. The two point variant updates p1 to be
  the intersection of the segment po->p1 and the problem domain.
  */
  int OutsideProblemDomain(const double pt[3]);
  int OutsideProblemDomain(const double p0[3], double p1[3]);

  /**
  Return boolean indicating if the field line is leaving the
  active sub-domain domain. Lines that are leaving the active
  sub-domain but not the problem domain can be integrated
  further.
  */
  int OutsideWorkingDomain(const double *p);

  /**
  Set/Get the problem domain, the valid region of the dataset.
  */
  void SetProblemDomain(const double domain[6], const int periodic[3]);
  void SetProblemDomain(const CartesianBounds &domain, const int periodic[3]);
  CartesianBounds &GetProblemDomain(){ return this->ProblemDomain; }

  /**
  Set/Get the working domain the valid region of the data block.
  */
  void SetWorkingDomain(const double domain[6]){ this->WorkingDomain.Set(domain); }
  void SetWorkingDomain(const CartesianBounds &domain){ this->WorkingDomain=domain; }
  CartesianBounds &GetWorkingDomain(){ return this->WorkingDomain; }

/**
  Remove any periodic BC's.
  */
  void ClearPeriodicBC();

  /**
  Set the problem domain to an empty domain. See OutsideProblemDomain.
  */
  void ResetProblemDomain() { this->ProblemDomain.Clear(); }

  /**
  Set the working domain to an empty domain. See OutsideWorkingDomain.
  */
  void ResetWorkingDomain() { this->WorkingDomain.Clear(); }

  /**
  Adds a termination surface. See IntersectsTerminationSurface.
  */
  void PushTerminationSurface(vtkPolyData *pd, const char *name=0);

  /**
  Remove all surfaces.See IntersectsTerminationSurface.
  */
  void ClearTerminationSurfaces();

  /**
  Convert implementation defined surface ids into a unique color.
  */
  int GetTerminationColor(FieldLine *line);
  int GetTerminationColor(int sId1, int sId2);

  /**
  Build the color mapper with the folowing scheme:

  0   -> field null
  1   -> s1
     ...
  n   -> sn
  n+1 -> problem domain
  n+2 -> short integration
  */
  void InitializeColorMapper();

  /**
  Return the indentifier for the special termination cases.
  */
  int GetProblemDomainSurfaceId(){ return 0; }
  int GetFieldNullId(){ return (int)this->TerminationSurfaces.size()+1; }
  int GetShortIntegrationId(){ return (int)this->TerminationSurfaces.size()+2; }

  /**
  Eliminate unused colors from the the lookup table and send
  a legend to the terminal on proc 0. This requires global
  communication all processes must be involved.
  */
  void SqueezeColorMap(vtkIntArray *colors)
    {
    this->CMap.SqueezeColorMap(colors);
    }

  /**
  Send a legend of the used colors to the terminal on proc 0. This
  requires global communication all processes must be involved.
  */
  void PrintColorMap()
    {
    this->CMap.PrintUsed();
    }

private:
  // Helper, to generate a polygonal box from a set of bounds.
  void DomainToLocator(vtkCellLocator *cellLoc, double dom[6]);

private:
  CartesianBounds ProblemDomain;                    // simulation bounds
  vtkCellLocator *PeriodicBCFaces[6];               // periodic faces
  CartesianBounds WorkingDomain;                    // current data bounds
  std::vector<vtkCellLocator*> TerminationSurfaces; // map surfaces
  std::vector<std::string> TerminationSurfaceNames; // names used in the map legend
  IntersectionSetColorMapper CMap;                  // helper assigning classes
};

//-----------------------------------------------------------------------------
inline
int TerminationCondition::OutsideWorkingDomain(const double pt[3])
{
  return this->WorkingDomain.Outside(pt);
}

//-----------------------------------------------------------------------------
inline
int TerminationCondition::OutsideProblemDomain(const double pt[3])
{
  return this->ProblemDomain.Outside(pt);
}

//-----------------------------------------------------------------------------
inline
int TerminationCondition::OutsideProblemDomain(const double p0[3], double p1[3])
{

  // The segment is always directed p0->p1, p0 is assumed to always
  // be inside. The caller will have previously applied point test.
  if (this->ProblemDomain.Outside(p1))
    {
    // clip segment p0->p1 at one of the six faces of
    // the domain bounds.
    double v[3]={
        p1[0]-p0[0],
        p1[1]-p0[1],
        p1[2]-p0[2]};

    // i
    if (p1[0]<this->ProblemDomain[0])
      {
      double t=(this->ProblemDomain[0]-p0[0])/v[0];
      p1[0]=this->ProblemDomain[0];
      p1[1]=p0[1]+v[1]*t;
      p1[2]=p0[2]+v[2]*t;
      }
    else
    if (p1[0]>this->ProblemDomain[1])
      {
      double t=(this->ProblemDomain[1]-p0[0])/v[0];
      p1[0]=this->ProblemDomain[1];
      p1[1]=p0[1]+v[1]*t;
      p1[2]=p0[2]+v[2]*t;
      }
    // j
    else
    if (p1[1]<this->ProblemDomain[2])
      {
      double t=(this->ProblemDomain[2]-p0[1])/v[1];
      p1[0]=p0[0]+v[0]*t;
      p1[1]=this->ProblemDomain[2];
      p1[2]=p0[2]+v[2]*t;
      }
    else
    if (p1[1]>this->ProblemDomain[3])
      {
      double t=(this->ProblemDomain[3]-p0[1])/v[1];
      p1[0]=p0[0]+v[0]*t;
      p1[1]=this->ProblemDomain[3];
      p1[2]=p0[2]+v[2]*t;
      }
    // k
    else
    if (p1[2]<this->ProblemDomain[4])
      {
      double t=(this->ProblemDomain[4]-p0[2])/v[2];
      p1[0]=p0[0]+v[0]*t;
      p1[1]=p0[1]+v[1]*t;
      p1[2]=this->ProblemDomain[4];
      }
    else
    if (p1[2]>this->ProblemDomain[5])
      {
      double t=(this->ProblemDomain[5]-p0[2])/v[2];
      p1[0]=p0[0]+v[0]*t;
      p1[1]=p0[1]+v[1]*t;
      p1[2]=this->ProblemDomain[5];
      }
    else
      {
      // never supposed to happen
      sqErrorMacro(std::cerr,"No intersection.");
      }
    return 1;
    }

  // p1 is inside the problem domain.
  return 0;
}

#endif

// VTK-HeaderTest-Exclude: TerminationCondition.h
