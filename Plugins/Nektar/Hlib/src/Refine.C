
/**************************************************************************/
//                                                                        //
//   Author:    T.Warburton                                               //
//   Design:    T.Warburton && S.Sherwin                                  //
//   Date  :    12/4/96                                                   //
//                                                                        //
//   Copyright notice:  This code shall not be replicated or used without //
//                      the permission of the author.                     //
//                                                                        //
/**************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <polylib.h>
#include "veclib.h"
#include "hotel.h"

#include <stdio.h>

static void un_link(Element_List *U, int eid1, int id1);
static void add_bc(Element_List *EL, Bndry **Ubc, int nfields,
       int bid, int eid, int face);
static void set_link(Element_List *U, int eid1, int id1, int eid2, int id2);

/*

Function name: Element::split_edge

Function Purpose:

Argument 1: int edg
Purpose:

Argument 2: Element_List *EL
Purpose:

Argument 3: Bndry **Ubc
Purpose:

Argument 4: int nfields
Purpose:

Argument 5: int *flag
Purpose:

Function Notes:

*/

void Tri::split_edge(int edg, Element_List *EL, Bndry **Ubc, int nfields, int *flag){
  int i, j, newid = EL->nel;
  int *link_eid  = ivector(0, Nedges-1);
  int *link_face = ivector(0, Nedges-1);
  Bndry *Bc;

  // store links
  for(i=0;i<Nedges;++i){
    j = (edg+i)%Nedges;
    if(edge[j].base)
      if(edge[j].link){
  link_eid[i]  = edge[j].link->eid;
  link_face[i] = edge[j].link->id;
      }
      else{
  link_eid[i]  = edge[j].base->eid;
  link_face[i] = edge[j].base->id;
      }
    else{
      link_eid[i]  = -1;
      link_face[i] = -1;
    }
  }

  // set flag on this element
  //flag[id] = 2;

  Element *new_E;

  new_E = (Element*) new Tri(this);

  // setup one element at a time
  // setup vertex positions:

  Coord origX, newX, centX;

  // store original
  origX.x = dvector(0, Nverts-1);
  origX.y = dvector(0, Nverts-1);

  for(i=0;i<Nverts;++i){
    j = (edg+i)%Nverts;
    origX.x[i] = vert[j].x;
    origX.y[i] = vert[j].y;
  }

  // store edge mid points
  centX.x = dvector(0, Nedges-1);
  centX.y = dvector(0, Nedges-1);

  calc_edge_centers(this, &centX);

  centX.x[0] = centX.x[edg];
  centX.y[0] = centX.y[edg];

  newX.x = dvector(0, Nverts-1);
  newX.y = dvector(0, Nverts-1);

  // Element 1
  newX.x[0] = origX.x[2];  newX.y[0] = origX.y[2];
  newX.x[1] = centX.x[0];  newX.y[1] = centX.y[0];
  newX.x[2] = origX.x[1];  newX.y[2] = origX.y[1];

  move_vertices(&newX);
  //  set_geofac();

  // Element 2
  newX.x[0] = origX.x[2];  newX.y[0] = origX.y[2];
  newX.x[1] = origX.x[0];  newX.y[1] = origX.y[0];
  newX.x[2] = centX.x[0];  newX.y[2] = centX.y[0];

  new_E->move_vertices(&newX);

  if(curve){
    if(curve->face == edg){
      curve->face = 1;
      new_E->curve = (Curve*) calloc(1, sizeof(Curve));
      new_E->curve[0] = curve[0];
      new_E->curve->face = 1;

      nomem_set_curved(EL, this);
      nomem_set_curved(EL, new_E);
    }
    else if(curve->face == (edg+1)%Nverts){
      curve->face = 2;
      new_E->curve = (Curve*)0;

      nomem_set_curved(EL, this);
    }
    else{
      new_E->curve = curve;
      new_E->curve->face = 0;
      curve = (Curve*)0;

      nomem_set_curved(EL, new_E);
    }
  }

  // replace element list with new list
  Element **total_E = (Element**) calloc(EL->nel+1, sizeof(Element*));
  for(i=0;i<EL->nel;++i)
    total_E[i] = EL->flist[i];
  total_E[newid] = new_E;

  free(EL->flist);
  EL->flist = total_E;

  // setup link list
  EL->flist[newid-1]->next = new_E;
  new_E->next = (Element*)NULL;

  // number elements

  new_E->id = newid;
  //  EL->flist[newid]->set_geofac();

  for(i=0;i<new_E->Nverts;++i){
    new_E->edge[i].eid = newid;
    new_E->edge[i].id = i;
  }

  // need to set links to outside world

  // *************************************

  // reformed element
  set_link(EL, id, 0, newid, 2);

  if(link_eid[1] != -1)
    set_link(EL, id, 2, link_eid[1], link_face[1]);
  else
    for(i=0;i<nfields;++i)
      for(Bc=Ubc[i];Bc;Bc=Bc->next)
  if(Bc->elmt->id == id && (Bc->face == ((edg+1)%Nverts))){
    Bc->face = 2;
    un_link(EL, id, 2);
    break;
  }

  // new element
  if(link_eid[2] != -1)
    set_link(EL, newid, 0, link_eid[2], link_face[2]);
  else
    for(i=0;i<nfields;++i)
      for(Bc=Ubc[i];Bc;Bc=Bc->next)
  if(Bc->elmt->id == id && (Bc->face == ((edg+2)%Nverts))){
    Bc->face = 0;
    Bc->elmt = new_E;
    un_link(EL, newid, 0);
    break;
  }


  EL->nel += 1;

  free(origX.x);  free(origX.y);
  free(newX.x);   free(newX.y);
  free(centX.x);  free(centX.y);

}




void Quad::split_edge(int edg, Element_List *EL, Bndry **Ubc, int nfields, int *flag){
  //  fprintf(stderr, "Quad::split_edge not implemented yet\n");

  int i, j, newid = EL->nel;
  int *link_eid  = ivector(0, Nedges-1);
  int *link_face = ivector(0, Nedges-1);
  Bndry *Bc;

  // store links
  for(i=0;i<Nedges;++i){
    j = (edg+i)%Nedges;
    if(edge[j].base)
      if(edge[j].link){
  link_eid[i]  = edge[j].link->eid;
  link_face[i] = edge[j].link->id;
      }
      else{
  link_eid[i]  = edge[j].base->eid;
  link_face[i] = edge[j].base->id;
      }
    else{
      link_eid[i]  = -1;
      link_face[i] = -1;
    }
  }


  // setup one element at a time
  // setup vertex positions:

  Coord origX, newX, centX, EcentX;

  // store original
  origX.x = dvector(0, Nverts-1);
  origX.y = dvector(0, Nverts-1);

  for(i=0;i<Nverts;++i){
    j = (edg+i)%Nverts;
    origX.x[i] = vert[j].x;
    origX.y[i] = vert[j].y;
  }

  // store edge mid points
  centX.x = dvector(0, Nverts-1);
  centX.y = dvector(0, Nverts-1);

  calc_edge_centers(this, &centX);

  centX.x[0] = centX.x[edg];
  centX.y[0] = centX.y[edg];

  EcentX.x = dvector(0, 1);
  EcentX.y = dvector(0, 1);
  EcentX.x[0] = 0.0;  EcentX.y[0] = 0.0;
  for(i=0;i<Nverts;++i){
    EcentX.x[0] += vert[i].x;
    EcentX.y[0] += vert[i].y;
  }
  EcentX.x[0] /= (double)Nverts;
  EcentX.y[0] /= (double)Nverts;

  newX.x = dvector(0, Nverts-1);
  newX.y = dvector(0, Nverts-1);

  Element **new_E;
  new_E = (Element**) calloc(2, sizeof(Element*));

  // Element 1
  newX.x[0] =  EcentX.x[0];  newX.y[0] =  EcentX.y[0];
  newX.x[1] =   centX.x[0];  newX.y[1] =   centX.y[0];
  newX.x[2] =   origX.x[1];  newX.y[2] =   origX.y[1];
  newX.x[3] =   origX.x[2];  newX.y[3] =   origX.y[2];

  move_vertices(&newX);
  //  set_geofac();

  // Element 2
  newX.x[0] =   origX.x[3];  newX.y[0] =   origX.y[3];
  newX.x[1] =   origX.x[0];  newX.y[1] =   origX.y[0];
  newX.x[2] =   centX.x[0];  newX.y[2] =   centX.y[0];
  newX.x[3] =  EcentX.x[0];  newX.y[3] =  EcentX.y[0];

  new_E[0] = (Element*) new Quad(this);
  new_E[0]->move_vertices(&newX);


  // Element 3
  newX.x[0] =   origX.x[2];  newX.y[0] =   origX.y[2];
  newX.x[1] =   origX.x[3];  newX.y[1] =   origX.y[3];
  newX.x[2] =  EcentX.x[0];  newX.y[2] =  EcentX.y[0];

  new_E[1] = (Element*) new Tri(0, type, lmax, qa, qa-1, 0, &newX);
#if 1
  if(curve){
    if(curve->face == edg){
      curve->face = 1;
      nomem_set_curved(EL, this);

      new_E[0]->curve = (Curve*) calloc(1, sizeof(Curve));
      new_E[0]->curve[0] = curve[0];
      new_E[0]->curve->face = 1;
      nomem_set_curved(EL, new_E[0]);

      new_E[1]->curve = (Curve*)0;
      new_E[1]->curvX = (Cmodes*)0;

    }
    else if(curve->face == (edg+1)%Nverts){
      curve->face = 2;
      nomem_set_curved(EL, this);

      new_E[0]->curve = (Curve*)0;
      new_E[0]->curvX = (Cmodes*)0;

      new_E[1]->curve = (Curve*)0;
      new_E[1]->curvX = (Cmodes*)0;
    }
    else if(curve->face == (edg+2)%Nverts){
#if 1
      new_E[1]->curve = (Curve*) calloc(1, sizeof(Curve));
      memcpy(new_E[1]->curve, curve, sizeof(Curve));
      //      new_E[1]->curve[0] = curve[0];
      new_E[1]->curve->face = 0;
      nomem_set_curved(EL, new_E[1]);

      curve = (Curve*)0;
      curvX = (Cmodes*)0;

      new_E[0]->curve = (Curve*)0;
      new_E[0]->curvX = (Cmodes*)0;
#endif
    }
    else if(curve->face == (edg+3)%Nverts){
      curve = (Curve*)0;
      curvX = (Cmodes*)0;

      new_E[0]->curve->face = 0;
      nomem_set_curved(EL, new_E[0]);

      new_E[1]->curve = (Curve*)0;
      new_E[1]->curvX = (Cmodes*)0;

    }
  }
#endif

  // must check links

  // replace element list with new list
  Element **total_E = (Element**) calloc(EL->nel+2, sizeof(Element*));
  for(i=0;i<EL->nel;++i)
    total_E[i] = EL->flist[i];
  total_E[newid] = new_E[0];
  total_E[newid+1] = new_E[1];

  free(EL->flist);
  EL->flist = total_E;

  // setup link list
  EL->flist[newid-1]->next = new_E[0];
  EL->flist[newid]->next   = new_E[1];
  EL->flist[newid+1]->next = (Element*)NULL;

  // number elements

  new_E[0]->id = newid;
  new_E[1]->id = newid+1;

  for(i=0;i<new_E[0]->Nverts;++i){
    new_E[0]->edge[i].eid = newid;
    new_E[0]->edge[i].id = i;
  }
  for(i=0;i<new_E[1]->Nverts;++i){
    new_E[1]->edge[i].eid = newid+1;
    new_E[1]->edge[i].id = i;
  }

  // need to set links to outside world

  //

  // Element 1
  set_link(EL, id, 0, newid, 2);

  if(link_eid[1] != -1)
    set_link(EL, id, 2, link_eid[1], link_face[1]);
  else
    for(i=0;i<nfields;++i)
      for(Bc=Ubc[i];Bc;Bc=Bc->next)
  if(Bc->elmt->id == id && (Bc->face == ((edg+1)%Nverts))){
    Bc->face = 2;
    un_link(EL, id, 2);
    break;
  }

  // Element 2
  if(link_eid[3] != -1)
    set_link(EL, newid, 0, link_eid[3], link_face[3]);
  else
    for(i=0;i<nfields;++i)
      for(Bc=Ubc[i];Bc;Bc=Bc->next)
  if(Bc->elmt->id == id && (Bc->face == ((edg+3)%Nverts))){
    Bc->face = 0;
    Bc->elmt = new_E[0];
    un_link(EL, newid, 0);
    break;
  }


  // Element 3
  if(link_eid[2] != -1)
    set_link(EL, newid+1, 0, link_eid[2], link_face[2]);
  else
    for(i=0;i<nfields;++i)
      for(Bc=Ubc[i];Bc;Bc=Bc->next)
  if(Bc->elmt->id == id && (Bc->face == ((edg+2)%Nverts))){
    Bc->face = 0;
    Bc->elmt = new_E[1];
    un_link(EL, newid+1, 0);
    break;
  }

  set_link(EL, id, 3, newid+1, 2);
  set_link(EL, newid, 3, newid+1, 1);

  EL->nel += 2;

  free(origX.x);  free(origX.y);
  free(newX.x);   free(newX.y);
  free(centX.x);  free(centX.y);
  free(EcentX.x);  free(EcentX.y);

}




