#ifndef listgen_h
#define listgen_h

enum 	ListType { t_dir, t_file, t_uniform, t_zipf, t_invalid };

/* listgen.c */
void describe(char* progname);
void read_names_from_list(char * listname);
void list_from_fileset(char *path, FILE * pFile, enum ListType ltype);
void list_from_list(FILE * pFile, enum ListType ltype, double zipfSkew, int targetSize, int create);

/* genzipf.c */
int 	zipf(double alpha, int n);
double 	rand_val(int seed);

#endif
