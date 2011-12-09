
#include "nektar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <veclib.h>

void partition_BOX_min_max(Element_List *EL, int *partition, double *XYZ_min_max);
void partition_radius_center(double *XYZ_min_max, double *R_center);
int *get_OuterElmntFaceList(Element_List *EL, int *partition, int *Nbndry_faces);
void WriteTEC_boundary_faces(int Nbndry_faces, int *OuterElmntFaceList, Element_List *EL);
int *get_AdjacentPartitions(Element_List *EL, int *partition, int Nbndry_faces, int *OuterElmntFaceList, double *R_center,int *NAdjPartitionID);
int IsPartInList(int part, int NAdjPartitionID, int *ListAdjacentPartitions);


int *test_partition_connectivity(Element_List *EL, int *partition, int *Npartitions ){

  int Nbndry_faces,NAdjPartitionID;
  double *XYZ_min_max,*R_center;
  int *OuterElmntFaceList,*AdjacentPartitions;

  XYZ_min_max = new double[6];
  R_center = new double[4];

  partition_BOX_min_max(EL, partition, XYZ_min_max);
  partition_radius_center(XYZ_min_max,R_center);

/*
  fprintf(stdout," partition %d BOX=[%f %f   %f %f   %f %f] \n",mynode(),XYZ_min_max[0],XYZ_min_max[1],
                                                                         XYZ_min_max[2],XYZ_min_max[3],
                                                                         XYZ_min_max[4],XYZ_min_max[5]);
  fprintf(stdout," partition %d R_center = [%f   %f %f %f]\n",mynode(),R_center[0],R_center[1],R_center[2],R_center[3]);
*/

  OuterElmntFaceList = get_OuterElmntFaceList(EL, partition, &Nbndry_faces);
//  WriteTEC_boundary_faces(Nbndry_faces, OuterElmntFaceList, EL);
  AdjacentPartitions = get_AdjacentPartitions(EL, partition, Nbndry_faces, OuterElmntFaceList, R_center, &NAdjPartitionID);

/*
  fprintf(stdout," partition %d NAdjPartitionID = %d\n",mynode(),NAdjPartitionID);
  for (int i = 0;  i < NAdjPartitionID; ++i)
     fprintf(stdout," Partition %d AdjacentPartitions[%d] = %d\n",mynode(),i,AdjacentPartitions[i]);
*/

  delete[] XYZ_min_max;
  delete[] R_center;
  delete[] OuterElmntFaceList;

  Npartitions[0] = NAdjPartitionID;
  return AdjacentPartitions;
}



void partition_BOX_min_max(Element_List *EL, int *partition, double *XYZ_min_max){

/* search for the [Xmin  Xmax Ymin Ymax Zmin Zmax] in local partition */

  int my_rank, nel, i, j;
  double x,y,z;
  Element *E;

  if (EL->fhead->dim()==2)
    error_msg(partition_BOX_min_max:: not implemented in 2D);

  nel = EL->nel;
  my_rank = mynode();


/*  set box dimensions to be coordinates of the first vertex in partition */
  for(i = 0; i < nel; ++i){
    if (partition[i] != my_rank) continue;
    E = EL->flist[i];
    XYZ_min_max[0] = E->vert[0].x;
    XYZ_min_max[1] = E->vert[0].x;
    XYZ_min_max[2] = E->vert[0].y;
    XYZ_min_max[3] = E->vert[0].y;
    XYZ_min_max[4] = E->vert[0].z;
    XYZ_min_max[5] = E->vert[0].z;
    break;
  }

  for(i = 0; i < nel; ++i){
    if (partition[i] != my_rank) continue;
    E = EL->flist[i];
    for(j = 0; j < E->Nverts; ++j){
      x = E->vert[j].x;
      y = E->vert[j].y;
      z = E->vert[j].z;
      if ( x < XYZ_min_max[0])
        XYZ_min_max[0] = x;
      if ( x > XYZ_min_max[1])
        XYZ_min_max[1] = x;
      if ( y < XYZ_min_max[2])
        XYZ_min_max[2] = y;
      if ( y > XYZ_min_max[3])
        XYZ_min_max[3] = y;
      if ( z < XYZ_min_max[4])
        XYZ_min_max[4] = z;
      if ( z > XYZ_min_max[5])
        XYZ_min_max[5] = z;
    }
  }
}


void partition_radius_center(double *XYZ_min_max, double *R_center){

  double dx,dy,dz;

  /* given a box find the radius of a sphere encapsulating the box. */

  /* [Xmax Ymax Zmax] [Xmin Ymin Zmin]  */

   dx = XYZ_min_max[1]-XYZ_min_max[0];
   dy = XYZ_min_max[3]-XYZ_min_max[2];
   dz = XYZ_min_max[5]-XYZ_min_max[4];
   R_center[0] = 0.5*sqrt(dx*dx+dy*dy+dz*dz);

   R_center[1] = 0.5*(XYZ_min_max[1]+XYZ_min_max[0]);
   R_center[2] = 0.5*(XYZ_min_max[3]+XYZ_min_max[2]);
   R_center[3] = 0.5*(XYZ_min_max[5]+XYZ_min_max[4]);
}