void Tet::split_edge(int, Element_List *EL, Bndry **Ubc, int nfields, int *flag){
  fprintf(stderr, "Hex::split_edge not implemented yet\n");
  return;
}




void Pyr::split_edge(int edg, Element_List *EL, Bndry **, int, int *flag){
  fprintf(stderr, "Pyr::split_edge not implemented yet\n");
  return;
}




void Prism::split_edge(int edg, Element_List *EL, Bndry **, int , int *flag){
  fprintf(stderr, "Prism::split_edge not implemented yet\n");
  return;
}




void Hex::split_edge(int, Element_List *EL, Bndry **Ubc, int nfields, int *flag){
  fprintf(stderr, "Hex::split_edge not implemented yet\n");
  return;
}




void Element::split_edge(int edg, Element_List *EL, Bndry **Ubc, int nfields,int *flag){ERR;}




/*

Function name: Element::split_element

Function Purpose:
 Split element, taking into account modification of neighbour elements.

Argument 1: Element_List *EL
Purpose:
 List of all elements. Other elements can be modified/added.

Argument 2: Bndry **Ubc
Purpose:
 List of all boundary conditions. Boundary conditions can be modified/added.

Argument 3: int nfields
Purpose:
 Number of fields that boundary conditions are supplied for.

Argument 4: int *&flag
Purpose:
 Flag that denotes which splitting algorithm is to be used:

 Options:
 kirby

Function Notes:

*/

void Tri::split_element(Element_List *EL, Bndry **Ubc, int nfields, int *&flag){
  Element *E;
  Bndry *Bc;
  int i, j, k, nel = EL->nel;
  Coord origX, newX, centX;
  Element **total_E;
  int cnt = 0;

  //  fprintf(stderr, "Tri::split_element not implemented yet\n");

  // This routine takes the triangle
  // Splits it into 4 triangles
  // Splits each neighbour into 2 triangles..

  //           /|                            /|
  //          / |                   / |
  //         /  |                  / .|
  //        /   |                 / . |
  //       /    |                / .  |
  //      -------  ===========>         -------
  //     /|    /\              /| /\ /\
  //    / |   /  \            / |/_ /  \
  //   /  |  /    \          / .|  / .  \
  //  /   | /      \        / . | /   .  \
  // /----|/--------       /----|/--------

  //the flag array is used to designate which splitting operation
  //is to take place, if flag[0]=0, the triangle is split into
  // three, with side splitting.  if flag[0]=3 the triangle is
  //split into three with no side splitting.  if flag[0]=1 or
  //flag[0]=2, then the ints following flag[0] designate the
  //edge to be split.  Note, if flag[0]>0, no element adjacent
  //to the element is split.

  //Assumed: edge order given in flag array is counter clockwise (ie 123, 312, etc..)

  //It is assumed that the flag array is declared dynamically.  Once the information is used,
  //The array is deleted, and a new array is passed back in its place.  The new array contains
  //flag[0] = number elements with unconnected edges
  //flag[1] = First unconnected element's id
  //flag[2] = number of unconnected edges
  //flag[3] = Second unconnected element's id

  Element * Quad_element;
  int *link_eid  = ivector(0, Nedges-1);
  int *link_face = ivector(0, Nedges-1);

  // store links
  for(i=0;i<Nedges;++i){
    if(edge[i].base)
      if(edge[i].link){
  link_eid[i] = edge[i].link->eid;
  link_face[i] = edge[i].link->id;
      }
      else{
  link_eid[i] = edge[i].base->eid;
  link_face[i] = edge[i].base->id;
      }
    else{
      link_eid[i] = -1;
      link_face[i] = -1;
    }
  }


  int dummy_flag=0;
  //case flag[0]=0 will be treated as the default.

  switch(flag[0]){
  case 1:
    split_edge(flag[1], EL, Ubc, nfields, &dummy_flag);
    edge[1].base = NULL;
    EL->flist[EL->nel-1]->edge[1].base=NULL;

    if(link_eid[flag[1]] == -1){
      for(Bc=Ubc[0];Bc;Bc=Bc->next)
  ++cnt;

      // check for bc on edge 0
      un_link(EL, id, 1);
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == flag[1]){
    Bc->face = 1;
    add_bc(EL, Ubc, nfields, k, EL->nel-1, 1);
    break;
  }//end if
      }//end for k
    }//end link_eid if

    delete[] flag;
    flag = new int[5];
    flag[0] = 2;
    flag[1] = id;
    flag[2] = 1;
    flag[3] = EL->nel-1;
    flag[4] = 1;
    break;


  case 2:
    if((flag[2]+1)%Nverts == flag[1])
      {
  int dummy = flag[1];
  flag[1]=flag[2];
  flag[2]=dummy;
      }//endif

    // store original
    origX.x = dvector(0, Nverts-1);
    origX.y = dvector(0, Nverts-1);

    for(i=0;i<Nverts;++i){
      origX.x[i] = vert[i].x;
      origX.y[i] = vert[i].y;
    }

    // store edge mid points

    centX.x = dvector(0, Nverts-1);
    centX.y = dvector(0, Nverts-1);

    calc_edge_centers(this, &centX);

    newX.x = dvector(0, Nverts-1);
    newX.y = dvector(0, Nverts-1);

   // Element 1 //Triangle element
    newX.x[0] = centX.x[flag[1]];     newX.y[0] = centX.y[flag[1]];
    newX.x[1] = origX.x[flag[2]];     newX.y[1] = origX.y[flag[2]];
    newX.x[2] = centX.x[flag[2]];     newX.y[2] = centX.y[flag[2]];

    move_vertices(&newX);

    free(newX.x);
    free(newX.y);

    newX.x = dvector(0, Nverts); //Nverts will be 3, so we are allocating for a quad
    newX.y = dvector(0, Nverts);

    // Element 2 //Quad element
    newX.x[0] = origX.x[flag[1]];             newX.y[0] = origX.y[flag[1]];
    newX.x[1] = centX.x[flag[1]];             newX.y[1] = centX.y[flag[1]];
    newX.x[2] = centX.x[flag[2]];             newX.y[2] = centX.y[flag[2]];
    newX.x[3] = origX.x[(flag[2]+1)%Nverts];  newX.y[3] = origX.y[(flag[2]+1)%Nverts];

    Quad_element = (Element*) new Quad(EL->nel, type, lmax, qa, qa, 0, &newX);

    //clean up the heap space allocation
    free(newX.x);
    free(newX.y);
    free(centX.x);
    free(centX.y);
    free(origX.x);
    free(origX.y);

    if(curve){
      if(curve->face == flag[1]){
  curve->face = 0;
  nomem_set_curved(EL, this);

  Quad_element->curve = (Curve*) calloc(1, sizeof(Curve));
  Quad_element->curve[0] = curve[0];
  Quad_element->curve->face = 0;
  nomem_set_curved(EL, Quad_element);
      }
      else if(curve->face == flag[2]){
  curve->face = 1;
  nomem_set_curved(EL, this);

  Quad_element->curve = (Curve*) calloc(1, sizeof(Curve));
  Quad_element->curve[0] = curve[0];
  Quad_element->curve->face = 2;
  nomem_set_curved(EL, Quad_element);
      }
      else {
  Quad_element->curve = curve;
  curve = (Curve *)0;
  Quad_element->curve->face = 3;
  nomem_set_curved(EL, Quad_element);
      }
    }//end if curve


    // replace element list with new list
    total_E = (Element**) calloc(EL->nel+1, sizeof(Element*));
    for(i=0;i<EL->nel;++i)
      total_E[i] = EL->flist[i];

    total_E[EL->nel] = Quad_element;

    free(EL->flist);
    EL->flist = total_E;

    // add new quad to the end of the linked list
    EL->flist[EL->nel-1]->next = EL->flist[EL->nel];
    EL->flist[EL->nel]->next = (Element*) 0; //set the last elements next to NULL

    Quad_element->id = EL->nel;
    for(j=0;j<Quad_element->Nverts;++j)
      {
  Quad_element->edge[j].eid = EL->nel;
  Quad_element->edge[j].id  = j;
      }

    set_link(EL, EL->nel,1,id,2); //set inside edge connection (between tri and quad)
    edge[0].base = NULL;
    edge[1].base = NULL;
    Quad_element->edge[0].base = NULL;
    Quad_element->edge[2].base = NULL;
    (EL->nel)++; //increment the number of elements by one


    for(Bc=Ubc[0];Bc;Bc=Bc->next)
      ++cnt;


    if(link_eid[flag[1]] == -1){
      // check for bc on edge 0
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == flag[1]){
    Bc->face = 0;
    add_bc(EL, Ubc, nfields, k, EL->nel-1, 0);
    break;
  }//end if
      }//end for k
    }//end link_eid if

    if(link_eid[flag[2]] == -1){
      // check for bc on edge 0
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == flag[2]){
    Bc->face = 1;
    add_bc(EL, Ubc, nfields, k, EL->nel-1, 2);
    break;
  }//end if
      }//end for k
    }//end link_eid if

    if(link_eid[(flag[2]+1)%Nverts]!=-1){
      set_link(EL, EL->nel-1,3,link_eid[(flag[2]+1)%Nverts],link_face[(flag[2]+1)%Nverts]); //connection to the outside element
    }
    else{
      // check for bc on edge 0
      un_link(EL, id, (flag[2]+1)%Nverts);
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == (flag[2]+1)%Nverts){
    Bc->face = 1;
    add_bc(EL, Ubc, nfields, k, EL->nel-1, 3);
    break;
  }//end if
      }//end for k
    }//end link_eid if

    delete[] flag;
    flag = new int[5];
    flag[0] = 2;
    flag[1] = id;
    flag[2] = 2;
    flag[3] = EL->nel-1;
    flag[4] = 2;

    break;
  case 3:
  default:

    if(flag[0]!=3) //if flag[0]=3, we want no side splitting
      for(i=0;i<Nverts;++i)
  if(link_eid[i] != -1)
    EL->flist[link_eid[i]]->split_edge(link_face[i], EL, Ubc, nfields, &dummy_flag);

    Element** new_E = (Element**) calloc(3, sizeof(Element*));

    new_E[0] = (Element*) new Tri(this);
    new_E[1] = (Element*) new Tri(this);
    new_E[2] = (Element*) new Tri(this);

    // setup one element at a time
    // setup vertex positions:

    // store original
    origX.x = dvector(0, Nverts-1);
    origX.y = dvector(0, Nverts-1);

    for(i=0;i<Nverts;++i){
      origX.x[i] = vert[i].x;
      origX.y[i] = vert[i].y;
    }

    // store edge mid points

    centX.x = dvector(0, Nverts-1);
    centX.y = dvector(0, Nverts-1);

    calc_edge_centers(this, &centX);

    newX.x = dvector(0, Nverts-1);
    newX.y = dvector(0, Nverts-1);

    // Element 1
    newX.x[0] = origX.x[0];  newX.y[0] = origX.y[0];
    newX.x[1] = centX.x[0];  newX.y[1] = centX.y[0];
    newX.x[2] = centX.x[2];  newX.y[2] = centX.y[2];

    move_vertices(&newX);

    // Element 2
    newX.x[0] = origX.x[1];  newX.y[0] = origX.y[1];
    newX.x[1] = centX.x[1];  newX.y[1] = centX.y[1];
    newX.x[2] = centX.x[0];  newX.y[2] = centX.y[0];

    new_E[0]->move_vertices(&newX);

    // Element 3
    newX.x[0] = origX.x[2];  newX.y[0] = origX.y[2];
    newX.x[1] = centX.x[2];  newX.y[1] = centX.y[2];
    newX.x[2] = centX.x[1];  newX.y[2] = centX.y[1];

    new_E[1]->move_vertices(&newX);

    // Element 4
    newX.x[0] = centX.x[0];  newX.y[0] = centX.y[0];
    newX.x[1] = centX.x[1];  newX.y[1] = centX.y[1];
    newX.x[2] = centX.x[2];  newX.y[2] = centX.y[2];

    new_E[2]->move_vertices(&newX);

    if(curve){
      switch(curve->face){
      case 0:
  curve->face = 0;
  nomem_set_curved(EL, this);

  new_E[0]->curve = (Curve*) calloc(1, sizeof(Curve));
  new_E[0]->curve[0] = curve[0];
  new_E[0]->curve->face = 2;
  nomem_set_curved(EL, new_E[0]);

  new_E[1]->curve = (Curve*)0;
  new_E[1]->curvX = (Cmodes*)0;

  new_E[2]->curve = (Curve*)0;
  new_E[2]->curvX = (Cmodes*)0;
  break;
      case 1:

  new_E[0]->curve = curve;
  new_E[0]->curvX = curvX;
  new_E[0]->curve->face = 0;
  nomem_set_curved(EL, new_E[0]);

  curve = (Curve*)0;
  curvX = (Cmodes*)0;

  new_E[1]->curve = (Curve*) calloc(1, sizeof(Curve));
  new_E[1]->curve[0] = new_E[0]->curve[0];
  new_E[1]->curve->face = 2;
  nomem_set_curved(EL, new_E[1]);

  new_E[2]->curve = (Curve*)0;
  new_E[2]->curvX = (Cmodes*)0;
      break;
      case 2:
  curve->face = 2;
  nomem_set_curved(EL, this);

  new_E[0]->curve = (Curve*)0;
  new_E[0]->curvX = (Cmodes*)0;

  new_E[1]->curve    = (Curve*) calloc(1, sizeof(Curve));
  new_E[1]->curve[0] = curve[0];
  new_E[1]->curve->face = 0;
  nomem_set_curved(EL, new_E[1]);

  new_E[2]->curve = (Curve*)0;
  new_E[2]->curvX = (Cmodes*)0;
  break;
      }
    }

    // replace element list with new list
    total_E = (Element**) calloc(EL->nel+3, sizeof(Element*));
    for(i=0;i<EL->nel;++i)
      total_E[i] = EL->flist[i];
    for(i=EL->nel;i<EL->nel+3;++i)
      total_E[i] = new_E[i-EL->nel];

    free(EL->flist);
    EL->flist = total_E;

    // setup link list
    for(i=EL->nel-1;i<EL->nel+2;++i)
      EL->flist[i]->next = EL->flist[i+1];
    EL->flist[i]->next = (Element*) 0;

    // number elements
    for(i=EL->nel;i<EL->nel+3;++i){
      E = EL->flist[i];
      E->id = i;
      for(j=0;j<E->Nverts;++j){
  E->edge[j].eid = i;
  E->edge[j].id  = j;
      }
    }

    // need to set links
    // this is done in a specific order

    for(Bc=Ubc[0];Bc;Bc=Bc->next)
      ++cnt;

    if(link_eid[0] != -1){
      if(flag[0]!=3){
  set_link(EL, id, 0, link_eid[0], 1);
  set_link(EL, EL->nel, 2, nel, 1);
  nel += (EL->flist[link_eid[0]]->identify() == Nek_Tri) ? 1:2;
      }//endif
    }
    else{
      // check for bc on edge 0
      un_link(EL, id, 0);
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == 0){
    add_bc(EL, Ubc, nfields, k, EL->nel, 2);
    break;
  }
      }//end for k
    }//end else

    if(link_eid[1] != -1){
      if(flag[0]!=3){
  set_link(EL, EL->nel, 0, link_eid[1], 1);
  set_link(EL, EL->nel+1, 2, nel, 1);
  nel += (EL->flist[link_eid[1]]->identify() == Nek_Tri) ? 1:2;
      }//endif
    }
    else{
      // check for bc on edge 1
      un_link(EL, EL->nel, 0);
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == 1){
    Ubc[0][k].face = 0;
    Ubc[0][k].elmt = EL->flist[EL->nel];
    add_bc(EL, Ubc, nfields, k, EL->nel+1, 2);
    break;
  }
      }
    }

    if(link_eid[2] != -1){
      if(flag[0]!=3){
  set_link(EL, EL->nel+1, 0, link_eid[2], 1);
  set_link(EL, id, 2, nel, 1);
  nel += (EL->flist[link_eid[2]]->identify() == Nek_Tri) ? 1:2;
      }
    }
    else {
      // check for bc on edge 2
      un_link(EL, EL->nel+1, 0);
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == 2){
    Ubc[0][k].face = 0;
    Ubc[0][k].elmt = EL->flist[EL->nel+1];
    add_bc(EL, Ubc, nfields, k, id, 2);
    break;
    }
      }
    }//end else


    if(flag[0]==3){
      for(int j=0; j < Nedges; j++)
  {
    edge[j].base = NULL;
    for(int k=0; k < 3; k++)
      new_E[k]->edge[j].base = NULL;
  }//end for j
    }//end if


    set_link(EL,   EL->nel, 1, EL->nel+2, 0);
    set_link(EL, EL->nel+1, 1, EL->nel+2, 1);
    set_link(EL,        id, 1, EL->nel+2, 2);

    // need to fix edge numbering
    EL->nel += 3;

    free(origX.x);  free(origX.y);
    free(newX.x);   free(newX.y);
    free(centX.x);  free(centX.y);
    if(flag[0]!=0)
      {
  delete[] flag;
  flag = new int[7];
  flag[0] = 3;
  flag[1] = id;
  flag[2] = 2;
  flag[3] = EL->nel-3;
  flag[4] = 2;
  flag[5] = EL->nel-2;
  flag[6] = 2;
      }//end if
  }//end of switch statement

  return;
}//end of split_element member function




