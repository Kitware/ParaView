
#include "nektar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <veclib.h>

//#define SHORTCUT_PER


void partition_BOX_min_max(Element_List *EL, int *partition, double *XYZ_min_max);
void partition_radius_center(double *XYZ_min_max, double *R_center);
int *get_OuterElmntFaceList(Element_List *EL, int *partition, int *Nbndry_faces);
void WriteTEC_boundary_faces(int Nbndry_faces, int *OuterElmntFaceList, Element_List *EL);
int *get_AdjacentPartitions(Element_List *EL, int *partition, int Nbndry_faces, int *OuterElmntFaceList, double *R_center,int *NAdjPartitionID);
int IsPartInList(int part, int NAdjPartitionID, int *ListAdjacentPartitions);


int *test_partition_connectivity(Element_List *EL, int *partition, int *Npartitions ){


  int   *AdjacentPartitions;

#ifdef SHORTCUT_PER   

  Npartitions[0] = numnodes()-1;
  AdjacentPartitions = new int[Npartitions[0]];

  for(int i = 0; i < mynode(); i++) 
    AdjacentPartitions[i] = i;

  for(int i = (mynode()+1); i < numnodes(); i++)
    AdjacentPartitions[i-1] = i;

  
  return AdjacentPartitions; 

#endif

  double t1;
  double *XYZ_min_max,*R_center;
  int Nbndry_faces,NAdjPartitionID;
  int *OuterElmntFaceList;  

  XYZ_min_max = new double[6];
  R_center = new double[4];


  t1 = MPI_Wtime();
  partition_BOX_min_max(EL, partition, XYZ_min_max);
  //ROOTONLY fprintf(stdout,"partition_BOX_min_max - done in %f sec\n",MPI_Wtime()-t1);
  t1 = MPI_Wtime();
  partition_radius_center(XYZ_min_max,R_center);
  //ROOTONLY fprintf(stdout,"partition_radius_center - done in %f sec\n",MPI_Wtime()-t1);
/*
  fprintf(stdout," partition %d BOX=[%f %f   %f %f   %f %f] \n",mynode(),XYZ_min_max[0],XYZ_min_max[1],
                                                                         XYZ_min_max[2],XYZ_min_max[3],
                                                                         XYZ_min_max[4],XYZ_min_max[5]);
  fprintf(stdout," partition %d R_center = [%f   %f %f %f]\n",mynode(),R_center[0],R_center[1],R_center[2],R_center[3]);
*/
  t1 = MPI_Wtime();
  OuterElmntFaceList = get_OuterElmntFaceList(EL, partition, &Nbndry_faces);
  gsync();  
  //ROOTONLY fprintf(stdout,"get_OuterElmntFaceList - done in %f sec\n",MPI_Wtime()-t1);
//  WriteTEC_boundary_faces(Nbndry_faces, OuterElmntFaceList, EL);

  t1 = MPI_Wtime();
  AdjacentPartitions = get_AdjacentPartitions(EL, partition, Nbndry_faces, OuterElmntFaceList, R_center, &NAdjPartitionID);
  gsync();
  //ROOTONLY fprintf(stdout,"get_AdjacentPartitions - done in %f sec\n",MPI_Wtime()-t1);

/*
  fprintf(stdout," partition %d NAdjPartitionID = %d\n",mynode(),NAdjPartitionID);
  for (int i = 0;  i < NAdjPartitionID; ++i)
     fprintf(stdout," Partition %d AdjacentPartitions[%d] = %d\n",mynode(),i,AdjacentPartitions[i]);
*/
  
  delete[] XYZ_min_max;
  gsync();
  //ROOTONLY 
  //  fprintf(stdout,"file = %s, line = %d\n",__FILE__,__LINE__);

  delete[] R_center;
  gsync();
  //ROOTONLY
  //  fprintf(stdout,"file = %s, line = %d\n",__FILE__,__LINE__);

  delete[] OuterElmntFaceList;
  gsync();
  //ROOTONLY
  //  fprintf(stdout,"file = %s, line = %d\n",__FILE__,__LINE__);

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

#ifdef TEST_OMP

 i = omp_get_max_threads(); 
 double Xmin[i];
 double Xmax[i];
 double Ymin[i];
 double Ymax[i];
 double Zmin[i];
 double Zmax[i];

#pragma omp parallel
{
  int TID = omp_get_thread_num();
  Xmin[TID] = Xmax[TID] = XYZ_min_max[0];         
  Ymin[TID] = Ymax[TID] = XYZ_min_max[2];          
  Zmin[TID] = Zmax[TID] = XYZ_min_max[4];    

  #pragma omp for private(E,x,y,z,j) schedule(dynamic)
  for(i = 0; i < nel; ++i){
    if (partition[i] != my_rank) continue;
    E = EL->flist[i];
    for(j = 0; j < E->Nverts; ++j){ 
      x = E->vert[j].x;
      y = E->vert[j].y;
      z = E->vert[j].z;
      if ( x < Xmin[TID])
        Xmin[TID] = x;
      else if ( x > Xmax[TID])
        Xmax[TID] = x;

      if ( y < Ymin[TID])
        Ymin[TID] = y;
      else if ( y > Ymax[TID])
        Ymax[TID] = y;

      if ( z < Zmin[TID])
        Zmin[TID] = z;
      else if ( z > Zmax[TID])
        Zmax[TID] = z;
    }
  }
  #pragma omp single
   {
   for (i = 0; i < omp_get_num_threads(); ++i){
     if (XYZ_min_max[0] > Xmin[i]) XYZ_min_max[0] = Xmin[i];
     if (XYZ_min_max[1] < Xmax[i]) XYZ_min_max[1] = Xmax[i];      
     if (XYZ_min_max[2] > Ymin[i]) XYZ_min_max[2] = Ymin[i];
     if (XYZ_min_max[3] < Ymax[i]) XYZ_min_max[3] = Ymax[i];
     if (XYZ_min_max[4] > Zmin[i]) XYZ_min_max[4] = Zmin[i];
     if (XYZ_min_max[5] < Zmax[i]) XYZ_min_max[5] = Zmax[i];
   }
  }
}

#else
  for(i = 0; i < nel; ++i){
    if (partition[i] != my_rank) continue;
    E = EL->flist[i];
    for(j = 0; j < E->Nverts; ++j){ 
      x = E->vert[j].x;
      y = E->vert[j].y;
      z = E->vert[j].z;
      if ( x < XYZ_min_max[0])
        XYZ_min_max[0] = x;
      else if ( x > XYZ_min_max[1])
        XYZ_min_max[1] = x;
      if ( y < XYZ_min_max[2])
        XYZ_min_max[2] = y;
      else if ( y > XYZ_min_max[3])
        XYZ_min_max[3] = y;
      if ( z < XYZ_min_max[4])
        XYZ_min_max[4] = z;
      else if ( z > XYZ_min_max[5])
        XYZ_min_max[5] = z;
    }
  }
#endif
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

  int INC_SIZE = 40; 
  int OuterElmntFaceList_size = 40;
  OuterElmntFaceList_temp1 = new int[OuterElmntFaceList_size];
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
          if (Nbndry_faces*2 < OuterElmntFaceList_size){
             OuterElmntFaceList_temp1[(Nbndry_faces-1)*2]   = i;
             OuterElmntFaceList_temp1[(Nbndry_faces-1)*2+1] = j;
          }
          else{
            OuterElmntFaceList_temp2 = new int[OuterElmntFaceList_size+INC_SIZE];
            memcpy(OuterElmntFaceList_temp2,OuterElmntFaceList_temp1,(Nbndry_faces-1)*2*sizeof(int));
            OuterElmntFaceList_size += INC_SIZE; 
            delete[] OuterElmntFaceList_temp1;
            OuterElmntFaceList_temp1 = OuterElmntFaceList_temp2;
            OuterElmntFaceList_temp1[(Nbndry_faces-1)*2]   = i;
            OuterElmntFaceList_temp1[(Nbndry_faces-1)*2+1] = j;
          }
         /*   
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
          */             
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

  double dx,dy,dz; 
  int my_rank, nel, i, j, k, vn, iv, el, AdjPartitionID, NAdjPartitionID, Linked_Element_ID;
  int *AdjacentPartitions_temp1, *AdjacentPartitions_temp2,*AdjacentPartitions;

  Element *E,*E2;
  int INC_SIZE = 50;
  int AdjacentPartitions_size = 50;

  AdjacentPartitions_temp1 = new int[AdjacentPartitions_size];
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
     #if 1
        if (NAdjPartitionID <= AdjacentPartitions_size)
          AdjacentPartitions_temp1[NAdjPartitionID-1] = AdjPartitionID;
        else{
          AdjacentPartitions_temp2 = new int[AdjacentPartitions_size+INC_SIZE];
          memcpy(AdjacentPartitions_temp2,AdjacentPartitions_temp1,(NAdjPartitionID-1)*sizeof(int));
          delete[] AdjacentPartitions_temp1;
          AdjacentPartitions_temp1 = AdjacentPartitions_temp2;
          AdjacentPartitions_temp1[NAdjPartitionID-1] = AdjPartitionID;
          AdjacentPartitions_size += INC_SIZE;
        }
     #else
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
      #endif
       }
    /* check if other partitions are linked at edge or vertex */
  }
  

  double R2_lim,TOL2;
  R2_lim = (R_center[0]*1.0001) * (R_center[0]*1.0001);
  TOL2 = 0.00001*0.00001;

  for (el = 0; el < nel; ++el){
     if ( (partition[el] == my_rank) || IsPartInList(partition[el],NAdjPartitionID,AdjacentPartitions_temp1) ) continue;
     /* check if partitions may overlapp */
      E = EL->flist[el];
      for (k = 0; k < E->Nverts; ++k){
        dx = E->vert[k].x - R_center[1];
        dy = E->vert[k].y - R_center[2];
        dz = E->vert[k].z - R_center[3];
        if ( (dx*dx+dy*dy+dz*dz) <= R2_lim ){
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
              if ( (dx*dx+dy*dy+dz*dz) <= TOL2){
                AdjPartitionID = partition[el];
                NAdjPartitionID++;
         #if 1
                if (NAdjPartitionID < AdjacentPartitions_size)
                   AdjacentPartitions_temp1[NAdjPartitionID-1] = AdjPartitionID;
                else{
                   AdjacentPartitions_temp2 = new int[AdjacentPartitions_size+INC_SIZE];
                   memcpy(AdjacentPartitions_temp2,AdjacentPartitions_temp1,(NAdjPartitionID-1)*sizeof(int));
                   delete[] AdjacentPartitions_temp1;
                   AdjacentPartitions_temp1 = AdjacentPartitions_temp2;
                   AdjacentPartitions_temp1[NAdjPartitionID-1] = AdjPartitionID;
                   AdjacentPartitions_size += INC_SIZE;
                }
         #else
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
         #endif
                goto next_element;
             }//end of "if ( scrt(dx*dx..."
           } //end of "for (iv = 0..."
        }  //end of "for(i = 0..."
      }  //end of "if ( scrt(dx*dx..."
    } // end of "for (k = 0..."
    next_element:
    ;
   }


