#include "vtkLagrangianIntegrationModelExample.h"

#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkLagrangianParticle.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"

#include <cstring>

vtkObjectFactoryNewMacro(vtkLagrangianIntegrationModelExample);

//----------------------------------------------------------------------------
vtkLagrangianIntegrationModelExample::vtkLagrangianIntegrationModelExample()
{
  // Fill the helper array
  // Here one should set the seed and surface array name and components
  // It will enable the uses of helper filters
  this->SeedArrayNames->InsertNextValue("Particle Diameter");
  this->SeedArrayComps->InsertNextValue(1);
  this->SeedArrayTypes->InsertNextValue(VTK_DOUBLE);
  this->SeedArrayNames->InsertNextValue("Particle Density");
  this->SeedArrayComps->InsertNextValue(1);
  this->SeedArrayTypes->InsertNextValue(VTK_DOUBLE);

  // No Enum surface entry
  SurfaceArrayDescription surfaceType2Description;
  surfaceType2Description.nComp = 1;
  surfaceType2Description.type = VTK_CHAR;
  this->SurfaceArrayDescriptions["SurfaceType2"] = surfaceType2Description;

  // More enum surface entry, use a reference "&" !
  SurfaceArrayDescription& surfaceTypeDescription = this->SurfaceArrayDescriptions["SurfaceType"];
  surfaceTypeDescription.enumValues.push_back(std::make_pair(101, "SpecificModel"));

  this->NumIndepVars = 8; // x, y, z, u, v, w, diameter, t
  this->NumFuncs = this->NumIndepVars - 1;
}

//----------------------------------------------------------------------------
void vtkLagrangianIntegrationModelExample::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkLagrangianIntegrationModelExample::FunctionValues(vtkLagrangianParticle* particle,
  vtkDataSet* dataSet, vtkIdType cellId, double* weights, double* x, double* f)
{
  // Initialize output
  std::fill(f, f + this->NumFuncs, 0.0);

  // Check for a particle
  if (!particle)
  {
    vtkErrorMacro("No Particle to integrate");
    return 0;
  }

  // Sanity Check
  if (!dataSet || cellId == -1)
  {
    vtkErrorMacro("No cell or dataset to integrate the Particle on : Dataset: "
      << dataSet << " CellId:" << cellId);
    return 0;
  }

  // Fetch flowVelocity at index 3
  double flowVelocity[3];
  if (this->GetFlowOrSurfaceDataNumberOfComponents(3, dataSet) != 3 ||
    !this->GetFlowOrSurfaceData(3, dataSet, cellId, weights, flowVelocity))
  {
    vtkErrorMacro("Flow velocity is not set in source flow dataset or "
                  "have incorrect number of components, cannot use Matida equations");
    return 0;
  }

  // Fetch flowDensity at index 4
  double flowDensity;
  if (this->GetFlowOrSurfaceDataNumberOfComponents(4, dataSet) != 1 ||
    !this->GetFlowOrSurfaceData(4, dataSet, cellId, weights, &flowDensity))
  {
    vtkErrorMacro("Flow density is not set in source flow dataset or "
                  "have incorrect number of components, cannot use Matida equations");
    return 0;
  }

  // Fetch flowDynamicViscosity at index 5
  double flowDynamicViscosity;
  if (this->GetFlowOrSurfaceDataNumberOfComponents(5, dataSet) != 1 ||
    !this->GetFlowOrSurfaceData(5, dataSet, cellId, weights, &flowDynamicViscosity))
  {
    vtkErrorMacro("Flow dynamic viscosity is not set in source flow dataset or "
                  "have incorrect number of components, cannot use Matida equations");
    return 0;
  }

  // Fetch Particle Properties
  vtkIdType tupleIndex = particle->GetSeedArrayTupleIndex();

  // Fetch Particle Diameter as the first user variables
  double particleDiameter = particle->GetUserVariables()[0];

  // Fetch Particle Density at index 7
  vtkDataArray* particleDensities = vtkDataArray::SafeDownCast(this->GetSeedArray(7, particle));
  if (!particleDensities)
  {
    vtkErrorMacro("Particle density is not set in particle data, "
                  "cannot use Matida equations");
    return 0;
  }
  double particleDensity = particleDensities->GetTuple1(tupleIndex);

  // Recover Gravity constant, idx 8, FieldData, as defined in the xml.
  // We read at a index 0 because these is the only tuple in the fieldData
  double gravityConstant;
  if (this->GetFlowOrSurfaceDataNumberOfComponents(8, dataSet) != 1 ||
    !this->GetFlowOrSurfaceData(8, dataSet, 0, weights, &gravityConstant))
  {
    vtkErrorMacro("GravityConstant is not set in source flow dataset or have"
                  "incorrect number of components, cannot use Matida Equations");
    return 0;
  }

  // Compute function values
  for (int i = 0; i < 3; i++)
  {
    // Matida Equation
    f[i + 3] = (flowVelocity[i] - x[i + 3]) *
      this->GetDragCoefficient(flowVelocity, particle->GetVelocity(), flowDynamicViscosity,
        particleDiameter, flowDensity) /
      (this->GetRelaxationTime(flowDynamicViscosity, particleDiameter, particleDensity));
    f[i] = x[i + 3];
  }

  // Use the read gravity constant instead of G
  f[5] -= gravityConstant * (1 - (flowDensity / particleDensity));

  // Compute the supplementary variable diameter
  f[6] = -particleDiameter * 1 / 10;
  return 1;
}