void Quad::split_element(Element_List *EL, Bndry **Ubc, int nfields, int *&flag){

  //  fprintf(stderr, "Quad::split_element not implemented yet\n");

  Element *E;
  Bndry *Bc;
  int i, j, k, nel = EL->nel;


  // This routine takes the Quad
  // Splits it into 4 quad.s
  // Splits each neighbour into 2 elements

  //            --------
  //            |     .|
  //            |.   . |
  //            | . .  |
  //            |  .   |
  //            ----------------
  //           /|  .   |   ..  |
  //          / |  .   | ..    |
  //         / .|......|.      |
  //        /.  |  .   |  ..   |
  //       /.   |  .   |    .. |
  //      ----------------------
  //            |  .  /
  //            | .  /
  //            | . /
  //            |. /
  //            |./
  //            |/


 //the flag array is used to designate which splitting operation
  //is to take place, if flag[0]=0, the triangle is split into
  // three, with side splitting.  if flag[0]=3 the triangle is
  //split into three with no side splitting.  if flag[0]=1 or
  //flag[0]=2, then the ints following flag[0] designate the
  //edge to be split.  Note, if flag[0]>0, no element adjacent
  //to the element is split.

  //Assumed: edge order given in flag array is counter clockwise (ie 123, 312, etc..)

  //It is assumed that the flag array is declared dynamically.  Once the information is used,
  //The array is deleted, and a new array is passed back in its place.  The new array contains
  //flag[0] = number elements with unconnected edges
  //flag[1] = First unconnected element's id
  //flag[2] = number of unconnected edges ...
  //flag[3] = Second unconnected element's id

  int *link_eid  = ivector(0, Nedges-1);
  int *link_face = ivector(0, Nedges-1);
  Element ** new_E;
  Element ** total_E;
  Coord origX, newX, centX, EcentX;

  // store links
  for(i=0;i<Nedges;++i){
    if(edge[i].base)
      if(edge[i].link){
  link_eid[i] = edge[i].link->eid;
  link_face[i] = edge[i].link->id;
      }
      else{
  link_eid[i] = edge[i].base->eid;
  link_face[i] = edge[i].base->id;
      }
    else{
      link_eid[i] = -1;
      link_face[i] = -1;
    }
  }

  int dummy_flag = 0;
  //case flag[0] = 0 will be treated as the default

  switch(flag[0]){
  case 1:
    split_edge(flag[1],EL,Ubc,nfields,&dummy_flag);
    edge[1].base = NULL;
    EL->flist[EL->nel-2]->edge[1].base=NULL;
    delete [] flag;
    flag = new int[5];
    flag[0] = 2;
    flag[1] = id;
    flag[2] = 1;
    flag[3] = EL->nel-2;
    flag[4] = 1;
    break;

  case 2:
    //this is done to for ordering purposes (must be counter-clockwise)
    if((flag[2]+1)%Nverts == flag[1])
      {
  int dummy = flag[1];
  flag[1]=flag[2];
  flag[2]=dummy;
      }//endif

    new_E = (Element**) calloc(2, sizeof(Element*));

    new_E[0] = (Element*) new Quad(this);
    new_E[1] = (Element*) new Quad(this);

    // setup one element at a time
   // setup vertex positions:


    // store original
    origX.x = dvector(0, Nverts-1);
    origX.y = dvector(0, Nverts-1);

    for(i=0;i<Nverts;++i){
      origX.x[i] = vert[i].x;
      origX.y[i] = vert[i].y;
    }

    // store edge mid points
    centX.x = dvector(0, Nverts-1);
    centX.y = dvector(0, Nverts-1);

    calc_edge_centers(this, &centX);

    EcentX.x = dvector(0, 1);
    EcentX.y = dvector(0, 1);
    EcentX.x[0] = 0.0;  EcentX.y[0] = 0.0;
    for(i=0;i<Nverts;++i){
      EcentX.x[0] += vert[i].x;
      EcentX.y[0] += vert[i].y;
    }
    EcentX.x[0] /= (double)Nverts;
    EcentX.y[0] /= (double)Nverts;

    newX.x = dvector(0, Nverts-1);
    newX.y = dvector(0, Nverts-1);

    // Element 1
    newX.x[0] =  centX.x[flag[1]];  newX.y[0] =  centX.y[flag[1]];
    newX.x[1] =  origX.x[flag[2]];  newX.y[1] =  origX.y[flag[2]];
    newX.x[2] =  centX.x[flag[2]];  newX.y[2] =  centX.y[flag[2]];
    newX.x[3] =  EcentX.x[0];       newX.y[3] =  EcentX.y[0];

    move_vertices(&newX);

    // Element 2
    newX.x[0] =  centX.x[flag[2]];            newX.y[0] = centX.y[flag[2]];
    newX.x[1] =  origX.x[(flag[2]+1)%Nverts]; newX.y[1] = origX.y[(flag[2]+1)%Nverts];
    newX.x[2] =  origX.x[(flag[2]+2)%Nverts]; newX.y[2] = origX.y[(flag[2]+2)%Nverts];
    newX.x[3] =  EcentX.x[0];                 newX.y[3] = EcentX.y[0];

    new_E[0]->move_vertices(&newX);

    // Element 3
    newX.x[0] =  origX.x[(flag[2]+2)%Nverts]; newX.y[0] =  origX.y[(flag[2]+2)%Nverts];
    newX.x[1] =  origX.x[flag[1]];            newX.y[1] =  origX.y[flag[1]];
    newX.x[2] =  centX.x[flag[1]];            newX.y[2] =  centX.y[flag[1]];
    newX.x[3] =  EcentX.x[0];                 newX.y[3] =  EcentX.y[0];

    new_E[1]->move_vertices(&newX);

   if(curve && curve->face != -1){
     if(curve->face == flag[1]) {
  curve->face = 0;
  nomem_set_curved(EL, this);

  new_E[1]->curve = (Curve*) calloc(1, sizeof(Curve));
  new_E[1]->curve[0] = curve[0];
  new_E[1]->curve->face = 1;
  nomem_set_curved(EL, new_E[1]);

  new_E[0]->curve = (Curve*)0;
  new_E[0]->curvX = (Cmodes*)0;
     } else if(curve->face == flag[2]) {
       curve->face = 1;
       nomem_set_curved(EL, this);

       new_E[0]->curve = (Curve*) calloc(1, sizeof(Curve));
       new_E[0]->curve[0] = curve[0];
       new_E[0]->curve->face = 0;
       nomem_set_curved(EL, new_E[0]);

       new_E[1]->curve = (Curve*)0;
       new_E[1]->curvX = (Cmodes*)0;
     }else if(curve->face == (flag[2]+1)%Nverts){
       new_E[0]->curve = curve;
       new_E[0]->curvX = curvX;
       new_E[0]->curve->face = 1;
       nomem_set_curved(EL, new_E[0]);

       curve = (Curve*)0;
       curvX = (Cmodes*)0;

       new_E[1]->curve =  (Curve*)0;
       new_E[1]->curvX =  (Cmodes*)0;
     } else {
       new_E[1]->curve = curve;
       new_E[1]->curvX = curvX;
       new_E[1]->curve->face = 0;
       nomem_set_curved(EL, new_E[1]);

       curve = (Curve*)0;
       curvX = (Cmodes*)0;

       new_E[0]->curve =  (Curve*)0;
       new_E[0]->curvX =  (Cmodes*)0;
     }
   }//end if curve

    // replace element list with new list
    total_E = (Element**) calloc(EL->nel+2, sizeof(Element*));
    for(i=0;i<EL->nel;++i)
      total_E[i] = EL->flist[i];
    for(i=EL->nel;i<EL->nel+2;++i)
      total_E[i] = new_E[i-EL->nel];

    free(EL->flist);
    EL->flist = total_E;

    // setup link list
    for(i=EL->nel-1;i<EL->nel+1;++i)
      EL->flist[i]->next = EL->flist[i+1];
    EL->flist[i]->next = (Element*) 0;

    // number elements
    for(i=EL->nel;i<EL->nel+2;++i){
      E = EL->flist[i];
      E->id = i;
      for(j=0;j<E->Nverts;++j){
  E->edge[j].eid = i;
  E->edge[j].id  = j;
      }
    }

    for(i=0; i<Nedges; i++) {
      edge[i].base = NULL;
      for(int j=0; j < 2; j++)
  new_E[j]->edge[i].base = NULL;
    }//end for i


    set_link(EL, EL->nel, 1, link_eid[(flag[2]+1)%Nverts], link_face[(flag[2]+1)%Nverts]);
    set_link(EL, EL->nel+1, 0, link_eid[(flag[2]+2)%Nverts], link_face[(flag[2]+2)%Nverts]);
    set_link(EL, id, 2, EL->nel, 3);
    set_link(EL, id, 3, EL->nel+1, 2);
    set_link(EL, EL->nel, 2, EL->nel+1, 3);
    EL->nel = EL->nel+2;

    if(link_eid[flag[1]] == -1)
      for(i=0;i<nfields;++i)
  for(Bc=Ubc[i];Bc;Bc=Bc->next)
    if(Bc->elmt->id == id && (Bc->face == flag[1])){
      Bc->face = 0;
      add_bc(EL, Ubc, nfields, Bc->id, EL->nel-1, 1);
      break;
    }

    if(link_eid[flag[2]] == -1)
      for(i=0;i<nfields;++i)
  for(Bc=Ubc[i];Bc;Bc=Bc->next)
    if(Bc->elmt->id == id && (Bc->face == flag[2])){
      Bc->face = 1;
      add_bc(EL, Ubc, nfields, Bc->id, EL->nel-2, 0);
      break;
    }

    if(link_eid[(flag[2]+1)%Nverts] == -1)
      for(i=0;i<nfields;++i)
  for(Bc=Ubc[i];Bc;Bc=Bc->next)
    if(Bc->elmt->id == id && (Bc->face == (flag[2]+1)%Nverts)){
      Bc->face = 1;
      Bc->elmt = new_E[0];
      break;
    }

    if(link_eid[(flag[2]+2)%Nverts] == -1)
      for(i=0;i<nfields;++i)
  for(Bc=Ubc[i];Bc;Bc=Bc->next)
    if(Bc->elmt->id == id && (Bc->face == (flag[2]+2)%Nverts)){
      Bc->face = 0;
      Bc->elmt = new_E[1];
      break;
    }

    delete[] flag;
    flag = new int[7];
    flag[0] = 3;
    flag[1] = id;
    flag[2] = 2;
    flag[3] = EL->nel-2;
    flag[4] = 1;
    flag[5] = EL->nel-1;
    flag[6] = 1;
    break;

  case 3:
      //this is done to for ordering purposes (must be counter-clockwise)
    if((flag[3]+1)%Nverts == flag[1]) {
      if(flag[2]+1 == flag[3]){
  int dummy = flag[1];
  flag[1]= flag[2];
  flag[2]= flag[3];
  flag[3]= dummy;
      }//end if ( 0 2 3 case)
      else {
  int dummy = flag[3];
  flag[3] = flag[2];
  flag[2] = flag[1];
  flag[1] = dummy;
      }//end if
   }//end if


    new_E = (Element**) calloc(3, sizeof(Element*));


    // setup one element at a time
   // setup vertex positions:


    // store original
    origX.x = dvector(0, Nverts-1);
    origX.y = dvector(0, Nverts-1);

    for(i=0;i<Nverts;++i){
      origX.x[i] = vert[i].x;
      origX.y[i] = vert[i].y;
    }

    // store edge mid points
    centX.x = dvector(0, Nverts-1);
    centX.y = dvector(0, Nverts-1);

    calc_edge_centers(this, &centX);

    EcentX.x = dvector(0, 1);
    EcentX.y = dvector(0, 1);
    EcentX.x[0] = 0.0;  EcentX.y[0] = 0.0;
    for(i=0;i<Nverts;++i){
      EcentX.x[0] += vert[i].x;
      EcentX.y[0] += vert[i].y;
    }
    EcentX.x[0] /= (double)Nverts;
    EcentX.y[0] /= (double)Nverts;

    newX.x = dvector(0, Nverts-1);
    newX.y = dvector(0, Nverts-1);

    // Element 1
    newX.x[0] = centX.x[flag[3]];             newX.y[0] =  centX.y[flag[3]];
    newX.x[1] = origX.x[(flag[3]+1)%Nedges];  newX.y[1] =  origX.y[(flag[3]+1)%Nedges];
    newX.x[2] = origX.x[(flag[3]+2)%Nedges];  newX.y[2] =  origX.y[(flag[3]+2)%Nedges];
    newX.x[3] = centX.x[(flag[3]+2)%Nedges];  newX.y[3] =  centX.y[(flag[3]+2)%Nedges];

    move_vertices(&newX);

    // Element 2
    newX.x[0] =  centX.x[(flag[3]+2)%Nedges];  newX.y[0] =  centX.y[(flag[3]+2)%Nedges];
    newX.x[1] =  origX.x[flag[2]];             newX.y[1] = origX.y[flag[2]];
    newX.x[2] =  centX.x[flag[2]];             newX.y[2] = centX.y[flag[2]];


    new_E[0] = (Element*) new Tri(EL->nel, type, lmax, qa, qb-1, 0, &newX);

    // Element 3
    newX.x[0] =  centX.x[flag[2]];             newX.y[0] = centX.y[flag[2]];
    newX.x[1] =  origX.x[flag[3]];             newX.y[1] = origX.y[flag[3]];
    newX.x[2] =  centX.x[flag[3]];             newX.y[2] = centX.y[flag[3]];

    new_E[1] = (Element*) new Tri(EL->nel, type, lmax, qa, qb-1, 0, &newX);

    //Element 4
    newX.x[0] =  centX.x[flag[2]];             newX.y[0] = centX.y[flag[2]];
    newX.x[1] =  centX.x[flag[3]];             newX.y[1] = centX.y[flag[3]];
    newX.x[2] =  centX.x[flag[1]];             newX.y[2] = centX.y[flag[1]];

    new_E[2] = (Element*) new Tri(EL->nel, type, lmax, qa, qb-1, 0, &newX);

   if(curve && curve->face != -1){
     if(curve->face == flag[1]) {
  curve->face = 2;
  nomem_set_curved(EL, this);

  new_E[0]->curve = (Curve*) calloc(1, sizeof(Curve));
  new_E[0]->curve[0] = curve[0];
  new_E[0]->curve->face = 0;
  nomem_set_curved(EL, new_E[0]);

  new_E[1]->curve = (Curve*)0;
  new_E[2]->curve = (Curve*)0;
     } else if(curve->face == flag[2]) {
       new_E[0]->curve = curve;
       new_E[0]->curve->face = 1;
       nomem_set_curved(EL, new_E[0]);

       new_E[1]->curve = (Curve*) calloc(1, sizeof(Curve));
       new_E[1]->curve[0] = curve[0];
       new_E[1]->curve->face = 0;
       nomem_set_curved(EL, new_E[1]);

       new_E[2]->curve = (Curve*)0;
       curve = (Curve*)0;
       curvX = (Cmodes*)0;
     }else if(curve->face == flag[3]){
       curve->face = 0;
       nomem_set_curved(EL, this);

       new_E[1]->curve = (Curve*) calloc(1, sizeof(Curve));
       new_E[1]->curve[0] = curve[0];
       new_E[1]->curve->face = 1;
       nomem_set_curved(EL, new_E[1]);

       new_E[0]->curve = (Curve*)0;
       new_E[2]->curve = (Curve*)0;
     } else {
       curve->face = 1;
       nomem_set_curved(EL, this);

       new_E[0]->curve =  (Curve*)0;
       new_E[1]->curve =  (Curve*)0;
       new_E[2]->curve =  (Curve*)0;
     }
   }//end if curve


    // replace element list with new list
    total_E = (Element**) calloc(EL->nel+3, sizeof(Element*));
    for(i=0;i<EL->nel;++i)
      total_E[i] = EL->flist[i];
    for(i=EL->nel;i<EL->nel+3;++i)
      total_E[i] = new_E[i-EL->nel];

    free(EL->flist);
    EL->flist = total_E;

    // setup link list
    for(i=EL->nel-1;i<EL->nel+2;++i)
      EL->flist[i]->next = EL->flist[i+1];
    EL->flist[i]->next = (Element*) 0;

    // number elements
    for(i=EL->nel;i<EL->nel+3;++i){
      E = EL->flist[i];
      E->id = i;
      for(j=0;j<E->Nverts;++j){
  E->edge[j].eid = i;
  E->edge[j].id  = j;
      }
    }

    for(i=0; i<Nedges; i++) {
      edge[i].base = NULL;
    } //end i

    for(i=0; i<3; i++) //for triangles
      for(int j=0; j < 3; j++)
  new_E[j]->edge[i].base = NULL;


    set_link(EL, id, 1, link_eid[(flag[3]+1)%Nverts], link_face[(flag[3]+1)%Nverts]);
    set_link(EL, id, 3, EL->nel+2, 1);
    set_link(EL, EL->nel, 2, EL->nel+2, 2);
    set_link(EL, EL->nel+1, 2, EL->nel+2, 0);
    EL->nel = EL->nel+3;

    if(link_eid[flag[1]] == -1){
      for(i=0;i<nfields;++i)
  for(Bc=Ubc[i];Bc;Bc=Bc->next)
    if(Bc->elmt->id == id && (Bc->face == flag[1])){
      Bc->face = 2;
      add_bc(EL, Ubc, nfields, Bc->id, EL->nel-3, 0);
      break;
    }
    }
    if(link_eid[flag[2]] == -1){
      for(i=0;i<nfields;++i)
  for(Bc=Ubc[i];Bc;Bc=Bc->next)
    if(Bc->elmt->id == id && (Bc->face == flag[2])){
      Bc->face = 1;
      Bc->elmt = new_E[0];
      add_bc(EL, Ubc, nfields,  Bc->id, EL->nel-2, 0);
      break;
    }
    }
    if(link_eid[flag[3]] == -1){
      for(i=0;i<nfields;++i)
  for(Bc=Ubc[i];Bc;Bc=Bc->next)
    if(Bc->elmt->id == id && (Bc->face == flag[3])){
      Bc->face = 0;
      add_bc(EL, Ubc, nfields,  Bc->id, EL->nel-2, 1);
      break;
    }
    }

    if(link_eid[(flag[3]+1)%Nverts] == -1){
      for(i=0;i<nfields;++i)
  for(Bc=Ubc[i];Bc;Bc=Bc->next)
    if(Bc->elmt->id == id && (Bc->face == (flag[3]+1)%Nverts)){
      Bc->face = 1;
      break;
    }
    }

    delete[] flag;
    flag = new int[7];
    flag[0] = 3;
    flag[1] = id;
    flag[2] = 3;
    flag[3] = EL->nel-3;
    flag[4] = 2;
    flag[5] = EL->nel-2;
    flag[6] = 2;

    break;

  case 4:
  default:

    if(flag[0]!=4) //if flag[0]=4, we want no side splitting
      for(i=0;i<Nverts;++i)
  if(link_eid[i] != -1)
    EL->flist[link_eid[i]]->split_edge(link_face[i], EL, Ubc, nfields, flag);

    Element** new_E = (Element**) calloc(3, sizeof(Element*));

    new_E[0] = (Element*) new Quad(this);
    new_E[1] = (Element*) new Quad(this);
    new_E[2] = (Element*) new Quad(this);

    // setup one element at a time
  // setup vertex positions:


    // store original
    origX.x = dvector(0, Nverts-1);
    origX.y = dvector(0, Nverts-1);

    for(i=0;i<Nverts;++i){
      origX.x[i] = vert[i].x;
      origX.y[i] = vert[i].y;
    }

    // store edge mid points
    centX.x = dvector(0, Nverts-1);
    centX.y = dvector(0, Nverts-1);

    calc_edge_centers(this, &centX);

    EcentX.x = dvector(0, 1);
    EcentX.y = dvector(0, 1);
    EcentX.x[0] = 0.0;  EcentX.y[0] = 0.0;
    for(i=0;i<Nverts;++i){
      EcentX.x[0] += vert[i].x;
      EcentX.y[0] += vert[i].y;
    }
    EcentX.x[0] /= (double)Nverts;
    EcentX.y[0] /= (double)Nverts;

    newX.x = dvector(0, Nverts-1);
    newX.y = dvector(0, Nverts-1);

    // Element 1
    newX.x[0] =  origX.x[0];  newX.y[0] =  origX.y[0];
    newX.x[1] =  centX.x[0];  newX.y[1] =  centX.y[0];
    newX.x[2] = EcentX.x[0];  newX.y[2] = EcentX.y[0];
    newX.x[3] =  centX.x[3];  newX.y[3] =  centX.y[3];

    move_vertices(&newX);

    // Element 2
    newX.x[0] =  origX.x[1];  newX.y[0] =  origX.y[1];
    newX.x[1] =  centX.x[1];  newX.y[1] =  centX.y[1];
    newX.x[2] = EcentX.x[0];  newX.y[2] = EcentX.y[0];
    newX.x[3] =  centX.x[0];  newX.y[3] =  centX.y[0];

    new_E[0]->move_vertices(&newX);

    // Element 3
    newX.x[0] =  origX.x[2];  newX.y[0] =  origX.y[2];
    newX.x[1] =  centX.x[2];  newX.y[1] =  centX.y[2];
    newX.x[2] = EcentX.x[0];  newX.y[2] = EcentX.y[0];
    newX.x[3] =  centX.x[1];  newX.y[3] =  centX.y[1];

    new_E[1]->move_vertices(&newX);

    // Element 4
    newX.x[0] =  origX.x[3];  newX.y[0] =  origX.y[3];
    newX.x[1] =  centX.x[3];  newX.y[1] =  centX.y[3];
    newX.x[2] = EcentX.x[0];  newX.y[2] = EcentX.y[0];
    newX.x[3] =  centX.x[2];  newX.y[3] =  centX.y[2];

    new_E[2]->move_vertices(&newX);

    if(curve && curve->face != -1){
      switch(curve->face){
      case 0:
  curve->face = 0;
  nomem_set_curved(EL, this);

  new_E[0]->curve = (Curve*) calloc(1, sizeof(Curve));
  new_E[0]->curve[0] = curve[0];
  new_E[0]->curve->face = 3;
  nomem_set_curved(EL, new_E[0]);

  new_E[1]->curve = (Curve*)0;
  new_E[1]->curvX = (Cmodes*)0;

  new_E[2]->curve = (Curve*)0;
  new_E[2]->curvX = (Cmodes*)0;
  break;
      case 1:

  new_E[0]->curve = curve;
  new_E[0]->curvX = curvX;
  new_E[0]->curve->face = 0;

  nomem_set_curved(EL, new_E[0]);

  curve = (Curve*)0;
  curvX = (Cmodes*)0;

  new_E[1]->curve = (Curve*) calloc(1, sizeof(Curve));
  new_E[1]->curve[0] = new_E[0]->curve[0];
  new_E[1]->curve->face = 3;
  nomem_set_curved(EL, new_E[1]);

  new_E[2]->curve = (Curve*)0;
  new_E[2]->curvX = (Cmodes*)0;
  break;
      case 2:

  new_E[0]->curve =  (Curve*)0;
  new_E[0]->curvX =  (Cmodes*)0;

  new_E[1]->curve = curve;
  new_E[1]->curvX = curvX;
  new_E[1]->curve->face = 0;
  nomem_set_curved(EL, new_E[1]);

  curve = (Curve*)0;
  curvX = (Cmodes*)0;

  new_E[2]->curve = (Curve*) calloc(1, sizeof(Curve));
  new_E[2]->curve[0] = new_E[2]->curve[0];
  new_E[2]->curve->face = 3;
  nomem_set_curved(EL, new_E[2]);

  break;
      case 3:
  curve->face = 3;
  nomem_set_curved(EL, this);

  new_E[0]->curve = (Curve*)0;
  new_E[0]->curvX = (Cmodes*)0;

  new_E[1]->curve = (Curve*)0;
  new_E[1]->curvX = (Cmodes*)0;

  new_E[2]->curve    = (Curve*) calloc(1, sizeof(Curve));
  new_E[2]->curve[0] = curve[0];
  new_E[2]->curve->face = 0;
  nomem_set_curved(EL, new_E[2]);

  break;
      }
    }


    // replace element list with new list
    total_E = (Element**) calloc(EL->nel+3, sizeof(Element*));
    for(i=0;i<EL->nel;++i)
      total_E[i] = EL->flist[i];
    for(i=EL->nel;i<EL->nel+3;++i)
      total_E[i] = new_E[i-EL->nel];

    free(EL->flist);
    EL->flist = total_E;

    // setup link list
    for(i=EL->nel-1;i<EL->nel+2;++i)
      EL->flist[i]->next = EL->flist[i+1];
    EL->flist[i]->next = (Element*) 0;

    // number elements
    for(i=EL->nel;i<EL->nel+3;++i){
      E = EL->flist[i];
      E->id = i;
      for(j=0;j<E->Nverts;++j){
  E->edge[j].eid = i;
  E->edge[j].id  = j;
      }
    }

    // need to set links
    // this is done in a specific order

    int cnt = 0;
    for(Bc=Ubc[0];Bc;Bc=Bc->next)
      ++cnt;

    if(link_eid[0] != -1){
      if(flag[0]!=4){
  set_link(EL, id, 0, link_eid[0], 1);
  set_link(EL, EL->nel, 3, nel, 1);
  nel += (EL->flist[link_eid[0]]->identify() == Nek_Tri) ? 1:2;
      }
    }
    else
      // check for bc on edge 0
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  un_link(EL, id, 0);
  if(Bc->elmt->id == id && Bc->face == 0){
    add_bc(EL, Ubc, nfields, k, EL->nel, 3);
    break;
  }
      }

    if(link_eid[1] != -1){
      if(flag[0]!=4){
  set_link(EL, EL->nel, 0, link_eid[1], 1);
  set_link(EL, EL->nel+1, 3, nel, 1);
  nel += (EL->flist[link_eid[1]]->identify() == Nek_Tri) ? 1:2;
      }
    }
    else
      // check for bc on edge 1
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == 1){
    Ubc[0][k].face = 0;
    Ubc[0][k].elmt = EL->flist[EL->nel];
    //  Ubc[1][k].face = 0;
    //  Ubc[1][k].elmt = EL->flist[EL->nel];

    un_link(EL, EL->nel, 0);

    add_bc(EL, Ubc, nfields, k, EL->nel+1, 3);
    break;
  }
      }


    if(link_eid[2] != -1){
      if(flag[0]!=4){
  set_link(EL, EL->nel+1, 0, link_eid[2], 1);
  set_link(EL, EL->nel+2, 3, nel, 1);
  nel += (EL->flist[link_eid[2]]->identify() == Nek_Tri) ? 1:2;
      }
    }
    else
      // check for bc on edge 2
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == 2){
    Ubc[0][k].face = 0;
    Ubc[0][k].elmt = EL->flist[EL->nel+1];
    //  Ubc[1][k].face = 0;
    //  Ubc[1][k].elmt = EL->flist[EL->nel+1];

    un_link(EL, EL->nel+1, 0);

    add_bc(EL, Ubc, nfields, k, EL->nel+2, 3);
    break;
  }
      }

    if(link_eid[3] != -1){
      if(flag[0]!=4){
  set_link(EL, EL->nel+2, 0, link_eid[3], 1);
  set_link(EL, id, 3, nel, 1);
  nel += (EL->flist[link_eid[3]]->identify() == Nek_Tri) ? 1:2;
      }
    }
    else
      // check for bc on edge 3
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == 3){
    Ubc[0][k].face = 0;
    Ubc[0][k].elmt = EL->flist[EL->nel+2];
    //  Ubc[1][k].face = 0;
    //  Ubc[1][k].elmt = EL->flist[EL->nel+2];

    un_link(EL, EL->nel+2, 0);

    add_bc(EL, Ubc, nfields, k, id, 3);
    break;
  }
      }

    if(flag[0]==4){
      for(int j=0; j < Nedges; j++)
  {
    edge[j].base = NULL;
    for(int k=0; k < 3; k++)
      new_E[k]->edge[j].base = NULL;
  }//end for j
    }//end if flag

    set_link(EL,        id, 1,   EL->nel, 2);
    set_link(EL,   EL->nel, 1, EL->nel+1, 2);
    set_link(EL, EL->nel+1, 1, EL->nel+2, 2);
    set_link(EL, EL->nel+2, 1,        id, 2);

    // need to fix edge numbering
    EL->nel += 3;

    if(flag[0]==4){
      delete []flag;
      flag = new int[9];
      flag[0] = 4;
      flag[1] = id;
      flag[2] = 2;
      flag[3] = EL->nel-3;
      flag[4] = 2;
      flag[5] = EL->nel-2;
      flag[6] = 2;
      flag[7] = EL->nel-1;
      flag[8] = 2;
    }//end if

    free(origX.x);  free(origX.y);
    free(newX.x);   free(newX.y);
    free(centX.x);  free(centX.y);
    free(EcentX.x);  free(EcentX.y);
  }//end switch statement
  return;
}