int *get_OuterElmntFaceList(Element_List *EL, int *partition, int *Nbndry_faces_output){

  /* clreate list of elements and faces facing partitions boundary */

  if (EL->fhead->dim()==2)
    error_msg(partition_BOX_min_max:: not implemented in 2D);

  int my_rank, nel, i, j, Nbndry_faces, BndryFace, Linked_Element_ID;
  int *OuterElmntFaceList_temp1,*OuterElmntFaceList_temp2,*OuterElmntFaceList;
  Element *E;

  nel = EL->nel;
  my_rank = mynode();

  OuterElmntFaceList_temp1 = NULL;
  OuterElmntFaceList_temp2 = NULL;

  Nbndry_faces = 0;
  for(i = 0; i < nel; ++i){
    if (partition[i] != my_rank) continue;
    E = EL->flist[i];
    for (j = 0; j < E->Nfaces; ++j){
  BndryFace = 0;
  if (E->face[j].link == 0) // face belongs to partitions boundary
          BndryFace = 1;
  else{ // check if the face is linked to element from the same or from different partition
     Linked_Element_ID = E->face[j].link->eid;
          if  (partition[Linked_Element_ID] != my_rank) //element in different partition
            BndryFace = 1;
  }
  if (BndryFace == 1){
          Nbndry_faces++;
    if (Nbndry_faces == 1){
             OuterElmntFaceList_temp1 = new int[2];
             OuterElmntFaceList_temp1[0] = i;
             OuterElmntFaceList_temp1[1] = j;
    }
    else{
             OuterElmntFaceList_temp2 = new int[(Nbndry_faces-1)*2];
             memcpy(OuterElmntFaceList_temp2,OuterElmntFaceList_temp1,(Nbndry_faces-1)*2*sizeof(int));
             delete[] OuterElmntFaceList_temp1;
             OuterElmntFaceList_temp1 = new int[Nbndry_faces*2];
             memcpy(OuterElmntFaceList_temp1,OuterElmntFaceList_temp2,(Nbndry_faces-1)*2*sizeof(int));
             OuterElmntFaceList_temp1[(Nbndry_faces-1)*2]   = i;
             OuterElmntFaceList_temp1[(Nbndry_faces-1)*2+1] = j;
       delete[] OuterElmntFaceList_temp2;
    }
  }
    }
    OuterElmntFaceList = new int[Nbndry_faces*2];
    memcpy(OuterElmntFaceList,OuterElmntFaceList_temp1,Nbndry_faces*2*sizeof(int));
  }
  if (OuterElmntFaceList_temp1 != NULL)
    delete[] OuterElmntFaceList_temp1;
  else
    error_msg(get_OuterElmntFaceList:: no BndryFace found);

  Nbndry_faces_output[0] = Nbndry_faces;
  return OuterElmntFaceList;
}


void WriteTEC_boundary_faces(int Nbndry_faces, int *OuterElmntFaceList, Element_List *EL){

  int i,k,vn,face;
  FILE *pFile;
  char fname[128];
  Element *E;

  sprintf(fname,"OuterFace_%d.dat",mynode());
  pFile = fopen(fname,"w");

  for (i = 0; i < Nbndry_faces; ++i){
    fprintf(pFile,"zone n=%d, e=%d, DATAPACKING=POINT, ZONETYPE=FETRIANGLE\n",3,1);
    E = EL->flist[OuterElmntFaceList[i*2]];
    face = OuterElmntFaceList[i*2+1];
    for (k = 0; k < E->Nfverts(face); ++k){
       vn = E->vnum(face,k);
       fprintf(pFile," %f %f %f %d \n",E->vert[vn].x,E->vert[vn].y,E->vert[vn].z,mynode());
    }
    fprintf(pFile," %d %d %d\n", 1, 2, 3);
  }
  fclose (pFile);
}


