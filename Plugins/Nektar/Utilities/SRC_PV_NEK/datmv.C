#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------------*
 * This is a routine to add +xvalue to the x coordinate and +yvalue        *
 * to the y coordinate of an rea file.                                     *
 * Usage:    mvgrid +xvalue +yvalue +zvalue file[.rea]                     *
 *-------------------------------------------------------------------------*/
/* only needed to compile with gen_utils.o */
char *prog   = "datmv";
char *usage  = "datmv\n";
char *author = "";
char *rcsid  = "";
char *help   = "";

main(int argc, char *argv[])
{
  register int i,j;
  double x[3],mv[3];
  char file[BUFSIZ],buf[BUFSIZ];
  int n1,n2,old=0,ncurv,nsurf;
  FILE *fp,*fp_new;

  if(argc != 5){
    fprintf(stdout,"Usage:    datmv +xvalue +yvalue +zvalue file[.dat] \n");
    exit(-1);
  }

  if(!strstr(argv[argc-1],".dat"))
    sprintf(file,"%s.dat",argv[argc-1]);
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

  fgets(buf,BUFSIZ,fp);
  if(strstr(buf,"Old")) old = 1;
  fputs(buf,fp_new);

  fgets(buf,BUFSIZ,fp);
  sscanf(buf,"%d%d\n",&ncurv,&nsurf);
  fputs(buf,fp_new);

  fgets(buf,BUFSIZ,fp);
  fputs(buf,fp_new);

  for(i = 0; i < ncurv; ++i){
    fgets(buf,BUFSIZ,fp);
    if(old)
      sscanf(buf,"%*d%*d%d\n",&n1);
    else{
      fputs(buf,fp_new);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%d\n",&n1);
    }
    fputs(buf,fp_new);

    for(j = 0; j < n1; ++j){
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf",x,x+1,x+2);
      fprintf(fp_new,"%lf  %lf  %lf\n",x[0]+mv[0],x[1]+mv[1],x[2]+mv[2]);
    }
  }

  fgets(buf,BUFSIZ,fp);
  fputs(buf,fp_new);

  for(i = 0; i < nsurf; ++i){
    fgets(buf,BUFSIZ,fp);
    if(old)
      sscanf(buf,"%*d%*d%d%d\n",&n1,&n2);
    else{
      fputs(buf,fp_new);
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%d%d\n",&n1,&n2);
    }
    fputs(buf,fp_new);
    for(j = 0; j < n1*n2; ++j){
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf%lf",x,x+1,x+2);
      fprintf(fp_new,"%lf  %lf  %lf\n",x[0]+mv[0],x[1]+mv[1],x[2]+mv[2]);
    }
  }

  /* dump out rest of file */
  while(fgets(buf,BUFSIZ,fp))
    fputs(buf,fp_new);

  fclose(fp);
  fclose(fp_new);

  return 0;
}