void Tet::split_element(Element_List *EL, Bndry **Ubc, int nfields, int *&flag){
  fprintf(stderr, "Hex::split_element not implemented yet\n");
  return;
}




void Pyr::split_element(Element_List *EL, Bndry **Ubc, int nfields, int *&flag){
  fprintf(stderr, "Hex::split_element not implemented yet\n");
  return;
}




void Prism::split_element(Element_List *EL, Bndry **Ubc, int nfields, int *&flag){
  fprintf(stderr, "Hex::split_element not implemented yet\n");
  return;
}




void Hex::split_element(Element_List *EL, Bndry **Ubc, int nfields, int *&flag){
  fprintf(stderr, "Hex::split_element not implemented yet\n");
  return;
}




void Element::split_element(Element_List *EL, Bndry **Ubc, int nfields, int *&flag){ERR;}




/*

Function name: Element::close_split

Function Purpose:

Argument 1: Element_List *EL
Purpose:

Argument 2: Bndry **Ubc
Purpose:

Argument 3: int nfields
Purpose:

Argument 4: int *&flag
Purpose:

Function Notes:

*/

void Tri::close_split(Element_List *EL, Bndry **Ubc, int nfields, int *&flag){
  Element *E;
  Bndry *Bc;
  int i, j, k, nel = EL->nel;
  Coord origX, newX, centX;
  Element **total_E;
  Element ** new_E;
  int cnt = 0;
  int which_split;
  int min_eorder = 0;
  int foo1,foo2,foo3;  //dummy ints used in edge ordering
  //  fprintf(stderr, "Tri::split_element not implemented yet\n");

  //This routine splits triangles into triangles ONLY.

  //           /|                            /|
  //          / |                   / |
  //         /  |                  / .|
  //        /   |                 / . |
  //       /    |                / .  |
  //      -------  ===========>         -------
  //     /|    /\              /| /\ /\
  //    / |   /  \            / |/_ /  \
  //   /  |  /    \          / .|  / .  \
  //  /   | /      \        / . | /   .  \
  // /----|/--------       /----|/--------

  //the flag array is used to designate which splitting operation
  //is to take place, if flag[0]=0, the triangle is split into
  // three, with side splitting.  if flag[0]=3 the triangle is
  //split into three with no side splitting.  if flag[0]=1 or
  //flag[0]=2, then the ints following flag[0] designate the
  //edge to be split.  Note, if flag[0]>0, no element adjacent
  //to the element is split.

  //Assumed: edge order given in flag array is counter clockwise (ie 123, 312, etc..)

  //It is assumed that the flag array is declared dynamically.  Once the information is used,
  //The array is deleted, and a new array is passed back in its place.  The new array contains
  //flag[0] = number elements with unconnected edges
  //flag[1] = First unconnected element's id
  //flag[2] = number of unconnected edges
  //flag[3] = Second unconnected element's id

  //  Element * Quad_element;
  int *link_eid  = ivector(0, Nedges-1);
  int *link_face = ivector(0, Nedges-1);

  min_eorder = edge[0].l;
  // store links
  for(i=0;i<Nedges;++i){
    min_eorder = (edge[i].l<min_eorder)? edge[i].l: min_eorder;
    if(edge[i].base)
      if(edge[i].link){
  link_eid[i] = edge[i].link->eid;
  link_face[i] = edge[i].link->id;
      }
      else{
  link_eid[i] = edge[i].base->eid;
  link_face[i] = edge[i].base->id;
      }
    else{
      link_eid[i] = -1;
      link_face[i] = -1;
    }
  }

  int dummy_flag=0;
  //case flag[0]=0 will be treated as the default.

  switch(flag[0]){
  case 1:
    foo1 = edge[flag[1]].l;
    foo2 = edge[(flag[1]+1)%Nverts].l;
    foo3 = edge[(flag[1]+2)%Nverts].l;

    split_edge(flag[1], EL, Ubc, nfields, &dummy_flag);
    edge[1].base = NULL;
    EL->flist[EL->nel-1]->edge[1].base=NULL;
    //set order
    EL->flist[EL->nel-1]->edge[0].l = foo3;
    EL->flist[EL->nel-1]->edge[1].l = foo1;
    EL->flist[EL->nel-1]->edge[2].l = min_eorder;
    edge[0].l = min_eorder;
    edge[1].l = foo1;
    edge[2].l = foo2;


    if(link_eid[flag[1]] == -1){
      for(Bc=Ubc[0];Bc;Bc=Bc->next)
  ++cnt;

      // check for bc on edge 0
      un_link(EL, id, 1);
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == flag[1]){
    Bc->face = 1;
    add_bc(EL, Ubc, nfields, k, EL->nel-1, 1);
    break;
  }//end if
      }//end for k
    }//end link_eid if

    delete[] flag;
    flag = new int[5];
    flag[0] = 2;
    flag[1] = id;
    flag[2] = 1;
    flag[3] = EL->nel-1;
    flag[4] = 1;
    break;

  case 2:

    if((flag[2]+1)%Nverts == flag[1])
      {
  int dummy = flag[1];
  flag[1]=flag[2];
  flag[2]=dummy;
      }//endif

    // store original
    origX.x = dvector(0, Nverts-1);
    origX.y = dvector(0, Nverts-1);

    for(i=0;i<Nverts;++i){
      origX.x[i] = vert[i].x;
      origX.y[i] = vert[i].y;
    }

    // store edge mid points

    centX.x = dvector(0, Nverts-1);
    centX.y = dvector(0, Nverts-1);

    calc_edge_centers(this, &centX);

    newX.x = dvector(0, Nverts-1);
    newX.y = dvector(0, Nverts-1);

    new_E = (Element**) calloc(2, sizeof(Element*));

    new_E[0] = (Element*) new Tri(this);
    new_E[1] = (Element*) new Tri(this);

    //------------------------------------------------------------------------
    //There are two cases : when the largest edge is flag[1] and
    //secondly is when the largest edge is flag[2]
    // This is a special addition made for Igor ----
    //------------------------------------------------------------------------

    which_split = 0;
    if( (origX.x[flag[2]]-origX.x[(flag[2]+1)%Nverts])*(origX.x[flag[2]]-origX.x[(flag[2]+1)%Nverts]) +
        (origX.y[flag[2]]-origX.y[(flag[2]+1)%Nverts])*(origX.y[flag[2]]-origX.y[(flag[2]+1)%Nverts]) >
        (origX.x[flag[1]]-origX.x[flag[2]])*(origX.x[flag[1]]-origX.x[flag[2]]) +
  (origX.y[flag[1]]-origX.y[flag[2]])*(origX.y[flag[1]]-origX.y[flag[2]]))
      which_split = 1;


    switch(which_split){
    case 1:
      foo1 = edge[flag[1]].l;
      foo2 = edge[flag[2]].l;
      foo3 = edge[(flag[2]+1)%Nverts].l;


      // Element 1 //Triangle element
      newX.x[0] = centX.x[flag[1]];     newX.y[0] = centX.y[flag[1]];
      newX.x[1] = origX.x[flag[2]];     newX.y[1] = origX.y[flag[2]];
      newX.x[2] = centX.x[flag[2]];     newX.y[2] = centX.y[flag[2]];

      move_vertices(&newX);


      // Element 2
      newX.x[0] = centX.x[flag[1]];             newX.y[0] = centX.y[flag[1]];
      newX.x[1] = centX.x[flag[2]];             newX.y[1] = centX.y[flag[2]];
      newX.x[2] = origX.x[flag[1]];             newX.y[2] = origX.y[flag[1]];


      new_E[0]->move_vertices(&newX);

      // Element 3

      newX.x[0] = centX.x[flag[2]];             newX.y[0] = centX.y[flag[2]];
      newX.x[1] = origX.x[(flag[2]+1)%Nverts];  newX.y[1] = origX.y[(flag[2]+1)%Nverts];
      newX.x[2] = origX.x[flag[1]];             newX.y[2] = origX.y[flag[1]];

      new_E[1]->move_vertices(&newX);

      //clean up the heap space allocation
      free(newX.x);
      free(newX.y);
      free(centX.x);
      free(centX.y);
      free(origX.x);
      free(origX.y);

      if(curve){
  if(curve->face == flag[1]){
    curve->face = 0;
    nomem_set_curved(EL, this);

    new_E[0]->curve = (Curve*) calloc(1, sizeof(Curve));
    new_E[0]->curve[0] = curve[0];
    new_E[0]->curve->face = 2;
    nomem_set_curved(EL, new_E[0]);
  }
  else if(curve->face == flag[2]){
    curve->face = 1;
    nomem_set_curved(EL, this);

    new_E[1]->curve = (Curve*) calloc(1, sizeof(Curve));
    new_E[1]->curve[0] = curve[0];
    new_E[1]->curve->face = 0;
    nomem_set_curved(EL, new_E[1]);
  }
  else {
    new_E[1]->curve = curve;
    curve = (Curve *)0;
    new_E[1]->curve->face = 1;
    nomem_set_curved(EL,new_E[1]);
  }
      }//end if curve

      // replace element list with new list
      total_E = (Element**) calloc(EL->nel+2, sizeof(Element*));
      for(i=0;i<EL->nel;++i)
  total_E[i] = EL->flist[i];
      for(i=EL->nel;i<EL->nel+2;++i)
  total_E[i] = new_E[i-EL->nel];

      free(EL->flist);
      EL->flist = total_E;

      // setup link list
      for(i=EL->nel-1;i<EL->nel+1;++i)
  EL->flist[i]->next = EL->flist[i+1];
      EL->flist[i]->next = (Element*) 0;

      // number elements
      for(i=EL->nel;i<EL->nel+2;++i){
  E = EL->flist[i];
  E->id = i;
  for(j=0;j<E->Nverts;++j){
    E->edge[j].eid = i;
    E->edge[j].id  = j;
  }
      }

      EL->nel = EL->nel+2;

      for(Bc=Ubc[0];Bc;Bc=Bc->next)
  ++cnt;

      if(link_eid[flag[1]] == -1){
  // check for bc on edge 0
  un_link(EL, id, flag[1]);
  for(k=0;k<cnt;++k){
    Bc = Ubc[0]+k;
    if(Bc->elmt->id == id && Bc->face == flag[1]){
      Bc->face = 0;
      add_bc(EL, Ubc, nfields, k, EL->nel-2, 2);
      break;
    }//end if
  }//end for k
      }//end link_eid if

      if(link_eid[flag[2]] == -1){
  // check for bc on edge 0
  un_link(EL, id, flag[2]);
  for(k=0;k<cnt;++k){
    Bc = Ubc[0]+k;
    if(Bc->elmt->id == id && Bc->face == flag[2]){
      Bc->face = 1;
      add_bc(EL, Ubc, nfields, k, EL->nel-1, 0);
      break;
    }//end if
  }//end for k
      }//end link_eid if

      if(link_eid[(flag[2]+1)%Nverts] == -1){
  un_link(EL, id, (flag[2]+1)%Nverts);
  for(k=0;k<cnt;++k){
    Bc = Ubc[0]+k;
    if(Bc->elmt->id == id && Bc->face == (flag[2]+1)%Nverts){
      Bc->elmt->id = EL->nel-1;
      Bc->face = 1;
      break;
    }//end if
  }//end for k
      }//end link_eid if


      for(j=0; j < Nedges; j++)
  {
    edge[j].base = NULL;
    for(k=0; k < 2; k++)
      new_E[k]->edge[j].base = NULL;
  }//end for j


      //set edge orders (on insides)

      EL->flist[EL->nel-2]->edge[0].l = min_eorder;
      EL->flist[EL->nel-2]->edge[1].l = min_eorder;
      EL->flist[EL->nel-2]->edge[2].l = foo1;

      EL->flist[EL->nel-1]->edge[0].l = foo2;
      EL->flist[EL->nel-1]->edge[1].l = foo3;
      EL->flist[EL->nel-1]->edge[2].l = min_eorder;

      edge[0].l = foo1;
      edge[1].l = foo2;
      edge[2].l = min_eorder;

      set_link(EL,   EL->nel-2, 0, id, 2);
      set_link(EL, EL->nel-2, 1, EL->nel-1, 2);
      set_link(EL, EL->nel-1, 1, link_eid[(flag[2]+1)%Nverts], link_face[(flag[2]+1)%Nverts]);
      break;
    default:

      foo1 = edge[flag[1]].l;
      foo2 = edge[flag[2]].l;
      foo3 = edge[(flag[2]+1)%Nverts].l;

      // Element 1 //Triangle element
      newX.x[0] = centX.x[flag[1]];     newX.y[0] = centX.y[flag[1]];
      newX.x[1] = origX.x[flag[2]];     newX.y[1] = origX.y[flag[2]];
      newX.x[2] = centX.x[flag[2]];     newX.y[2] = centX.y[flag[2]];

      move_vertices(&newX);


      // Element 2
      newX.x[0] = centX.x[flag[1]];             newX.y[0] = centX.y[flag[1]];
      newX.x[1] = centX.x[flag[2]];             newX.y[1] = centX.y[flag[2]];
      newX.x[2] = origX.x[(flag[2]+1)%Nverts];  newX.y[2] = origX.y[(flag[2]+1)%Nverts];


      new_E[0]->move_vertices(&newX);

      // Element 3

      newX.x[0] = centX.x[flag[1]];             newX.y[0] = centX.y[flag[1]];
      newX.x[1] = origX.x[(flag[2]+1)%Nverts];  newX.y[1] = origX.y[(flag[2]+1)%Nverts];
      newX.x[2] = origX.x[flag[1]];             newX.y[2] = origX.y[flag[1]];

      new_E[1]->move_vertices(&newX);

      //clean up the heap space allocation
      free(newX.x);
      free(newX.y);
      free(centX.x);
      free(centX.y);
      free(origX.x);
      free(origX.y);

      if(curve){
  if(curve->face == flag[1]){
    curve->face = 0;
    nomem_set_curved(EL, this);

    new_E[1]->curve = (Curve*) calloc(1, sizeof(Curve));
    new_E[1]->curve[0] = curve[0];
    new_E[1]->curve->face = 2;
    nomem_set_curved(EL, new_E[1]);
  }
  else if(curve->face == flag[2]){
    curve->face = 1;
    nomem_set_curved(EL, this);

    new_E[0]->curve = (Curve*) calloc(1, sizeof(Curve));
    new_E[0]->curve[0] = curve[0];
    new_E[0]->curve->face = 1;
    nomem_set_curved(EL, new_E[0]);
  }
  else {
    new_E[1]->curve = curve;
    curve = (Curve *)0;
    new_E[1]->curve->face = 1;
    nomem_set_curved(EL,new_E[1]);
  }
      }//end if curve

      // replace element list with new list
      total_E = (Element**) calloc(EL->nel+2, sizeof(Element*));
      for(i=0;i<EL->nel;++i)
  total_E[i] = EL->flist[i];
      for(i=EL->nel;i<EL->nel+2;++i)
  total_E[i] = new_E[i-EL->nel];

      free(EL->flist);
      EL->flist = total_E;

      // setup link list
      for(i=EL->nel-1;i<EL->nel+1;++i)
  EL->flist[i]->next = EL->flist[i+1];
      EL->flist[i]->next = (Element*) 0;

      // number elements
      for(i=EL->nel;i<EL->nel+2;++i){
  E = EL->flist[i];
  E->id = i;
  for(j=0;j<E->Nverts;++j){
    E->edge[j].eid = i;
    E->edge[j].id  = j;
  }
      }

      EL->nel = EL->nel+2;

      for(Bc=Ubc[0];Bc;Bc=Bc->next)
  ++cnt;

      if(link_eid[flag[1]] == -1){
  // check for bc on edge 0
  un_link(EL, id, flag[1]);
  for(k=0;k<cnt;++k){
    Bc = Ubc[0]+k;
    if(Bc->elmt->id == id && Bc->face == flag[1]){
      Bc->face = 0;
      add_bc(EL, Ubc, nfields, k, EL->nel-1, 2);
      break;
    }//end if
  }//end for k
      }//end link_eid if

      if(link_eid[flag[2]] == -1){
  // check for bc on edge 0
  un_link(EL, id, flag[2]);
  for(k=0;k<cnt;++k){
    Bc = Ubc[0]+k;
    if(Bc->elmt->id == id && Bc->face == flag[2]){
      Bc->face = 1;
      add_bc(EL, Ubc, nfields, k, EL->nel-2, 1);
      break;
    }//end if
  }//end for k
      }//end link_eid if

      if(link_eid[(flag[2]+1)%Nverts] == -1){
  un_link(EL, id, (flag[2]+1)%Nverts);
  for(k=0;k<cnt;++k){
    Bc = Ubc[0]+k;
    if(Bc->elmt->id == id && Bc->face == (flag[2]+1)%Nverts){
      Bc->elmt->id = EL->nel-1;
      Bc->face = 2;
      break;
    }//end if
  }//end for k
      }//end link_eid if

      for(j=0; j < Nedges; j++)
  {
    edge[j].base = NULL;
    for(k=0; k < 2; k++)
      new_E[k]->edge[j].base = NULL;
  }//end for j

      //set edge orders (on insides)

      EL->flist[EL->nel-2]->edge[0].l = min_eorder;
      EL->flist[EL->nel-2]->edge[1].l = foo2;
      EL->flist[EL->nel-2]->edge[2].l = min_eorder;


      EL->flist[EL->nel-1]->edge[0].l = min_eorder;
      EL->flist[EL->nel-1]->edge[1].l = foo3;
      EL->flist[EL->nel-1]->edge[2].l = foo1;

      edge[0].l = foo1;
      edge[1].l = foo2;
      edge[2].l = min_eorder;

      set_link(EL,   EL->nel-2, 0, id, 2);
      set_link(EL, EL->nel-2, 2, EL->nel-1, 0);
      set_link(EL, EL->nel-1, 1, link_eid[(flag[2]+1)%Nverts], link_face[(flag[2]+1)%Nverts]);
    }//end switch(which_split)

    delete[] flag;
    flag = new int[7];
    flag[0] = 3;
    flag[1] = id;
    flag[2] = 2;
    flag[3] = EL->nel-2;
    flag[4] = 1;
    flag[5] = EL->nel-1;
    flag[6] = 2;

    break;

  case 3:
  default:

    if(flag[0]!=3){ //if flag[0]=3, we want no side splitting
      for(i=0;i<Nverts;++i)
  if(link_eid[i] != -1){
    foo1 = EL->flist[link_eid[i]]->edge[link_face[i]].l;
    foo2 = EL->flist[link_eid[i]]->edge[(link_face[i]+1)%EL->flist[link_eid[i]]->Nedges].l;
    foo3 = EL->flist[link_eid[i]]->edge[(link_face[i]+2)%EL->flist[link_eid[i]]->Nedges].l;
    EL->flist[link_eid[i]]->split_edge(link_face[i], EL, Ubc, nfields, &dummy_flag);

    //set order
    EL->flist[EL->nel-1]->edge[0].l = foo3;
    EL->flist[EL->nel-1]->edge[1].l = foo1;
    foo3 = (foo1 < foo3)?foo1:foo3;
    foo3 = (foo2 < foo3)?foo2:foo3;
    EL->flist[EL->nel-1]->edge[2].l = foo3;   //foo3 is used to hold the minimum
    EL->flist[link_eid[i]]->edge[0].l = foo3;
    EL->flist[link_eid[i]]->edge[1].l = foo1;
    EL->flist[link_eid[i]]->edge[2].l = foo2;
  }

    }
    foo1 = edge[0].l;
    foo2 = edge[1].l;
    foo3 = edge[2].l;

    new_E = (Element**) calloc(3, sizeof(Element*));

    new_E[0] = (Element*) new Tri(this);
    new_E[1] = (Element*) new Tri(this);
    new_E[2] = (Element*) new Tri(this);

    // setup one element at a time
    // setup vertex positions:

    // store original
    origX.x = dvector(0, Nverts-1);
    origX.y = dvector(0, Nverts-1);

    for(i=0;i<Nverts;++i){
      origX.x[i] = vert[i].x;
      origX.y[i] = vert[i].y;
    }

    // store edge mid points

    centX.x = dvector(0, Nverts-1);
    centX.y = dvector(0, Nverts-1);

    calc_edge_centers(this, &centX);

    newX.x = dvector(0, Nverts-1);
    newX.y = dvector(0, Nverts-1);

    // Element 1
    newX.x[0] = origX.x[0];  newX.y[0] = origX.y[0];
    newX.x[1] = centX.x[0];  newX.y[1] = centX.y[0];
    newX.x[2] = centX.x[2];  newX.y[2] = centX.y[2];

    move_vertices(&newX);

    // Element 2
    newX.x[0] = origX.x[1];  newX.y[0] = origX.y[1];
    newX.x[1] = centX.x[1];  newX.y[1] = centX.y[1];
    newX.x[2] = centX.x[0];  newX.y[2] = centX.y[0];

    new_E[0]->move_vertices(&newX);

    // Element 3
    newX.x[0] = origX.x[2];  newX.y[0] = origX.y[2];
    newX.x[1] = centX.x[2];  newX.y[1] = centX.y[2];
    newX.x[2] = centX.x[1];  newX.y[2] = centX.y[1];

    new_E[1]->move_vertices(&newX);

    // Element 4
    newX.x[0] = centX.x[0];  newX.y[0] = centX.y[0];
    newX.x[1] = centX.x[1];  newX.y[1] = centX.y[1];
    newX.x[2] = centX.x[2];  newX.y[2] = centX.y[2];

    new_E[2]->move_vertices(&newX);

    if(curve){
      switch(curve->face){
      case 0:
  curve->face = 0;
  nomem_set_curved(EL, this);

  new_E[0]->curve = (Curve*) calloc(1, sizeof(Curve));
  new_E[0]->curve[0] = curve[0];
  new_E[0]->curve->face = 2;
  nomem_set_curved(EL, new_E[0]);

  new_E[1]->curve = (Curve*)0;
  new_E[1]->curvX = (Cmodes*)0;

  new_E[2]->curve = (Curve*)0;
  new_E[2]->curvX = (Cmodes*)0;
  break;
      case 1:
  curve = (Curve*)0;
  curvX = (Cmodes*)0;

  new_E[0]->curve->face = 0;
  nomem_set_curved(EL, new_E[0]);

  new_E[1]->curve = (Curve*) calloc(1, sizeof(Curve));
  new_E[1]->curve[0] = new_E[0]->curve[0];
  new_E[1]->curve->face = 2;
  nomem_set_curved(EL, new_E[1]);

  new_E[2]->curve = (Curve*)0;
  new_E[2]->curvX = (Cmodes*)0;
      break;
      case 2:
  curve->face = 2;
  nomem_set_curved(EL, this);

  new_E[0]->curve = (Curve*)0;
  new_E[0]->curvX = (Cmodes*)0;

  new_E[1]->curve    = (Curve*) calloc(1, sizeof(Curve));
  new_E[1]->curve[0] = curve[0];
  new_E[1]->curve->face = 0;
  nomem_set_curved(EL, new_E[1]);

  new_E[2]->curve = (Curve*)0;
  new_E[2]->curvX = (Cmodes*)0;
  break;
      }
    }

    // replace element list with new list
    total_E = (Element**) calloc(EL->nel+3, sizeof(Element*));
    for(i=0;i<EL->nel;++i)
      total_E[i] = EL->flist[i];
    for(i=EL->nel;i<EL->nel+3;++i)
      total_E[i] = new_E[i-EL->nel];

    free(EL->flist);
    EL->flist = total_E;

    // setup link list
    for(i=EL->nel-1;i<EL->nel+2;++i)
      EL->flist[i]->next = EL->flist[i+1];
    EL->flist[i]->next = (Element*) 0;

    // number elements
    for(i=EL->nel;i<EL->nel+3;++i){
      E = EL->flist[i];
      E->id = i;
      for(j=0;j<E->Nverts;++j){
  E->edge[j].eid = i;
  E->edge[j].id  = j;
      }
    }

    // need to set links
    // this is done in a specific order

    for(Bc=Ubc[0];Bc;Bc=Bc->next)
      ++cnt;

    if(link_eid[0] != -1){
      if(flag[0]!=3){
  set_link(EL, id, 0, link_eid[0], 1);
  set_link(EL, EL->nel, 2, nel, 1);
  nel += (EL->flist[link_eid[0]]->identify() == Nek_Tri) ? 1:2;
      }//endif
    }
    else{
      // check for bc on edge 0
      un_link(EL, id, 0);
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == 0){
    add_bc(EL, Ubc, nfields, k, EL->nel, 2);
    break;
  }
      }//end for k
    }//end else

    if(link_eid[1] != -1){
      if(flag[0]!=3){
  set_link(EL, EL->nel, 0, link_eid[1], 1);
  set_link(EL, EL->nel+1, 2, nel, 1);
  nel += (EL->flist[link_eid[1]]->identify() == Nek_Tri) ? 1:2;
      }//endif
    }
    else{
      // check for bc on edge 1
      un_link(EL, EL->nel, 0);
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == 1){
    Ubc[0][k].face = 0;
    Ubc[0][k].elmt = EL->flist[EL->nel];
    add_bc(EL, Ubc, nfields, k, EL->nel+1, 2);
    break;
  }
      }
    }

    if(link_eid[2] != -1){
      if(flag[0]!=3){
  set_link(EL, EL->nel+1, 0, link_eid[2], 1);
  set_link(EL, id, 2, nel, 1);
  nel += (EL->flist[link_eid[2]]->identify() == Nek_Tri) ? 1:2;
      }
    }
    else {
      // check for bc on edge 2
      un_link(EL, EL->nel+1, 0);
      for(k=0;k<cnt;++k){
  Bc = Ubc[0]+k;
  if(Bc->elmt->id == id && Bc->face == 2){
    Ubc[0][k].face = 0;
    Ubc[0][k].elmt = EL->flist[EL->nel+1];
    add_bc(EL, Ubc, nfields, k, id, 2);
    break;
    }
      }
    }//end else


    if(flag[0]==3){
      for(j=0; j < Nedges; j++)
  {
    edge[j].base = NULL;
    for(k=0; k < 3; k++)
      new_E[k]->edge[j].base = NULL;
  }//end for j
    }//end if


    for(i=0; i<Nverts; i++)
      EL->flist[EL->nel+2]->edge[i].l = min_eorder;

    EL->flist[EL->nel]->edge[0].l = foo2;
    EL->flist[EL->nel]->edge[1].l = min_eorder;
    EL->flist[EL->nel]->edge[2].l = foo1;

    EL->flist[EL->nel+1]->edge[0].l = foo3;
    EL->flist[EL->nel+1]->edge[1].l = min_eorder;
    EL->flist[EL->nel+1]->edge[2].l = foo2;

    edge[0].l = foo1;
    edge[1].l = min_eorder;
    edge[2].l = foo3;

    set_link(EL,   EL->nel, 1, EL->nel+2, 0);
    set_link(EL, EL->nel+1, 1, EL->nel+2, 1);
    set_link(EL,        id, 1, EL->nel+2, 2);

    // need to fix edge numbering
    EL->nel += 3;

    free(origX.x);  free(origX.y);
    free(newX.x);   free(newX.y);
    free(centX.x);  free(centX.y);
    if(flag[0]!=0)
      {
  delete[] flag;
  flag = new int[7];
  flag[0] = 3;
  flag[1] = id;
  flag[2] = 2;
  flag[3] = EL->nel-3;
  flag[4] = 2;
  flag[5] = EL->nel-2;
  flag[6] = 2;
      }//end if
  }//end of switch statement

  return;
}//end of Closed_split member function



