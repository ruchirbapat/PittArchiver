Yuriy Koziy

Pitt Archiver

Description:

	This project implements a Pitt Archiver program.

Files Included:

	README.txt
	report.txt
	Makefile
    compress - a program that compresses files as a .z file.
	pittar.c - pitt acrhiver.
	pittar.h - pitt archiver header file, contains function prototypes, etc.
	stack.c - implements a simple stack.	
	stack.h - stack header file contains function prototypes, etc.


Use Instruction:

	The program should be compiled by typing "make", Makefile
	has other usable functions such as "make clean" which cleans all the
	.o files and finally "make clean2" removes all the executables which 
	were created. Pittar acceps the following parameters: 
		
     -c -- archive files from <file/directory list> into .pitt archive
     -a -- append files from <file/directory list> into  existing .pitt archive
     -x -- extract files and directories from existing .pitt archive file
     -m -- print metadata of existing .pitt archive
     -p -- print hierchies of existing .pitt archive
     -j -- compress files as .Z, can be only used with -a and -c options

	
Known bugs/issues:

	NONE
