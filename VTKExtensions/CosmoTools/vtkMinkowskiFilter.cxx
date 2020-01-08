#include "vtkMinkowskiFilter.h"

#include <cmath>
#include <cstdio>
#include <map>

#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolygon.h>
#include <vtkPolyhedron.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGrid.h>

#include <vtkSmartPointer.h>
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
#define VTK_NEW(type, name) name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkMinkowskiFilter);

vtkMinkowskiFilter::vtkMinkowskiFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkMinkowskiFilter::~vtkMinkowskiFilter()
{
}

void vtkMinkowskiFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "vtkMinkowskiFilter\n";
}

int vtkMinkowskiFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  vtkUnstructuredGrid* input =
    vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  VTK_CREATE(vtkDoubleArray, S);
  VTK_CREATE(vtkDoubleArray, V);
  VTK_CREATE(vtkDoubleArray, C);
  VTK_CREATE(vtkDoubleArray, X);
  VTK_CREATE(vtkDoubleArray, G);
  VTK_CREATE(vtkDoubleArray, T);
  VTK_CREATE(vtkDoubleArray, B);
  VTK_CREATE(vtkDoubleArray, L);
  VTK_CREATE(vtkDoubleArray, P);
  VTK_CREATE(vtkDoubleArray, F);

  compute_mf(input, S, V, C, X, G, T, B, L, P, F);

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
  output->GetCellData()->AddArray(S);
  output->GetCellData()->AddArray(V);
  output->GetCellData()->AddArray(C);
  output->GetCellData()->AddArray(X);
  output->GetCellData()->AddArray(G);
  output->GetCellData()->AddArray(T);
  output->GetCellData()->AddArray(B);
  output->GetCellData()->AddArray(L);
  output->GetCellData()->AddArray(P);
  output->GetCellData()->AddArray(F);

  return 1;
}

int vtkMinkowskiFilter::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");

    return 1;
  }

  return 0;
}

void vtkMinkowskiFilter::compute_mf(vtkUnstructuredGrid* ugrid, vtkDoubleArray* S,
  vtkDoubleArray* V, vtkDoubleArray* C, vtkDoubleArray* X, vtkDoubleArray* G, vtkDoubleArray* T,
  vtkDoubleArray* B, vtkDoubleArray* L, vtkDoubleArray* P, vtkDoubleArray* F)
{
  int i;
  int num_cells = ugrid->GetNumberOfCells();
  double *S_array, *V_array, *C_array, *X_array, *G_array, *T_array, *B_array, *L_array, *P_array,
    *F_array;

  S_array = new double[num_cells];
  V_array = new double[num_cells];
  C_array = new double[num_cells];
  X_array = new double[num_cells];
  G_array = new double[num_cells];
  T_array = new double[num_cells];
  B_array = new double[num_cells];
  L_array = new double[num_cells];
  P_array = new double[num_cells];
  F_array = new double[num_cells];

  for (i = 0; i < num_cells; i++)
  {
    vtkPolyhedron* cell = vtkPolyhedron::SafeDownCast(ugrid->GetCell(i));

    S_array[i] = compute_S(cell);
    V_array[i] = compute_V(ugrid, i);
    C_array[i] = compute_C(cell);
    X_array[i] = compute_X(cell);
    G_array[i] = compute_G(X_array[i]);
    T_array[i] = compute_T(V_array[i], S_array[i]);
    B_array[i] = compute_B(S_array[i], C_array[i]);
    L_array[i] = compute_L(C_array[i], G_array[i]);
    P_array[i] = compute_P(B_array[i], T_array[i]);
    F_array[i] = compute_F(B_array[i], L_array[i]);
  }

  S->SetName("S");
  S->SetNumberOfComponents(1);
  S->SetNumberOfTuples(num_cells);
  S->SetVoidArray(S_array, num_cells, 0);

  V->SetName("V");
  V->SetNumberOfComponents(1);
  V->SetNumberOfTuples(num_cells);
  V->SetVoidArray(V_array, num_cells, 0);

  C->SetName("C");
  C->SetNumberOfComponents(1);
  C->SetNumberOfTuples(num_cells);
  C->SetVoidArray(C_array, num_cells, 0);

  X->SetName("X");
  X->SetNumberOfComponents(1);
  X->SetNumberOfTuples(num_cells);
  X->SetVoidArray(X_array, num_cells, 0);

  G->SetName("G");
  G->SetNumberOfComponents(1);
  G->SetNumberOfTuples(num_cells);
  G->SetVoidArray(G_array, num_cells, 0);

  T->SetName("T");
  T->SetNumberOfComponents(1);
  T->SetNumberOfTuples(num_cells);
  T->SetVoidArray(T_array, num_cells, 0);

  B->SetName("B");
  B->SetNumberOfComponents(1);
  B->SetNumberOfTuples(num_cells);
  B->SetVoidArray(B_array, num_cells, 0);

  L->SetName("L");
  L->SetNumberOfComponents(1);
  L->SetNumberOfTuples(num_cells);
  L->SetVoidArray(L_array, num_cells, 0);

  P->SetName("P");
  P->SetNumberOfComponents(1);
  P->SetNumberOfTuples(num_cells);
  P->SetVoidArray(P_array, num_cells, 0);

  F->SetName("F");
  F->SetNumberOfComponents(1);
  F->SetNumberOfTuples(num_cells);
  F->SetVoidArray(F_array, num_cells, 0);
}