void Quad::close_split(Element_List *, Bndry **, int , int *&){
}




void Tet::close_split(Element_List *EL, Bndry **Ubc, int nfields, int *&flag){
  fprintf(stderr, "Hex::split_element not implemented yet\n");
  return;
}




void Pyr::close_split(Element_List *EL, Bndry **Ubc, int nfields, int *&flag){
  fprintf(stderr, "Hex::split_element not implemented yet\n");
  return;
}




void Prism::close_split(Element_List *EL, Bndry **Ubc, int nfields, int *&flag){
  fprintf(stderr, "Hex::split_element not implemented yet\n");
  return;
}




void Hex::close_split(Element_List *EL, Bndry **Ubc, int nfields, int *&flag){
  fprintf(stderr, "Hex::split_element not implemented yet\n");
  return;
}




void Element::close_split  (Element_List *EL, Bndry **Ubc, int nfields, int *&flag){ERR;}



static void un_link(Element_List *U, int eid1, int id1){
  Element *E=U->flist[eid1];

  /* set links */

  E->edge[id1].base = (Edge*)0;
  E->edge[id1].link = (Edge*)0;
}


static void add_bc(Element_List *EL, Bndry **Ubc, int nfields,
       int bid, int eid, int face){
  int i,n;
  Bndry *Bc;
  int cnt = 0;
  for(Bc=Ubc[0];Bc;Bc=Bc->next)
    ++cnt;
  //  fprintf(stderr, "add_bc: %d bcs\n", cnt);
  int Je = iparam("INTYPE"); Je=(Je)?Je:1;
  for(n=0;n<nfields;++n){
    Bc = (Bndry*) calloc(cnt+1, sizeof(Bndry));
    memcpy(Bc, Ubc[n], cnt*sizeof(Bndry));
    memcpy(Bc+cnt, Ubc[n]+bid, sizeof(Bndry));
    free(Ubc[n]);
    Ubc[n] = Bc;
    Ubc[n][cnt].id = cnt;
    for(i=0;i<cnt;++i)
      Ubc[n][i].next = Ubc[n]+i+1;
    Ubc[n][cnt].next = (Bndry*)0;
    Ubc[n][cnt].elmt = EL->flist[eid];
    Ubc[n][cnt].face = face;
    EL->flist[eid]->MemBndry(Ubc[n]+cnt, face, Je);
    un_link(EL, eid, face);
  }
  cnt = 0;
  for(Bc=Ubc[0];Bc;Bc=Bc->next)
    ++cnt;
  //  fprintf(stderr, "add_bc(2): %d bcs\n", cnt);
}



