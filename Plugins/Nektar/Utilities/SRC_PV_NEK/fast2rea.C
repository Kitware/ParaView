/*
 * program computes connectivity of mesh given in rea file
 *
 * cc -n32 -O2 -c -I/users/tcew/Nektar/include fast2rea.c
 * cc -n32 -O2 -o fast2rea.o -L/users/tcew/Nektar/lib/IRIX64 -lvec -lm

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <veclib.h>

static int same (double z1, double z2)
{
  if (fabs(z1-z2) < 1e-5)
    return 1;
  else
    return 0;
}

main (int argc, char *argv[])
{
  fscanf(stdin, "%d%d%d", &Npt, &Ntri, &Ntet);

  double *x = dvector(0, Npt-1);
  double *y = dvector(0, Npt-1);
  double *z = dvector(0, Npt-1);

  /* read  coordinates */
  for(i=0;i<Npt;++i)
    fscanf(stdin, "%lf", x+i);

  for(i=0;i<Npt;++i)
    fscanf(stdin, "%lf", y+i);

  for(i=0;i<Npt;++i)
    fscanf(stdin, "%lf", z+i);

  int **face_ids = imatrix(0, Ntri-1, 0, 2);

  /* read  face vertices */
  for(i=0;i<Ntri;++i)
    fscanf(stdin, "%d%d%d", face_ids[i],face_ids[i]+1,face_ids[i]+2);

  int *face_flags = ivector(0, Ntri-1);
  for(i=0;i<Ntri;++i)
    fscanf(stdin, "%d", face_flags+i);

  int **tet_ids = imatrix(0, Ntet-1, 0, 3);

  /* read  face vertices */
  for(i=0;i<Ntet;++i)
    fscanf(stdin, "%d%d%d%d", tet_ids[i],tet_ids[i]+1,tet_ids[i]+2,tet_ids[i]+3);

  /* Now find the connectivity */

  int *Ntet_vertin  = ivector(0, Npt-1);
  izero( Npt, Ntet_vertin, 1);
  for(i=0;i<Ntet;++i)
    for(j=0;j<4;++j)
      ++Ntet_vertin[tet_ids[i][j]-1];  /* take 1 off vertex id */

  int **vertintet = (int**) calloc(Npt, sizeof(int*));
  for(i=0;i<Npt;++i)
    vertintet[i] = ivector(0, Ntet_vertin[i]-1);

  int *cnt = ivector(0, Npt-1);
  izero( Npt, cnt, 1);

  for(i=0;i<Ntet;++i)
    for(j=0;j<4;++j){
      vertintet[tet_ids[i][j]-1][cnt[tet_ids[i][j]-1]] = i;  /* take 1 off vertex id */
      ++cnt[tet_ids[i][j]-1];
    }

  int **conn = imatrix(0, Ntet-1, 0, 3);
  ifill(Ntet*4, -1, conn[0], 1);
  /* Face 1 */

  int *flag = ivector(0, Ntet-1);
  izero( Ntet, flag, 1);

  for(i=0;i<Ntet;++i){

    izero( Ntet, flag, 1);
    va = tet_ids[i][0];
    vb = tet_ids[i][1];
    vc = tet_ids[i][2];

    for(j=0;j<cnt[va];++j)
      ++flag[vertintet[va][j]];

    for(j=0;j<cnt[vb];++j)
      ++flag[vertintet[vb][j]];

    for(j=0;j<cnt[vc];++j)
      ++flag[vertintet[vc][j]];

    for(j=0;j<Ntet;++j)
      if(flag[j] == 3){
  conn[i][0] = j;
  break;
      }

    izero( Ntet, flag, 1);
    va = tet_ids[i][0];
    vb = tet_ids[i][1];
    vc = tet_ids[i][3];

    for(j=0;j<cnt[va];++j)
      ++flag[vertintet[va][j]];

    for(j=0;j<cnt[vb];++j)
      ++flag[vertintet[vb][j]];

    for(j=0;j<cnt[vc];++j)
      ++flag[vertintet[vc][j]];

    for(j=0;j<Ntet;++j)
      if(flag[j] == 3){
  conn[i][1] = j;
  break;
      }


    izero( Ntet, flag, 1);
    va = tet_ids[i][1];
    vb = tet_ids[i][2];
    vc = tet_ids[i][3];

    for(j=0;j<cnt[va];++j)
      ++flag[vertintet[va][j]];

    for(j=0;j<cnt[vb];++j)
      ++flag[vertintet[vb][j]];

    for(j=0;j<cnt[vc];++j)
      ++flag[vertintet[vc][j]];

    for(j=0;j<Ntet;++j)
      if(flag[j] == 3){
  conn[i][2] = j;
  break;
      }


    izero( Ntet, flag, 1);
    va = tet_ids[i][0];
    vb = tet_ids[i][2];
    vc = tet_ids[i][3];

    for(j=0;j<cnt[va];++j)
      ++flag[vertintet[va][j]];

    for(j=0;j<cnt[vb];++j)
      ++flag[vertintet[vb][j]];

    for(j=0;j<cnt[vc];++j)
      ++flag[vertintet[vc][j]];

    for(j=0;j<Ntet;++j)
      if(flag[j] == 3){
  conn[i][3] = j;
  break;
      }
  }
}