int * get_AdjacentPartitions(Element_List *EL, int *partition, int Nbndry_faces, int *OuterElmntFaceList, double *R_center, int *NAdjPartitionID_output){

 if (EL->fhead->dim()==2)
          error_msg(partition_BOX_min_max:: not implemented in 2D);

 if (numnodes() == 1)
   return NULL;

  int my_rank, nel, i, j, k, vn, iv, el, AdjPartitionID, NAdjPartitionID, Linked_Element_ID;
  int *AdjacentPartitions_temp1, *AdjacentPartitions_temp2,*AdjacentPartitions;
  double dx,dy,dz;

  Element *E,*E2;
  AdjacentPartitions_temp1 = NULL;
  AdjacentPartitions_temp2 = NULL;

 // FILE *pFile;
 // char fname[128];
 // sprintf(fname,"part_list.dat.%d",mynode());
 // pFile = fopen(fname,"w");

  nel = EL->nel;
  my_rank = mynode();
  NAdjPartitionID = 0;

  for(i = 0; i < Nbndry_faces; ++i){
    E = EL->flist[OuterElmntFaceList[i*2]];
    j = OuterElmntFaceList[i*2+1];
    AdjPartitionID = -1;
    if (E->face[j].link == 0) continue;
       if (partition[E->face[j].link->eid] != my_rank);{ //adjacent partition, add to list
         AdjPartitionID = partition[E->face[j].link->eid];

         if (IsPartInList(AdjPartitionID,NAdjPartitionID,AdjacentPartitions_temp1)) continue;

         NAdjPartitionID++;
       //  fprintf(pFile," Pf %d\n",AdjPartitionID);

         if (NAdjPartitionID == 1){
           AdjacentPartitions_temp1 = new int[1];
           AdjacentPartitions_temp1[0] = AdjPartitionID;
         }
         else{
           AdjacentPartitions_temp2 = new int[NAdjPartitionID-1];
           memcpy(AdjacentPartitions_temp2,AdjacentPartitions_temp1,(NAdjPartitionID-1)*sizeof(int));
           delete[] AdjacentPartitions_temp1;
           AdjacentPartitions_temp1 = new int[NAdjPartitionID];
           memcpy(AdjacentPartitions_temp1,AdjacentPartitions_temp2,(NAdjPartitionID-1)*sizeof(int));
           AdjacentPartitions_temp1[NAdjPartitionID-1] = AdjPartitionID;
           delete[] AdjacentPartitions_temp2;
         }
       }
    /* check if other partitions are linked at edge or vertex */
  }




  for (el = 0; el < nel; ++el){
     if ( (partition[el] == my_rank) || IsPartInList(partition[el],NAdjPartitionID,AdjacentPartitions_temp1) ) continue;
     /* check if partitions may overlapp */
      E = EL->flist[el];
      for (k = 0; k < E->Nverts; ++k){
        dx = E->vert[k].x - R_center[1];
        dy = E->vert[k].y - R_center[2];
        dz = E->vert[k].z - R_center[3];
        if ( sqrt(dx*dx+dy*dy+dz*dz) <= (R_center[0]*1.0001) ){
        // possibly adjacent partition
        // need to check if there is a vertex partition "my_rank" with the same coordinates
        // check vertices on the boundary of partition "my_rank" only
          for(i = 0; i < Nbndry_faces; ++i){
            E2 =  EL->flist[OuterElmntFaceList[i*2]];
            j = OuterElmntFaceList[i*2+1];
            for (iv = 0; iv < E2->Nfverts(j); ++iv){
              vn = E->vnum(j,iv);
              dx = E->vert[k].x - E2->vert[vn].x;
              dy = E->vert[k].y - E2->vert[vn].y;
              dz = E->vert[k].z - E2->vert[vn].z;
              if ( sqrt(dx*dx+dy*dy+dz*dz) <= 0.00001){
                AdjPartitionID = partition[el];
                NAdjPartitionID++;
            //    fprintf(pFile," Pve %d\n",AdjPartitionID);
                if (NAdjPartitionID == 1){
                  AdjacentPartitions_temp1 = new int[1];
                  AdjacentPartitions_temp1[0] = AdjPartitionID;
                }
                else{
                  AdjacentPartitions_temp2 = new int[NAdjPartitionID-1];
                  memcpy(AdjacentPartitions_temp2,AdjacentPartitions_temp1,(NAdjPartitionID-1)*sizeof(int));
                  delete[] AdjacentPartitions_temp1;
                  AdjacentPartitions_temp1 = new int[NAdjPartitionID];
                  memcpy(AdjacentPartitions_temp1,AdjacentPartitions_temp2,(NAdjPartitionID-1)*sizeof(int));
                  AdjacentPartitions_temp1[NAdjPartitionID-1] = AdjPartitionID;
                  delete[] AdjacentPartitions_temp2;
                }
                goto next_element;
             }//end of "if ( scrt(dx*dx..."
           } //end of "for (iv = 0..."
        }  //end of "for(i = 0..."
      }  //end of "if ( scrt(dx*dx..."
    } // end of "for (k = 0..."
    next_element:
    ;
   }
   //fclose(pFile);
   AdjacentPartitions = new int[NAdjPartitionID];
   memcpy(AdjacentPartitions,AdjacentPartitions_temp1,(NAdjPartitionID)*sizeof(int));
   delete[] AdjacentPartitions_temp1;
   NAdjPartitionID_output[0] = NAdjPartitionID;
   return AdjacentPartitions;

}//end of function

int IsPartInList(int part, int NAdjPartitionID, int *ListAdjacentPartitions){

  int i;
  for (i = 0; i < NAdjPartitionID; ++i){
    if (ListAdjacentPartitions[i] == part)
      return 1;
  }

  return 0;
}
