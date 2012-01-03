#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------------*
 * This is a routine to add +xvalue to the x coordinate and +yvalue        *
 * to the y coordinate of an rea file.                                     *
 * Usage:    mvgrid +xvalue +yvalue +zvalue file[.rea]                     *
 *-------------------------------------------------------------------------*/
/* only needed to compile with gen_utils.o */
char *prog   = "gridscal";
char *usage  = "Usage: gridscal +xscal +yscal +zscal file[.rea] \n");
char *author = "";
char *rcsid  = "";
char *help   = "";

main(int argc, char *argv[]){
  register int i;
  double x[8],mv[3];
  char file[BUFSIZ],buf[BUFSIZ];
  FILE *fp,*fp_new;

  if(argc != 5){
    fprintf(stdout,"usage");
    exit(-1);
  }

  if(!strstr(argv[argc-1],".rea"))
    sprintf(file,"%s.rea",argv[argc-1]);
  else
    sprintf(file,"%s",argv[argc-1]);

  mv[0] = atof(argv[argc-4]);
  mv[1] = atof(argv[argc-3]);
  mv[2] = atof(argv[argc-2]);

  if(!(fp = fopen(file,"r"))){
    fprintf(stdout,"File %s does not exist\n",file);
    exit(-1);
  }

  fp_new = stdout;

  while(fgets(buf,BUFSIZ,fp)){
    fputs(buf,fp_new);
    if(strstr(buf,"ELEMENT")||strstr(buf,"Element")){
      if(strstr(buf,"Hex") || strstr(buf,"Hex")){
  for(i = 0; i < 3; ++i){
    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf%lf%lf%lf%lf%lf%lf",x,x+1,x+2,x+3,x+4,x+5,
     x+6,x+7);
    sprintf(buf," %lf %lf %lf %lf %lf %lf %lf %lf \n",
      x[0]*mv[i],x[1]*mv[i],x[2]*mv[i],x[3]*mv[i],
      x[4]*mv[i],x[5]*mv[i],x[6]*mv[i],x[7]*mv[i]);
    fputs(buf,fp_new);
  }
      }
      else if(strstr(buf,"Prism") || strstr(buf,"prism")){
  for(i = 0; i < 3; ++i){
    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf%lf%lf%lf%lf",x,x+1,x+2,x+3,x+4,x+5);
    sprintf(buf," %lf %lf %lf %lf %lf %lf \n",
      x[0]*mv[i],x[1]*mv[i],x[2]*mv[i],
      x[3]*mv[i],x[4]*mv[i],x[5]*mv[i]);
    fputs(buf,fp_new);
  }
      }
      else if(strstr(buf,"Pyr") || strstr(buf,"pyr")){
  for(i = 0; i < 3; ++i){
    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf%lf%lf%lf",x,x+1,x+2,x+3,x+4);
    sprintf(buf," %lf %lf %lf %lf %lf \n",
      x[0]*mv[i],x[1]*mv[i],x[2]*mv[i],
      x[3]*mv[i],x[4]*mv[i]);
    fputs(buf,fp_new);
  }
      }
      else {
  for(i = 0; i < 3; ++i){
    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf%lf%lf",x,x+1,x+2,x+3);
    sprintf(buf," %lf %lf %lf %lf %lf \n",
      x[0]*mv[i],x[1]*mv[i],x[2]*mv[i],x[3]*mv[i]);
    fputs(buf,fp_new);
  }
      }
    }
  }

  fclose(fp);
  fclose(fp_new);

  return 0;
}
