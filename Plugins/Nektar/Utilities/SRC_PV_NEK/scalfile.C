#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------------*
 * This is a routine to add +xvalue to the x coordinate and +yvalue        *
 * to the y coordinate of an rea file.                                     *
 * Usage:    mvgrid +xvalue +yvalue +zvalue file[.rea]                     *
 *-------------------------------------------------------------------------*/
/* only needed to compile with gen_utils.o */
char *prog   = "gridscal";
char *usage  = "Usage: fcnscal  file[.rea] \n";
char *author = "";
char *rcsid  = "";
char *help   = "";

main(int argc, char *argv[]){
  int    i,j,npts;
  double x[4],y[4];
  double *xw,*yw,*cw,yloc,cloc,factor,fac0,fac1;
  char   file[BUFSIZ],buf[BUFSIZ];
  FILE   *fp,*fp_new,*datfp;

  if(argc != 4){
    fprintf(stdout,"usage: scalfile factor file.dat file.rea \n");
    exit(-1);
  }

  // open file.rea
  if(!strstr(argv[argc-1],".rea"))
    sprintf(file,"%s.rea",argv[argc-1]);
  else
    sprintf(file,"%s",argv[argc-1]);

  if(!(fp = fopen(file,"r"))){
    fprintf(stdout,"File %s does not exist\n",file);
    exit(-1);
  }

  // open file.dat
  if(!strstr(argv[argc-2],".dat"))
    sprintf(file,"%s.dat",argv[argc-2]);
  else
    sprintf(file,"%s",argv[argc-2]);

  if(!(datfp = fopen(file,"r"))){
    fprintf(stdout,"File %s does not exist\n",file);
    exit(-1);
  }

  fp_new = stdout;
  factor = atof(argv[argc-3]);

  // read in date file.
  fgets(buf,BUFSIZ,datfp);
  sscanf(buf,"%d",&npts);

  xw  = new double[npts];
  yw  = new double[npts];
  cw  = new double[npts];

  for(i = 0; i < npts; ++i){
    fgets(buf,BUFSIZ,datfp);
    sscanf(buf,"%lf%lf%lf",xw+i,yw+i,cw+i);
  }

  // subtract off cw the linear of the end points;
  fac0 = cw[0]; fac1 = cw[npts-1];
  for(i = 0; i < npts; ++i)
    cw[i] -= fac0*(xw[npts-1]-xw[i])/(xw[npts-1]-xw[0]) +
      fac1*(xw[i]-xw[0])/(xw[npts-1]-xw[0]);


  while(fgets(buf,BUFSIZ,fp)){
    fputs(buf,fp_new);

    if(strstr(buf,"ELEMENT")||strstr(buf,"Element")){

      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf",x,x+1,x+2,x+3);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf",y,y+1,y+2,y+3);

      // Move y
      // find x location of point
      for(j = 0; j < 4; ++j){
  // find index of nearest point
  if(x[j] <= xw[0])
    i = 0;
  else if (x[j] >= xw[npts-1])
    i = npts-2;
  else
    for(i = 0; i < npts-1; ++i)
      if((x[j] >= xw[i])&&(x[j] <= xw[i+1]))
        break;

  fac0 =  (xw[i+1]-x[j]) / (xw[i+1]-xw[i]);
  fac1 =   (x[j]-xw[i])  / (xw[i+1]-xw[i]);

  //scale y value assuming top is at y = 0;
  yloc = yw[i]*fac0 + yw[i+1]*fac1;
  cloc = cw[i]*fac0 + cw[i+1]*fac1;
  y[j] = y[j] + y[j]/yloc*factor*cloc;
      }

      // leave x untouched
      sprintf(buf," %lf %lf %lf %lf \n", x[0],x[1],x[2],x[3]);
      fputs(buf,fp_new);
      sprintf(buf," %lf %lf %lf %lf \n", y[0],y[1],y[2],y[3]);
      fputs(buf,fp_new);
    }
  }


  delete[] xw, yw, cw;

  fclose(fp);
  fclose(fp_new);
  fclose(datfp);

  return 0;
}