double vtkMinkowskiFilter::compute_S(vtkPolyhedron* cell)
{
  int i;
  int num_faces = cell->GetNumberOfFaces();
  double area = 0.0;

  vtkCell* face;
  for (i = 0; i < num_faces; i++)
  {
    face = cell->GetFace(i);
    area += compute_face_area(face);
  }

  return area;
}

double vtkMinkowskiFilter::compute_V(vtkPolyhedron* vtkNotUsed(cell))
{
  // for manually compute polyhedron volume
  return 0; // doing nothing now
}

double vtkMinkowskiFilter::compute_V(vtkUnstructuredGrid* ugrid, int cid)
{
  // for precomputed cell volume
  return ugrid->GetCellData()->GetArray("Volumes")->GetTuple1(cid);
}

double vtkMinkowskiFilter::compute_C(vtkPolyhedron* cell)
{
  int i, j;
  int num_edges = cell->GetNumberOfEdges();
  int num_faces = cell->GetNumberOfFaces();

  double face_normals[num_faces][3];

  int edge_faces[num_edges][2];
  for (i = 0; i < num_edges; i++)
  {
    edge_faces[i][0] = -1;
    edge_faces[i][1] = -1;
  }

  std::map<std::string, int> edge_map;

  char key[100];
  for (i = 0; i < num_edges; i++)
  {
    vtkCell* e = cell->GetEdge(i);

    sprintf(key, "%d_%d", (int)e->GetPointId(0), (int)e->GetPointId(1));
    std::string key_str = std::string(key);
    edge_map[key_str] = i;
  }

  for (i = 0; i < num_faces; i++)
  {
    vtkCell* f = cell->GetFace(i);
    vtkIdList* verts = f->GetPointIds();
    int num_verts = verts->GetNumberOfIds();

    for (j = 0; j < num_verts; j++)
    {
      sprintf(
        key, "%d_%d", (int)verts->GetId(j % num_verts), (int)verts->GetId((j + 1) % num_verts));
      std::string key_str = std::string(key);
      std::map<std::string, int>::iterator it = edge_map.find(key_str);
      if (it == edge_map.end())
      {
        sprintf(
          key, "%d_%d", (int)verts->GetId((j + 1) % num_verts), (int)verts->GetId(j % num_verts));
        key_str = std::string(key);
        it = edge_map.find(key_str);
      }

      if (it == edge_map.end())
        std::cerr << "error: can't find the edge" << std::endl;

      int edge_id = it->second;
      if (edge_faces[edge_id][0] == -1)
        edge_faces[edge_id][0] = i;
      else if (edge_faces[edge_id][1] == -1)
        edge_faces[edge_id][1] = i;
      else
        std::cerr << "error: edge is accessed more than twice" << std::endl;
    }

    compute_normal(f, face_normals[i]);
  }

  double C = 0;
  double l, phi, epsilon;
  VTK_CREATE(vtkPolygon, f1); // hack, vtkPolyhedron::GetFace keeps returning
  VTK_CREATE(vtkPolygon, f2); // the same pointer but different content
  for (i = 0; i < num_edges; i++)
  {
    f1->DeepCopy(cell->GetFace(edge_faces[i][0]));
    f2->DeepCopy(cell->GetFace(edge_faces[i][1]));
    vtkCell* e = cell->GetEdge(i);

    l = compute_edge_length(e);
    phi = compute_face_angle(f1, f2);
    epsilon = compute_epsilon(f1, f2, e);

    C += l * phi * epsilon;
  }
  C *= 0.5;

  return C;
}

double vtkMinkowskiFilter::compute_X(vtkPolyhedron* cell)
{
  int nfaces = cell->GetNumberOfFaces();
  int nedges = cell->GetNumberOfEdges();
  int nverts = cell->GetNumberOfPoints();

  double X = nfaces - nedges + nverts;

  return X;
}

double vtkMinkowskiFilter::compute_G(double X)
{
  return 1 - X / 2;
}

double vtkMinkowskiFilter::compute_T(double V, double S)
{
  return 3 * V / S;
}

double vtkMinkowskiFilter::compute_B(double S, double C)
{
  return S / C;
}

double vtkMinkowskiFilter::compute_L(double C, double G)
{
  return C / (4 * vtkMath::Pi() * (G + 1));
}

double vtkMinkowskiFilter::compute_P(double B, double T)
{
  return (B - T) / (B + T);
}

double vtkMinkowskiFilter::compute_F(double B, double L)
{
  return (L - B) / (L + B);
}