/************************************************************/

#if 0 

//should take periodicity into account

  double Xdist = 1.0;
  double Ydist = 0.0;
  double Zdist = 0.0;

  int flag_per;

  for (el = 0; el < nel; ++el){
     if ( (partition[el] == my_rank) || IsPartInList(partition[el],NAdjPartitionID,AdjacentPartitions_temp1) ) continue;
     /* check if partitions may overlapp */
      E = EL->flist[el];
      for (k = 0; k < E->Nverts; ++k){
        // need to check if there is a vertex partition "my_rank" with the coordinates shifted by Xdist or Ydist or Zdist
        // check vertices on the boundary of partition "my_rank" only
          for(i = 0; i < Nbndry_faces; ++i){
            E2 =  EL->flist[OuterElmntFaceList[i*2]];
            j = OuterElmntFaceList[i*2+1];
            for (iv = 0; iv < E2->Nfverts(j); ++iv){
              vn = E->vnum(j,iv);
              flag_per = 0;


              /* test X direction */
              dx = E->vert[k].x - E2->vert[vn].x;
              dy = E->vert[k].y - E2->vert[vn].y;
              dz = E->vert[k].z - E2->vert[vn].z;
              dx = fabs(dx) - Xdist;              
              if (sqrt(dx*dx+dy*dy+dz*dz) <= 0.00001)
                 flag_per = 1;

              /* test Y direction */
              dx = E->vert[k].x - E2->vert[vn].x;
              dy = E->vert[k].y - E2->vert[vn].y;
              dz = E->vert[k].z - E2->vert[vn].z;
              dy = fabs(dy) - Ydist;
              if (sqrt(dx*dx+dy*dy+dz*dz) <= 0.00001)
                 flag_per = 1;

              /* test Z direction */
              dx = E->vert[k].x - E2->vert[vn].x;
              dy = E->vert[k].y - E2->vert[vn].y;
              dz = E->vert[k].z - E2->vert[vn].z;
              dz = fabs(dz) - Zdist;
              if (sqrt(dx*dx+dy*dy+dz*dz) <= 0.00001)
                 flag_per = 1;


              if ( flag_per == 1){
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
                goto next_element_P;
             }//end of "if ( scrt(dx*dx..."
           } //end of "for (iv = 0..."
        }  //end of "for(i = 0..."
    } // end of "for (k = 0..."
    next_element_P:
    ;
   }


#endif

/************************************************************/


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

