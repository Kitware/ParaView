/*=========================================================================

  Program:   ParaView
  Module:    TestSubsetInclusionLattice.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkNew.h"
#include "vtkSubsetInclusionLattice.h"

void Lookup(vtkSubsetInclusionLattice* sil, const char* name, int id)
{
  int gid = sil->FindNode(name);
  if (gid != id)
  {
    cout << "ERROR: Looking for `" << name << "`. " << endl
         << "       Expected `" << id << "`, Got `" << gid << "`." << endl;
  }
}

int TestA()
{
  vtkNew<vtkSubsetInclusionLattice> sil;
  sil->Initialize();
  auto world = sil->AddNode("World");
  auto europe = sil->AddNode("Europe", world);
  auto uk = sil->AddNode("United Kingdom", europe);
  auto eu = sil->AddNode("EU", europe);
  auto namerica = sil->AddNode("North America", world);
  auto usa = sil->AddNode("USA", namerica);
  auto canada = sil->AddNode("Canada", namerica);
  (void)canada;

  auto field = sil->AddNode("Field");
  auto physics = sil->AddNode("Physics", field);
  auto chemistry = sil->AddNode("Chemistry", field);
  auto literature = sil->AddNode("Literature", field);
  (void)literature;

  auto thouless = sil->AddNode("Thouless", uk);
  sil->AddCrossLink(physics, thouless);

  auto haldane = sil->AddNode("Haldane", uk);
  sil->AddCrossLink(physics, haldane);

  auto kosterlitz = sil->AddNode("Kosterlitz", uk);
  sil->AddCrossLink(physics, kosterlitz);

  auto sauvage = sil->AddNode("Sauvage", eu);
  sil->AddCrossLink(chemistry, sauvage);

  auto stoddart = sil->AddNode("Stoddart", uk);
  sil->AddCrossLink(chemistry, stoddart);

  auto feringa = sil->AddNode("Feringa", eu);
  sil->AddCrossLink(chemistry, feringa);

  sil->Select(europe);
  // sil->Print(cout);

  sil->Deselect(eu);
  // sil->Print(cout);

  // this will add a new node.
  sil->Select("/World/North America/Mexico");
  if (sil->FindNode("//Mexico") == -1)
  {
    cout << "ERROR: Failed to add node through selection!" << endl;
  }
  // sil->Print(cout);

  Lookup(sil, "//USA", usa);
  Lookup(sil, "/Field/Physics", physics);
  Lookup(sil, "/World//Sauvage", sauvage);
  Lookup(sil, "//Sauvage", sauvage);

  auto state = sil->GetSelection();

  cout << "*** Deselect('/World') ***" << endl;
  sil->Deselect("/World");
  // sil->Print(cout);
  sil->SetSelection(state);
  sil->Print(cout);

  return EXIT_SUCCESS;
}

int TestB()
{
  vtkNew<vtkSubsetInclusionLattice> sil;
  sil->AddNodeAtPath("/base/blk-1_proc-0/Grid");
  sil->AddNodeAtPath("/base/blk-1_proc-0/bc-1");
  sil->AddNodeAtPath("/base/blk-1_proc-0/bc-2");
  sil->AddNodeAtPath("/base/blk-1_proc-0/bc-4");
  sil->AddNodeAtPath("/base/blk-1_proc-0/bc-5");
  sil->AddNodeAtPath("/base/blk-1_proc-0/bc-6");

  sil->AddNodeAtPath("/base/blk-1_proc-1/Grid");
  sil->AddNodeAtPath("/base/blk-1_proc-1/bc-1");
  sil->AddNodeAtPath("/base/blk-1_proc-1/bc-2");
  sil->AddNodeAtPath("/base/blk-1_proc-1/bc-5");
  sil->AddNodeAtPath("/base/blk-1_proc-1/bc-6");

  sil->Select("/base/blk-1_proc-0");
  sil->Print(cout);
  sil->Deselect("/base/blk-1_proc-0/Grid");
  sil->Print(cout);

  for (auto item : sil->GetSelection())
  {
    cout << item.first.c_str() << " : " << item.second << endl;
  }

  return EXIT_SUCCESS;
}

int TestSubsetInclusionLattice(int, char* [])
{
  int ret = TestA();
  ret = TestB() == EXIT_FAILURE ? EXIT_FAILURE : ret;
  return ret;
}
