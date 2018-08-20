/*
 * listrates
 * An extension of metarates working with access lists on arbitrary file sets.
 *
 * metarates
 * Copyright 2004, Scientific Computing Division, University Corporation for Atmospheric Research
 * See http://www.cisl.ucar.edu/css/software/metarates/
 */
#ifndef listrates_h
#define listrates_h

extern void describe(char* progname);
extern int 	initsec();
extern double 	secondr();
extern void	getFileList(char** pname, int node,
			    int numfilesPerProc, char* listname);
extern void free_names(int numNames);
#endif
