#ifndef PITTAR_H_INCLUDED
#define PITTAR_H_INCLUDED

#include "stack.h"

//defines a header for entire archive file
struct header
{
    long int dict_offset; //location of dictionary
};

struct string
{
    char name[200];
};

typedef struct
{
	int c;
	int a;
	int x;
	int m;
	int p;
	int j;
	char *name;
} args;

/* global array stores list of files and directories*/
struct string *fileList;

/* eight file access modes */
char * modes []={"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};

/* creates archive file*/
void create_archive(const char *a_name, int size, char c_flag, const char *mode);

// use recursion if file/directories exist in <directory-list>, to store in archive file
void create_archive_r(const char *name, const char *dir_name, char c_flag);

void create_archive_f(const char *name, char *f_name, char c_flag);

/* append to existing archive */
void append_archive(const char *a_name, int size, char c_flag);

/* extract files from archive, uncompressing when necessary */
void extract_archive(const char *a_name);

/* print meta-data for every file and directory in archive */
void print_metadata(const char *a_name);

/* display the hierarchies of the files and directories  */
void print_hierarchy(const char *a_name);



/* UTILITY FUNCTIONS */

/* process command line arguments */
int process_c_args(int argc, char *argv[], args *list);

/* compress file into .z file DONE*/
void compress_file(const char *f_name);

/* extract .z file DONE*/
void extract_file(const char *f_name);

/* extract stat information for a file */
void get_file_stat(const char *name, DATA *dict);

/* split full path into components*/
void process_path(const char *p_name, int level, const char type);

/* copy file contents into archive file */
void copy_to_archive(const char *f_name, const char *a_nme, DATA *dict);

/* checks file extension */
int check_ext(const char *f_name, const char *extn);

/* printf usage instructions */
void print_usage();

#endif