static void set_link(Element_List *U, int eid1, int id1, int eid2, int id2){

  if(eid1 == -1 || eid2 == -1){
    if(eid1 != -1){
      U->flist[eid1]->edge[id1].base = (Edge*)NULL;
      U->flist[eid1]->edge[id1].link = (Edge*)NULL;
    }
    else if(eid2 != -1){
      U->flist[eid2]->edge[id2].base = (Edge*)NULL;
      U->flist[eid2]->edge[id2].link = (Edge*)NULL;
    }
    fprintf(stderr," set_link problem\n");

    return;
  }

  // 2d
  int con_modal[4][4]= {{1,1,0,0},{1,1,0,0},{0,0,1,1},{0,0,1,1}};

  /* set up connectivity inverses if required */
  /* like faces meet */
  Element *E=U->flist[eid1],*F=U->flist[eid2];

  if(eid1>eid2)
    E->edge[id1].con = con_modal[id1][id2];
  else
    F->edge[id2].con = con_modal[id1][id2];

  /* set links */

  E->edge[id1].base = E->edge + id1;
  E->edge[id1].link = F->edge + id2;
  F->edge[id2].base = E->edge + id1;
  F->edge[id2].link = (Edge*)NULL;

  // fprintf(stderr, "set_link (Tri) connecting elmt: %d edge: %d to elmt: %d edge: %d\n", eid1+1, id1+1, eid2+1, id2+1);


}