double vtkMinkowskiFilter::compute_face_area(vtkCell* face)
{
  int i, j, k;
  int coord; // coord to ignore: 1=x, 2=y, 3=z
  double area = 0;
  double an, ax, ay, az, N[3];

  int num_verts = face->GetNumberOfPoints();
  vtkPoints* verts = face->GetPoints();
  compute_normal(face, N);

  ax = fabs(N[0]);
  ay = fabs(N[1]);
  az = fabs(N[2]);

  coord = 3;
  if (ax > ay)
  {
    if (ax > az)
      coord = 1;
  }
  else if (ay > az)
    coord = 2;

  // compute area of the 2D projection
  double v1[3], v2[3], v3[3];
  for (i = 1, j = 2, k = 0; i <= num_verts; i++, j++, k++)
  {
    verts->GetPoint(i % num_verts, v1);
    verts->GetPoint(j % num_verts, v2);
    verts->GetPoint(k % num_verts, v3);

    if (coord == 1)
      area += v1[1] * (v2[2] - v3[2]);
    else if (coord == 2)
      area += v1[0] * (v2[2] - v3[2]);
    else if (coord == 3)
      area += v1[0] * (v2[1] - v3[1]);
  }

  // scale to get area before projection
  an = sqrt(ax * ax + ay * ay + az * az);
  if (coord == 1)
    area *= an / (2 * ax);
  else if (coord == 2)
    area *= an / (2 * ay);
  else if (coord == 3)
    area *= an / (2 * az);

  return fabs(area); // area is always positive
}

double vtkMinkowskiFilter::compute_edge_length(vtkCell* edge)
{
  double v[3], v1[3], v2[3];
  edge->GetPoints()->GetPoint(0, v1);
  edge->GetPoints()->GetPoint(1, v2);

  v[0] = v2[0] - v1[0];
  v[1] = v2[1] - v1[1];
  v[2] = v2[2] - v1[2];

  return sqrt(vtkMath::Dot(v, v));
}

int vtkMinkowskiFilter::compute_epsilon(vtkCell* f1, vtkCell* f2, vtkCell* e)
{
  int i;
  vtkIdType pe1, pe2, pf1 = -1, pf2 = -1;

  pe1 = e->GetPointId(0);
  pe2 = e->GetPointId(1);

  vtkIdList* vs1 = f1->GetPointIds();
  vtkIdList* vs2 = f2->GetPointIds();
  for (i = 0; i < vs1->GetNumberOfIds(); i++)
  {
    if (vs1->GetId(i) != pe1 && vs1->GetId(i) != pe2)
    {
      pf1 = i;
      break;
    }
  }
  for (i = 0; i < vs2->GetNumberOfIds(); i++)
  {
    if (vs2->GetId(i) != pe1 && vs2->GetId(i) != pe2)
    {
      pf2 = i;
      break;
    }
  }

  if (pf1 == -1 || pf2 == -1)
    std::cerr << "error: compute_epsilon" << std::endl;

  double ve[3], vf1[3], vf2[3], vf[3];
  e->GetPoints()->GetPoint(0, ve);
  f1->GetPoints()->GetPoint(pf1, vf1);
  f2->GetPoints()->GetPoint(pf2, vf2);

  vf[0] = (vf1[0] + vf2[0]) * 0.5;
  vf[1] = (vf1[1] + vf2[1]) * 0.5;
  vf[2] = (vf1[2] + vf2[2]) * 0.5;

  double vec[3];
  vec[0] = vf[0] - ve[0];
  vec[1] = vf[1] - ve[1];
  vec[2] = vf[2] - ve[2];
  double N[3];
  compute_normal(f1, N);

  double val = vtkMath::Dot(vec, N);

  if (val > 0)
    return -1;
  else
    return 1;
}

double vtkMinkowskiFilter::compute_face_angle(vtkCell* f1, vtkCell* f2)
{
  double n1[3], n2[3];

  compute_normal(f1, n1);
  compute_normal(f2, n2);

  return acos(vtkMath::Dot(n1, n2));
}

// compute normal of a face using Newell's method
void vtkMinkowskiFilter::compute_normal(vtkCell* face, double normal[3])
{
  int i;
  int num_verts = face->GetNumberOfPoints();
  vtkPoints* verts = face->GetPoints();
  double vcurr[3], vnext[3];

  normal[0] = 0.0;
  normal[1] = 0.0;
  normal[2] = 0.0;

  int curr, next;
  for (i = 0; i < num_verts; i++)
  {
    curr = i;
    next = (i + 1) % num_verts;
    verts->GetPoint(curr, vcurr);
    verts->GetPoint(next, vnext);
    normal[0] += (vcurr[1] - vnext[1]) * (vcurr[2] + vnext[2]);
    normal[1] += (vcurr[2] - vnext[2]) * (vcurr[0] + vnext[0]);
    normal[2] += (vcurr[0] - vnext[0]) * (vcurr[1] + vnext[1]);
  }

  vtkMath::Normalize(normal);

  // direction is inward, need to invert
  normal[0] *= -1.0;
  normal[1] *= -1.0;
  normal[2] *= -1.0;
}