//---------------------------------------------------------------------------
double vtkLagrangianIntegrationModelExample::GetRelaxationTime(
  const double& dynVisc, const double& diameter, const double& density)
{
  if (dynVisc == 0)
  {
    return std::numeric_limits<double>::infinity();
  }
  else
  {
    return (density * diameter * diameter) / (18.0 * dynVisc);
  }
}

//---------------------------------------------------------------------------
double vtkLagrangianIntegrationModelExample::GetDragCoefficient(const double* flowVelocity,
  const double* particleVelocity, const double& dynVisc, const double& particleDiameter,
  const double& flowDensity)
{
  if (dynVisc == 0)
  {
    return -1.0 * std::numeric_limits<double>::infinity();
  }
  else
  {
    double relativeVelocity[3];
    for (int i = 0; i < 3; i++)
    {
      relativeVelocity[i] = particleVelocity[i] - flowVelocity[i];
    }
    double relativeSpeed = vtkMath::Norm(relativeVelocity);
    double reynolds = flowDensity * relativeSpeed * particleDiameter / dynVisc;

    return (1 + 0.15 * pow(reynolds, 0.687));
  }
}

//----------------------------------------------------------------------------
void vtkLagrangianIntegrationModelExample::InitializeParticle(vtkLagrangianParticle* particle)
{
  double* diameter = particle->GetUserVariables();

  // Recover Particle Diameter, idx 6, see xml file
  vtkDoubleArray* particleDiameters = vtkDoubleArray::SafeDownCast(this->GetSeedArray(6, particle));
  if (!particleDiameters)
  {
    vtkErrorMacro("ParticleDiameter is not set in particle data, variable not set");
  }
  else
  {
    // Copy seed data to user variables
    *diameter = particleDiameters->GetTuple1(particle->GetSeedArrayTupleIndex());
  }
}

//----------------------------------------------------------------------------
void vtkLagrangianIntegrationModelExample::InitializeVariablesParticleData(
  vtkPointData* variablesParticleData, int maxTuples)
{
  // Create a double array
  vtkNew<vtkDoubleArray> diameterArray;

  // Name it
  diameterArray->SetName("VariableDiameter");

  // Set the number of component
  int nComp = 1;
  diameterArray->SetNumberOfComponents(nComp);

  // Allocate a default number of tuple, multiplied
  // by the number of components
  diameterArray->Allocate(maxTuples * nComp);

  variablesParticleData->AddArray(diameterArray.Get());
}

//----------------------------------------------------------------------------
void vtkLagrangianIntegrationModelExample::InsertVariablesParticleData(
  vtkLagrangianParticle* particle, vtkPointData* data, int stepEnum)
{
  vtkDataArray* diameterArray = data->GetArray("VariableDiameter");

  // Add the correct data depending of the step enum
  // Previous, Current or Next
  switch (stepEnum)
  {
    case (vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_PREV):
    {
      // Always use InsertNextTuple methods to add data
      diameterArray->InsertNextTuple1(particle->GetPrevUserVariables()[0]);
      break;
    }
    case (vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_CURRENT):
    {
      diameterArray->InsertNextTuple1(particle->GetUserVariables()[0]);
      break;
    }
    case (vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_NEXT):
    {
      diameterArray->InsertNextTuple1(particle->GetNextUserVariables()[0]);
      break;
    }
  }
}

//----------------------------------------------------------------------------
bool vtkLagrangianIntegrationModelExample::CheckFreeFlightTermination(
  vtkLagrangianParticle* particle)
{
  // Free Flight example, depending of the variable diameter of the particle we
  // choose to terminate the particle or not
  return (particle->GetUserVariables()[0] < 0.07);
}

//----------------------------------------------------------------------------
bool vtkLagrangianIntegrationModelExample::InteractWithSurface(int vtkNotUsed(surfaceType),
  vtkLagrangianParticle* particle, vtkDataSet* surface, vtkIdType cellId,
  std::queue<vtkLagrangianParticle*>& vtkNotUsed(particles))
{
  // Surface Interaction example, depending of the variable diameter of the
  // particle we choose to do one thing or the other
  if (particle->GetUserVariables()[0] > 0.075)
  {
    // One could even redefine its own way to bounce, eg BounceSpecific(particle...)
    // Or even redefine BounceParticle for all bounce surface
    return this->BounceParticle(particle, surface, cellId);
  }
  else
  {
    return this->TerminateParticle(particle);
  }
}

//----------------------------------------------------------------------------
bool vtkLagrangianIntegrationModelExample::IntersectWithLine(
  vtkCell* cell, double p1[3], double p2[3], double tol, double& t, double x[3])
{
  // Here one could implement its own intersection code
  return this->Superclass::IntersectWithLine(cell, p1, p2, tol, t, x);
}

//----------------------------------------------------------------------------
void vtkLagrangianIntegrationModelExample::ComputeSurfaceDefaultValues(
  const char* arrayName, vtkDataSet* dataset, int nComponents, double* defaultValues)
{
  if (strcmp(arrayName, "SurfaceType2") == 0)
  {
    for (int i = 0; i < nComponents; i++)
    {
      defaultValues[i] = 3;
    }
    return;
  }
  else if (strcmp(arrayName, "SurfaceType") == 0)
  {
    for (int i = 0; i < nComponents; i++)
    {
      defaultValues[i] = 2;
    }
    return;
  }
  else
  {
    this->Superclass::ComputeSurfaceDefaultValues(arrayName, dataset, nComponents, defaultValues);
  }
}
