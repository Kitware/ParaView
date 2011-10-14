#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <veclib.h>

/* only needed to compile with gen_utils.o */
char *prog   = "vertmv";
char *usage  = "vertmv:  [options]  file[.rea] \n";
char *author = "";
char *rcsid  = "";
char *help   = "";

main(int argc, char *argv[])
{
  register int i,j;
  double **x,**y,**z,ratio;
  char file[BUFSIZ],buf[BUFSIZ];
  FILE *fp,*fp_new;
  int  eid, vid,vid1, *nvert,nel;
  double xmv,ymv,zmv,xmv1,ymv1,zmv1;

  if(argc != 6){
    fprintf(stdout,"Usage: vertmg eid vid vid1 ratio file[.rea] \n");
    exit(-1);
  }

  if(!strstr(argv[argc-1],".rea"))
    sprintf(file,"%s.rea",argv[argc-1]);
  else
    sprintf(file,"%s",argv[argc-1]);

  eid   = atof(argv[argc-5]);
  vid   = atoi(argv[argc-4]);
  vid1  = atoi(argv[argc-3]);
  ratio = atof(argv[argc-2]);

  if(!(fp = fopen(file,"r"))){
    fprintf(stdout,"File %s does not exist\n",file);
    exit(-1);
  }

  fp_new = stdout;

  while(fgets(buf,BUFSIZ,fp)){
    fputs(buf,fp_new);
    if(strstr(buf,"MESH"))
      break;
  }

  fgets(buf,BUFSIZ,fp);
  fputs(buf,fp_new);

  sscanf(buf,"%d",&nel);

  x = dmatrix(0,nel-1,0,7);
  y = dmatrix(0,nel-1,0,7);
  z = dmatrix(0,nel-1,0,7);
  nvert = ivector(0,nel-1);

  /* read mesh */
  for(i = 0; i < nel; ++i){
    fgets(buf,BUFSIZ,fp);
    if(strstr(buf,"Hex") || strstr(buf,"Hex")){
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf%lf%lf%lf%lf",x[i],x[i]+1,x[i]+2,x[i]+3,
       x[i]+4,x[i]+5,x[i]+6,x[i]+7);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf%lf%lf%lf%lf",y[i],y[i]+1,y[i]+2,y[i]+3,
       y[i]+4,y[i]+5,y[i]+6,y[i]+7);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf%lf%lf%lf%lf",z[i],z[i]+1,z[i]+2,z[i]+3,
       z[i]+4,z[i]+5,z[i]+6,z[i]+7);
      nvert[i] = 8;
    }
    else if(strstr(buf,"Prism") || strstr(buf,"prism")){
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf%lf%lf",x[i],x[i]+1,x[i]+2,x[i]+3,x[i]+4,x[i]+5);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf%lf%lf",y[i],y[i]+1,y[i]+2,y[i]+3,y[i]+4,y[i]+5);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf%lf%lf",z[i],z[i]+1,z[i]+2,z[i]+3,z[i]+4,z[i]+5);
      nvert[i] = 6;
    }
    else if(strstr(buf,"Pyr") || strstr(buf,"pyr")){
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf%lf",x[i],x[i]+1,x[i]+2,x[i]+3,x[i]+4);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf%lf",y[i],y[i]+1,y[i]+2,y[i]+3,y[i]+4);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf%lf",z[i],z[i]+1,z[i]+2,z[i]+3,z[i]+4);
      nvert[i] = 5;
    }
    else {
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf",x[i],x[i]+1,x[i]+2,x[i]+3);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf",y[i],y[i]+1,y[i]+2,y[i]+3);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf",z[i],z[i]+1,z[i]+2,z[i]+3);
      nvert[i] = 4;
    }
  }

  if(vid > nvert[eid-1]){
    fprintf(stderr,"Error: vertex id %d does not exist in element %d\n",
      vid,eid);
  exit(-1);
  }

  xmv  = x[eid-1][vid-1];
  ymv  = y[eid-1][vid-1];
  zmv  = z[eid-1][vid-1];
  xmv1 = x[eid-1][vid1-1];
  ymv1 = y[eid-1][vid1-1];
  zmv1 = z[eid-1][vid1-1];


  for(i = 0; i < nel; ++i){
    for(j = 0; j < nvert[i]; ++j)
      if(sqrt((x[i][j]-xmv)*(x[i][j]-xmv) + (y[i][j]-ymv)*(y[i][j]-ymv) +
        (z[i][j]-zmv)*(z[i][j]-zmv))< 1E-6){
  x[i][j] = xmv + ratio*(xmv1-xmv);
  y[i][j] = ymv + ratio*(ymv1-ymv);
  z[i][j] = zmv + ratio*(zmv1-zmv);
      }
  }

  /* print mesh */
  for(i = 0; i < nel; ++i)
    switch(nvert[i]){
    case 8:
      fprintf(fp_new,"Element %d Hex\n",i+1);
      fprintf(fp_new,"%le %le %le %le %le %le %le %le \n",x[i][0],x[i][1],
        x[i][2],x[i][3],x[i][4],x[i][5],x[i][6],x[i][7]);
      fprintf(fp_new,"%le %le %le %le %le %le %le %le \n",y[i][0],y[i][1],
        y[i][2],y[i][3],y[i][4],y[i][5],y[i][6],y[i][7]);
      fprintf(fp_new,"%le %le %le %le %le %le %le %le \n",z[i][0],z[i][1],
        z[i][2],z[i][3],z[i][4],z[i][5],z[i][6],z[i][7]);
      break;
    case 6:
      fprintf(fp_new,"Element %d Prism\n",i+1);
      fprintf(fp_new,"%le %le %le %le %le %le \n",x[i][0],x[i][1],
        x[i][2],x[i][3],x[i][4],x[i][5]);
      fprintf(fp_new,"%le %le %le %le %le %le \n",y[i][0],y[i][1],
        y[i][2],y[i][3],y[i][4],y[i][5]);
      fprintf(fp_new,"%le %le %le %le %le %le \n",z[i][0],z[i][1],
        z[i][2],z[i][3],z[i][4],z[i][5]);
      break;
    case 5:
      fprintf(fp_new,"Element %d Pyramid\n",i+1);
      fprintf(fp_new,"%le %le %le %le %le \n",x[i][0],x[i][1],
        x[i][2],x[i][3],x[i][4]);
      fprintf(fp_new,"%le %le %le %le %le \n",y[i][0],y[i][1],
        y[i][2],y[i][3],y[i][4]);
      fprintf(fp_new,"%le %le %le %le %le \n",z[i][0],z[i][1],
        z[i][2],z[i][3],z[i][4]);
      break;
    case 4:
      fprintf(fp_new,"Element %d Tet\n",i+1);
      fprintf(fp_new,"%le %le %le %le  \n",x[i][0],x[i][1],
        x[i][2],x[i][3]);
      fprintf(fp_new,"%le %le %le %le \n",y[i][0],y[i][1],
        y[i][2],y[i][3]);
      fprintf(fp_new,"%le %le %le %le \n",z[i][0],z[i][1],
        z[i][2],z[i][3]);
      break;
    default:
      fprintf(stderr,"do not recognise nvert %d \n",nvert[i]);
      exit(-1);
      break;
    }

  while(fgets(buf,BUFSIZ,fp))
    fputs(buf,fp_new);

  free_dmatrix(x,0,0); free_dmatrix(y,0,0);  free_dmatrix(z,0,0);
  free(nvert);
  fclose(fp);
  fclose(fp_new);

  return 0;
}