void get_links(Element *E, int *link_eid, int *link_face){
  int i;
  for(i=0;i<E->Nedges;++i){
    if(E->edge[i].base)
      if(E->edge[i].link){
  link_eid[i]  = E->edge[i].link->eid;
  link_face[i] = E->edge[i].link->id;
      }
      else{
  link_eid[i]  = E->edge[i].base->eid;
  link_face[i] = E->edge[i].base->id;
      }
    else{
      link_eid[i] = -1;
      link_face[i] = -1;
    }
  }
}


void tri_move_vertex(Element_List *EL, int eid, int vn, double x, double y){
  Element *Estart = EL->flist[eid];
  Element *E      = Estart;
  int     newvn;

  Edge *newedg;

  newvn = vn;

  // works for triangles only
  do{
    E->vert[newvn].x = x;
    E->vert[newvn].y = y;
    newedg = E->edge+(newvn+E->Nverts-1)%E->Nverts;
    if(newedg->base){
      if(newedg->link){
  E = EL->flist[newedg->link->eid];
  newvn = newedg->link->id;
      }
      else{
  E = EL->flist[newedg->base->eid];
  newvn = newedg->base->id;
      }
    }
  } while(E != Estart);

}



/*

Function name: Element::delete_element

Function Purpose:

Argument 1: Element_List *EL
Purpose:

Argument 2: Bndry **Ubc
Purpose:

Argument 3: int nfields
Purpose:

Argument 4: int *flag
Purpose:

Function Notes:

*/

