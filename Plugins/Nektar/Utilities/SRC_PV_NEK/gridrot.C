#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*-------------------------------------------------------------------------*
 * This is a routine to add +xvalue to the x coordinate and +yvalue        *
 * to the y coordinate of an rea file.                                     *
 * Usage:    gridrot theta phi file[.rea]                     *
 *-------------------------------------------------------------------------*/
/* only needed to compile with gen_utils.o */
char *prog   = "nek2tec";
char *usage  = "gridrot:  [options]  -r file[.rea]  input[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   = "";

main(int argc, char *argv[])
{
  register int i,j;
  double x[8],y[8],z[8],mv[2],x1,y1;
  char file[BUFSIZ],buf[BUFSIZ];
  FILE *fp,*fp_new;

  if(argc != 4){
    fprintf(stdout,"Usage:    gridrot +theta +phi file[.rea] \n");
    fprintf(stdout,"theta is about the z axis\n");
    fprintf(stdout,"phi is about the new y axis\n");
    exit(-1);
  }

  if(!strstr(argv[argc-1],".rea"))
    sprintf(file,"%s.rea",argv[argc-1]);
  else
    sprintf(file,"%s",argv[argc-1]);

  mv[0] = atof(argv[argc-4]);
  mv[1] = atof(argv[argc-3]);

  if(!(fp = fopen(file,"r"))){
    fprintf(stdout,"File %s does not exist\n",file);
    exit(-1);
  }

  fp_new = stdout;

  while(fgets(buf,BUFSIZ,fp)){
    fputs(buf,fp_new);
    if(strstr(buf,"ELEMENT")||strstr(buf,"Element")){
      if(strstr(buf,"Hex") || strstr(buf,"Hex")){
  fgets(buf,BUFSIZ,fp);
  sscanf(buf,"%lf%lf%lf%lf%lf%lf%lf%lf",x,x+1,x+2,x+3,x+4,x+5,
         x+6,x+7);
  sscanf(buf,"%lf%lf%lf%lf%lf%lf%lf%lf",y,y+1,y+2,y+3,y+4,y+5,
         y+6,y+7);
  sscanf(buf,"%lf%lf%lf%lf%lf%lf%lf%lf",z,z+1,z+2,z+3,z+4,z+5,
         z+6,z+7);

  for(j = 0; j < 8; ++j){
    x1 = x[j]*cos(180*mv[0]/2/M_PI) + y[j]*sin(180*mv[0]/2/M_PI);
    y1 = y[j]*cos(180*mv[0]/2/M_PI) - x[j]*sin(180*mv[0]/2/M_PI);
    x[j] = x1;
    y[j] = y1;
    x1 = x[j]*cos(180*mv[0]/2/M_PI) - z[j]*sin(180*mv[0]/2/M_PI);
    y1 = z[j]*cos(180*mv[0]/2/M_PI) + x[j]*sin(180*mv[0]/2/M_PI);
    x[j] = x1;
    z[j] = y1;
  }

  sprintf(buf," %lf %lf %lf %lf %lf %lf %lf %lf \n",
    x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7]);
  sprintf(buf," %lf %lf %lf %lf %lf %lf %lf %lf \n",
    y[0],y[1],y[2],y[3],y[4],y[5],y[6],y[7]);
  sprintf(buf," %lf %lf %lf %lf %lf %lf %lf %lf \n",
    z[0],z[1],z[2],z[3],z[4],z[5],z[6],z[7]);
  fputs(buf,fp_new);
      }
      else if(strstr(buf,"Prism") || strstr(buf,"prism")){
  fgets(buf,BUFSIZ,fp);
  sscanf(buf,"%lf%lf%lf%lf%lf%lf",x,x+1,x+2,x+3,x+4,x+5);
  sscanf(buf,"%lf%lf%lf%lf%lf%lf",y,y+1,y+2,y+3,y+4,y+5);
  sscanf(buf,"%lf%lf%lf%lf%lf%lf",z,z+1,z+2,z+3,z+4,z+5);
  for(j = 0; j < 6; ++j){
    x1 = x[j]*cos(180*mv[0]/2/M_PI) + y[j]*sin(180*mv[0]/2/M_PI);
    y1 = y[j]*cos(180*mv[0]/2/M_PI) - x[j]*sin(180*mv[0]/2/M_PI);
    x[j] = x1;
    y[j] = y1;
    x1 = x[j]*cos(180*mv[0]/2/M_PI) - z[j]*sin(180*mv[0]/2/M_PI);
    y1 = z[j]*cos(180*mv[0]/2/M_PI) + x[j]*sin(180*mv[0]/2/M_PI);
    x[j] = x1;
    z[j] = y1;
  }
  sprintf(buf," %lf %lf %lf %lf %lf %lf \n",
    x[0],x[1],x[2],x[3],x[4],x[5]);
  sprintf(buf," %lf %lf %lf %lf %lf %lf \n",
    y[0],y[1],y[2],y[3],y[4],y[5]);
  sprintf(buf," %lf %lf %lf %lf %lf %lf \n",
    z[0],z[1],z[2],z[3],z[4],z[5]);
  fputs(buf,fp_new);
      }
      else if(strstr(buf,"Pyr") || strstr(buf,"pyr")){
  fgets(buf,BUFSIZ,fp);
  sscanf(buf,"%lf%lf%lf%lf%lf",x,x+1,x+2,x+3,x+4);
  sscanf(buf,"%lf%lf%lf%lf%lf",y,y+1,y+2,y+3,y+4);
  sscanf(buf,"%lf%lf%lf%lf%lf",z,z+1,z+2,z+3,z+4);
  for(j = 0; j < 5; ++j){
    x1 = x[j]*cos(180*mv[0]/2/M_PI) + y[j]*sin(180*mv[0]/2/M_PI);
    y1 = y[j]*cos(180*mv[0]/2/M_PI) - x[j]*sin(180*mv[0]/2/M_PI);
    x[j] = x1;
    y[j] = y1;
    x1 = x[j]*cos(180*mv[0]/2/M_PI) - z[j]*sin(180*mv[0]/2/M_PI);
    y1 = z[j]*cos(180*mv[0]/2/M_PI) + x[j]*sin(180*mv[0]/2/M_PI);
    x[j] = x1;
    z[j] = y1;
  }
  sprintf(buf," %lf %lf %lf %lf %lf \n",
    x[0],x[1],x[2],x[3],x[4]);
  sprintf(buf," %lf %lf %lf %lf %lf \n",
    y[0],y[1],y[2],y[3],y[4]);
  sprintf(buf," %lf %lf %lf %lf %lf \n",
    z[0],z[1],z[2],z[3],z[4]);
  fputs(buf,fp_new);
      }
      else {
  fgets(buf,BUFSIZ,fp);
  sscanf(buf,"%lf%lf%lf%lf",x,x+1,x+2,x+3);
  sscanf(buf,"%lf%lf%lf%lf",y,y+1,y+2,y+3);
  sscanf(buf,"%lf%lf%lf%lf",z,z+1,z+2,z+3);
  for(j = 0; j < 4; ++j){
    x1 = x[j]*cos(180*mv[0]/2/M_PI) + y[j]*sin(180*mv[0]/2/M_PI);
    y1 = y[j]*cos(180*mv[0]/2/M_PI) - x[j]*sin(180*mv[0]/2/M_PI);
    x[j] = x1;
    y[j] = y1;
    x1 = x[j]*cos(180*mv[0]/2/M_PI) - z[j]*sin(180*mv[0]/2/M_PI);
    y1 = z[j]*cos(180*mv[0]/2/M_PI) + x[j]*sin(180*mv[0]/2/M_PI);
    x[j] = x1;
    z[j] = y1;
  }
  sprintf(buf," %lf %lf %lf %lf %lf \n",x[0],x[1],x[2],x[3]);
  sprintf(buf," %lf %lf %lf %lf %lf \n",y[0],y[1],y[2],y[3]);
  sprintf(buf," %lf %lf %lf %lf %lf \n",z[0],z[1],z[2],z[3]);
  fputs(buf,fp_new);
      }
    }
  }

  fclose(fp);
  fclose(fp_new);

  return 0;
}
