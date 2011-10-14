#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------------*
 * This is a routine to add +xvalue to the x coordinate and +yvalue        *
 * to the y coordinate of an rea file.                                     *
 * Usage:    mvgrid +xvalue +yvalue +zvalue file[.rea]                     *
 *-------------------------------------------------------------------------*/
/* only needed to compile with gen_utils.o */
char *prog   = "mvpts";
char *usage  = "mvpts\n";
char *author = "";
char *rcsid  = "";
char *help   = "";

main(int argc, char *argv[])
{
  register int i,j;
  double x[3],mv[3],c;
  char file[BUFSIZ],buf[BUFSIZ];
  int   n;
  FILE *fp,*fp_new;

  if(argc != 5){
    fprintf(stdout,"Usage:    mvpts +xvalue +yvalue +zvalue file \n");
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

  fgets(buf,BUFSIZ,fp);
  sscanf(buf,"%d\n",&n);
  fputs(buf,fp_new);

  for(i = 0; i < n; ++i){
    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf%lf%lf ",x,x+1,x+2,&c);
    fprintf(fp_new,"%lf  %lf  %lf  %lf\n",x[0]+mv[0],x[1]+mv[1],x[2]+mv[2],c);
  }

  fclose(fp);
  fclose(fp_new);

  return 0;
}