void Tri::delete_element(Element_List *EL, Bndry **Ubc, int nfields, int *flag){
  Element *E;

  int i, j, k, eid, fac;

  int *link_eid  = ivector(0, Nedges-1);
  int *link_face = ivector(0, Nedges-1);

  int *linka_eid  = ivector(0, Nedges-1);
  int *linka_face = ivector(0, Nedges-1);

  Coord EcentX;

  EcentX.x = dvector(0, 1);
  EcentX.y = dvector(0, 1);
  EcentX.x[0] = 0.0;  EcentX.y[0] = 0.0;
  for(i=0;i<Nverts;++i){
    EcentX.x[0] += vert[i].x;
    EcentX.y[0] += vert[i].y;
  }
  EcentX.x[0] /= (double)Nverts;
  EcentX.y[0] /= (double)Nverts;

  // store links
  get_links(this, link_eid, link_face);

  for(i=0;i<Nedges;++i)
    if(link_eid[i] == -1)
      return;

  // use edge 0 for now

  eid = link_eid[0];
  fac = link_face[0];
  get_links(EL->flist[eid], linka_eid, linka_face);
  if(EL->flist[eid]->identify() != Nek_Tri)
    return;

  for(j=0;j<EL->flist[eid]->Nedges;++j)
    if(linka_eid[j] == -1)
      return;

  if(link_eid[0] != -1){
    eid = link_eid[0];
    fac = link_face[0];
    get_links(EL->flist[eid], linka_eid, linka_face);
    fprintf(stderr, "nel: %d elmta: %d facea: %delmtb: %d faceb: %d\n",
      EL->nel, linka_eid [(fac+1)%Nedges]+1,
      linka_face[(fac+1)%Nedges]+1,
      linka_eid [(fac+2)%Nedges]+1,
      linka_face[(fac+2)%Nedges]+1);

    set_link(EL,  linka_eid [(fac+1)%Nedges],
       linka_face[(fac+1)%Nedges],
       linka_eid [(fac+2)%Nedges],
       linka_face[(fac+2)%Nedges]);

    set_link(EL,  link_eid [1],
       link_face[1],
       link_eid [2],
       link_face[2]);


  }
  fprintf(stderr, "Deleting elements: %d %d \n", id+1,
    link_eid[0]+1);


  int cnt = 0;// fix
  Element **new_flist = (Element**) calloc(EL->nel, sizeof(Element*));

  for(cnt= 0, k=0;k<EL->nel;++k){
    if(link_eid[0] != k && k != id){
      new_flist[cnt] = EL->flist[k];
      ++cnt;
    }
  }

  for(i=0;i<cnt;++i){
    E = new_flist[i];
    E->id = i;
    for(j=0;j<E->Nedges;++j)
      E->edge[j].eid = i;
  }

  for(i=0;i<cnt-1;++i)
    new_flist[i]->next = new_flist[i+1];
  new_flist[i]->next = (Element*)0;

  free(EL->flist);
  EL->fhead = new_flist[0];
  EL->flist = new_flist;
  EL->nel = cnt;
  // should do some freeing here..
}




void Quad::delete_element(Element_List *, Bndry **, int , int *){
  /* reference values to turn off compiler warnings */


  fprintf(stderr, "Quad::delete_element not implemented yet\n");
}




void Tet::delete_element(Element_List *EL, Bndry **Ubc, int nfields, int *flag){

  fprintf(stderr, "Quad::delete_element not implemented yet\n");
}




void Pyr::delete_element(Element_List *EL, Bndry **Ubc, int nfields, int *flag){

  fprintf(stderr, "Quad::delete_element not implemented yet\n");
}




void Prism::delete_element(Element_List *EL, Bndry **Ubc, int nfields, int *flag){

  fprintf(stderr, "Quad::delete_element not implemented yet\n");
}




void Hex::delete_element(Element_List *EL, Bndry **Ubc, int nfields, int *flag){

  fprintf(stderr, "Quad::delete_element not implemented yet\n");
}




void Element::delete_element(Element_List *EL, Bndry **Ubc, int nfields, int *flag){ERR;}
