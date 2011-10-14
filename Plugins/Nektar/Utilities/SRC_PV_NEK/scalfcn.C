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
  register int i;
  double x[4],y[4];
  char file[BUFSIZ],buf[BUFSIZ];
  FILE *fp,*fp_new;

  if(argc != 2){
    fprintf(stdout,"usage");
    exit(-1);
}

  if(!strstr(argv[argc-1],".rea"))
    sprintf(file,"%s.rea",argv[argc-1]);
  else
    sprintf(file,"%s",argv[argc-1]);

  if(!(fp = fopen(file,"r"))){
    fprintf(stdout,"File %s does not exist\n",file);
    exit(-1);
  }

  fp_new = stdout;

  while(fgets(buf,BUFSIZ,fp)){
    fputs(buf,fp_new);

    if(strstr(buf,"ELEMENT")||strstr(buf,"Element")){

      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf",x,x+1,x+2,x+3);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf%lf",y,y+1,y+2,y+3);

      //Scale y
      for(i = 0; i < 4; ++i)
  if (x[i] > 4.0 && x[i] < 6.0)
    y[i] *= (1.0 - 0.75*sin(0.5*M_PI*(x[i]-4.0))*
       sin(0.5*M_PI*(x[i]-4.0)));

      // leave x untouched
      sprintf(buf," %lf %lf %lf %lf \n", x[0],x[1],x[2],x[3]);
      fputs(buf,fp_new);
      sprintf(buf," %lf %lf %lf %lf \n", y[0],y[1],y[2],y[3]);
      fputs(buf,fp_new);
    }
  }

  fclose(fp);
  fclose(fp_new);

  return 0;
}
