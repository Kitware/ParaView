#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <veclib.h>

/*-------------------------------------------------------------------------*
 * This is a routine to add +xvalue to the x coordinate and +yvalue        *
 * to the y coordinate of an rea file.                                     *
 * Usage:    mvgrid +xvalue +yvalue +zvalue file[.rea]                     *
 *-------------------------------------------------------------------------*/
/* only needed to compile with gen_utils.o */
char *prog   = "rottec";
char *usage  = "rottec\n";
char *author = "";
char *rcsid  = "";
char *help   = "";

main(int argc, char *argv[])
{
  register int i,j;
  double x[3],mv[3],cp,sp,c;
  char file[BUFSIZ],buf[BUFSIZ];
  int   n,n1,n2;
  FILE *fp,*fp_new;

  if(argc != 5){
    fprintf(stdout,"Usage:  rottec +xrot +yrot +zrot file \n");
    exit(-1);
  }

  sprintf(file,"%s",argv[argc-1]);

  mv[0] = atof(argv[argc-4]);
  mv[1] = atof(argv[argc-3]);
  mv[2] = atof(argv[argc-2]);

  if(!(fp = fopen(file,"r"))){
    fprintf(stdout,"File %s does not exist\n",file);
    exit(-1);
  }

  fp_new = stdout;

  fp_new = stdout;

  fgets(buf,BUFSIZ,fp);
  fputs(buf,fp_new);

  while(fgets(buf,BUFSIZ,fp)){
    fputs(buf,fp_new);
    sscanf(buf,"ZONE I=%d, J=%d",&n1,&n2);
    for(i =0 ; i < n1*n2; ++i){
      fscanf(fp,"%lf%lf%lf ",x,x+1,x+2);

      cp = cos(mv[0]*M_PI/180.0);
      sp = sin(mv[0]*M_PI/180.0);
      drot(1,x+1,1,x+2,1,cp,sp);
      cp = cos(mv[1]*M_PI/180.0);
      sp = sin(mv[1]*M_PI/180.0);
      drot(1,x+2,1,x,1,cp,sp);
      cp = cos(mv[2]*M_PI/180.0);
      sp = sin(mv[2]*M_PI/180.0);
      drot(1,x,1,x+1,1,cp,sp);
      fprintf(fp_new,"%lf  %lf  %lf ",x[0],x[1],x[2]);
      fgets(buf,BUFSIZ,fp);
      fputs(buf,fp_new);
    }
  }

  fclose(fp);
  fclose(fp_new);

  return 0;
}
