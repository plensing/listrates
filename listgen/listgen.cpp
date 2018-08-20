#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "listgen.h"

#define MAXPATHLENGTH 4096
char    tmpName[MAXPATHLENGTH];
std::vector< std::string > vname;

main(int argc, char* argv[])
{
	enum ListType ltype = t_invalid;
	char *input_listname = 0;
	char *output_listname = 0;
	char *start_directory = 0;
	double zipf_skew = 0.95;
	unsigned int target_size = 1000;
	char 	opts[] = "i:o:d:n:s:DFUZc";
	int c, create = 0;

	while ( ( c = getopt(argc,argv,opts) ) != -1 ) {
		switch (c) {
			case 'i':
				input_listname = optarg;
				break;
			case 'o':
				output_listname = optarg;
				break;
			case 'd':
				start_directory = optarg;
				break;
			case 'D':
				ltype = t_dir;
				break;
			case 'F':
				ltype = t_file;
				break;
			case 'U':
				ltype = t_uniform;
				break;
			case 'Z':
				ltype = t_zipf;
				break;
			case 'n':
				target_size = atoi(optarg);
				break;
			case 's':
				zipf_skew = atof(optarg);
				break;
			case 'c':
				create = 1;
				break;
			}
		}
	if(!output_listname){
		describe(argv[0]);
		return 0;
	}

	FILE * pFile;
	pFile = fopen (output_listname,"w");

	switch (ltype) {
		case t_file:
			printf("Creating file list from existing file set. \n");
			break;
		case t_dir:
			printf("Creating directory list from existing file set. \n");
			break;
		case t_uniform:
			printf("Creating uniformly random access list. \n");
			break;
		case t_zipf:
			printf("Creating zipf distributed access list, with a skew of %f. \n",zipf_skew);
			break;
	}
	switch (ltype) {
		case t_dir:
		case t_file:
			if(!start_directory){
				describe(argv[0]);
				return 0;
			}
			printf("Starting from path '%s' \n",start_directory);
			printf("Output of list to: %s \n",output_listname);
		    list_from_fileset(start_directory, pFile, ltype);
			break;

		case t_zipf:
		case t_uniform:
			if(!input_listname){
				describe(argv[0]);
				return 0;
			}
			read_names_from_list(input_listname);
			printf("Input list is: '%s', containing %d entries. \n",input_listname, (int) vname.size());
			printf("Output list is: '%s', with %d target entries. \n",output_listname, target_size);
			list_from_list(pFile, ltype, zipf_skew, target_size, create);
			break;
	}

    fclose (pFile);
}

/*
 * Create new access list from existing list.
 */
void list_from_list(FILE * pFile, enum ListType ltype, double zipfSkew, int targetSize, int create)
{
	unsigned int numNames = (int) vname.size();
	unsigned int i, pick;
	rand_val(time(NULL));
	srand (time (NULL));

	std::random_shuffle( vname.begin(), vname.end() );
	for ( i = 0; i < targetSize; i++){

		switch(ltype){
		case t_uniform:
			pick = rand();
			break;
		case t_zipf:
			pick = zipf(zipfSkew,numNames);
			break;
		default:
			printf("Invalid list type selected. \n");
			return;
		}

		pick = pick % numNames;

		if(create)
			fprintf (pFile, "%s/genfile_%d\n",vname[pick].c_str(),i);
		else
			fprintf (pFile, "%s\n", vname[pick].c_str());
	}


}

/*
 * Create list from existing fileset.
 */
void list_from_fileset(char *path, FILE * pFile, enum ListType ltype)
{
	int i;
	struct stat st;
	struct dirent **eps;
	unsigned char d_type;
	char tmpName[MAXPATHLENGTH];
	int n = scandir (path, &eps, 0, alphasort);

	for( i = 0; i < n; i++ ){
		memset(tmpName,0,MAXPATHLENGTH);
		strcpy(tmpName,path);
		if(tmpName[strlen(tmpName)-1] != '/')
			strcat(tmpName,"/");
		strcat(tmpName,eps[i]->d_name);

		stat(tmpName,&st);
		d_type = (st.st_mode >> 12) & 15;

		if(d_type == DT_DIR && eps[i]->d_name[0] == '.' && strlen(eps[i]->d_name) < 3)
			continue;

		if(d_type == DT_DIR)
			list_from_fileset(tmpName, pFile, ltype);

		if( (ltype == t_dir && d_type == DT_DIR) || (ltype == t_file && d_type == DT_REG) )
			fprintf (pFile, "%s\n", tmpName);
	}

}

/*
 * Read names from list.
 */
void read_names_from_list(char * listname)
{
	std::ifstream file;
	file.open(listname);

	if(!file.is_open()){
		printf("Failed opening input list: %s \n",listname);
		return;
	}

	std::string line;
	while(!file.eof()){
		getline(file,line);
		vname.push_back(line);
	}
	file.close();
}

/*
 * describe usage of program
 */
void describe(char* progname)
{
	printf("\n\n");
	printf("Create list from existing file set: \n");
	printf("\t%s -d dir -o ouput_list [-D][-F] \n", progname);
	printf("\t\t D: directory list \n");
	printf("\t\t F: file list \n");

	printf("Create list from an existing list: \n");
	printf("\t%s -i input_list -o output_list [-U][-Z -s skew] -n size -c  \n", progname);
	printf("\t\t U: picks entries from input list according to uniformly random distribution \n");
	printf("\t\t Z: picks entries from input list according to zipf distribtion with the given skew factor\n");
	printf("\t\t c: picks directories according to distribution from input list and generates new filenames there\n");
	printf("\n\n");
}
