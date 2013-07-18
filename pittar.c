/*
 * Yuriy Koziy
 * yuriykoziy@hotmail.com
 * Pitt archiver program
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libgen.h>
#include <unistd.h>
#include "pittar.h"
#include "stack.h"

STACK *stack;
STACK *temp;


int main(int argc, char *argv[], char *envp[])
{
    args list;
    int ret = 0;
    ret = process_c_args(argc, argv, &list);
    if(ret < 0)
    {
        return -1;
    }
    printf("c: %d a: %d x: %d m: %d p: %d j: %d\n", list.c, list.a, list.x, list.m, list.p, list.j);

    if(list.c == 1 && list.j == 0)
    {
        create_archive(list.name, ret, 0, "wb");
    }
    if(list.m == 1)
    {
        print_metadata(list.name);
    }
    if(list.x == 1)
    {
        extract_archive(list.name);
    }
    if(list.p == 1)
    {
        print_hierarchy(list.name);
    }
    if(list.a == 1 && list.j == 0)
    {
        append_archive(list.name, ret, 0);
    }
    if(list.j == 1 && list.c == 1)
    {
        create_archive(list.name, ret, 1, "wb");
    }
    if(list.j == 1 && list.a == 1)
    {
        append_archive(list.name, ret, 1);
    }
    free(fileList);
    fileList = 0;
    return 0;
}

void create_archive(const char *a_name, int size, char c_flag, const char *mode)
{
    FILE *fp;
    DATA *ret = malloc(sizeof(DATA));
    int i = 0;
    struct header head;
    struct stat s;
    head.dict_offset = -1;
    if ((fp = fopen(a_name, mode)) == NULL)
    {
        perror("file I/O");
        return;
    }
    fwrite(&head, sizeof(head), 1, fp);
    fclose(fp);
    for(i = 0; i < size; i++)
    {
        if( stat(fileList[i].name,&s) == 0 )
        {
            if(s.st_mode & S_IFDIR)
            {
                create_archive_r(a_name, fileList[i].name, c_flag);
            }
            else if(s.st_mode & S_IFREG)
            {
                create_archive_f(a_name, fileList[i].name, c_flag);
            }
            else
            {
                printf("not a file but something else %s\n",fileList[i].name);
            }
        }
        else
        {
            perror("Failed to get file status");
            exit(EXIT_FAILURE);
        }
    }

    if ((fp = fopen(a_name, "ab")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }

    head.dict_offset = ftell(fp);
    while((ret = pop(&stack)) != NULL)
    {
        if(fwrite(ret, sizeof(DATA), 1, fp) != 1)
        {
            perror("file writing error");
            exit(EXIT_FAILURE);
        }
    }
    free(ret);
    fclose(fp);

    if ((fp = fopen(a_name, "r+")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }
    if(fwrite(&head, sizeof(head), 1, fp) < 1)
    {
        perror("file writing error");
        exit(EXIT_FAILURE);
    }
    fclose(fp);
    return;
}

void create_archive_f(const char *name, char *f_name, char c_flag)
{
    DATA dict;
    char *file_name = 0;
    char *file_name_c = 0;

    file_name = basename(f_name);

    //compression flag set
    if(c_flag == 1)
    {
        if(check_ext(f_name, "Z") == 0)
        {
            compress_file(f_name);
            file_name_c = (char*)malloc(strlen(f_name)+3);
            strcpy(file_name_c, f_name);
            strcat(file_name_c, ".Z");
            get_file_stat(file_name_c, &dict);

            file_name = basename(file_name_c);
            *dict.f_name = '\0';
            strncat(dict.f_name, file_name, 199);
            dict.c_flag = 1;

            copy_to_archive(file_name_c, name, &dict);

            extract_file(f_name);
            free(file_name_c);
            file_name_c  = 0;
        } else
        {
            get_file_stat(f_name, &dict);

            *dict.f_name = '\0';
            strncat(dict.f_name, file_name, 199);
            dict.c_flag = 1;

            copy_to_archive(f_name, name, &dict);
        }
    } else if (c_flag == 0)
    {
        get_file_stat(f_name, &dict);

        *dict.f_name = '\0';
        strncat(dict.f_name, file_name, 199);

        if(check_ext(dict.f_name, "Z") == 1)
        {
            dict.c_flag = 1;
        } else
        {
            dict.c_flag = 0;
        }
        copy_to_archive(f_name, name, &dict);
    }
    return;
}

void create_archive_r(const char *name, const char *dir_name, char c_flag)
{
    DIR * dp;
    DATA dict;
    struct dirent *dir;
    char *newname = 0;
    char *file_name = 0;

    if((dp = opendir(dir_name)) == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        dir = readdir(dp);
        if(dir == NULL)
        {
            break;
        }
        if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
        {
            newname = (char*)malloc(strlen(dir_name)+strlen(dir->d_name)+2);
            strcpy(newname, dir_name);
            strcat(newname,"/");
            strcat(newname, dir->d_name);
            get_file_stat(newname, &dict);
            if(dict.type == '-')
            {
                //compression flag set
                if(c_flag == 1)
                {
                    if(check_ext(newname, "Z") == 0)
                    {
                        compress_file(newname);
                        file_name = (char*)malloc(strlen(newname)+3);
                        strcpy(file_name, newname);
                        strcat(file_name, ".Z");
                        get_file_stat(file_name, &dict);
                        dict.c_flag = 1;
                        copy_to_archive(file_name, name, &dict);
                        extract_file(file_name);
                        free(file_name);
                        file_name  = 0;
                    } else
                    {
                        dict.c_flag = 1;
                        copy_to_archive(newname, name, &dict);
                    }
                } else if (c_flag == 0)
                {
                    if(check_ext(newname, "Z") == 1)
                    {
                        dict.c_flag = 1;
                    } else
                    {
                        dict.c_flag = 0;
                    }
                    printf("write file: %s\n",dict.f_name); //debug
                    copy_to_archive(newname, name, &dict);
                }
            }
            free(newname);
            newname = 0;
        }
        if(dir->d_type & DT_DIR)
        {
            if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
            {
                newname = (char*)malloc(strlen(dir_name)+strlen(dir->d_name)+2);
                strcpy(newname, dir_name);
                strcat(newname,"/");
                strcat(newname, dir->d_name);
                create_archive_r(name, newname, c_flag);
                free(newname);
                newname = 0;
            }
        }
    }

    if(closedir(dp))
    {
        perror("closedir");
        exit(EXIT_FAILURE);
    }

    get_file_stat(dir_name, &dict);
    if(dict.type == 'd')
    {
        push(&stack,&dict);
    }
    return;
}

void append_archive(const char *a_name, int size, char c_flag)
{
    struct header h;
    DATA d;
    DATA *ret = malloc(sizeof(*ret));
    FILE *fp;

    //read dictionary into stack
    if ((fp = fopen(a_name, "rb")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }
    if(fread(&h, sizeof(h), 1, fp) < 1)
    {
        perror("file reading");
        exit(EXIT_FAILURE);
    }
    fclose(fp);
    if ((fp = fopen(a_name, "rb")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }
    if(fseek(fp, h.dict_offset, SEEK_SET) < 0)
    {
        perror("fseek error");
        exit(EXIT_FAILURE);
    }

    while(fread(&d, sizeof(DATA), 1, fp) == 1)
    {
        push(&stack,&d);
    }
    fclose(fp);

    //remove old dictionary
    if(truncate(a_name, h.dict_offset) != 0)
    {
        perror("truncate error");
        exit(EXIT_FAILURE);
    }

    while((ret = pop(&temp)) != NULL)
    {
        push(&stack,ret);
    }
    free(ret);
    create_archive(a_name, size, c_flag, "rb+");
    return;
}

void extract_archive(const char *a_name)
{
    struct header h;
    int status = 0;
    int n = 0;
    char buffer[1];
    DATA d;
    FILE *fp;
    FILE *write;
    FILE *read;

    if((fp = fopen(a_name, "rb")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }
    if(fread(&h, sizeof(h), 1, fp) < 1)
    {
        perror("file reading");
        exit(EXIT_FAILURE);
    }
    fclose(fp);
    if((fp = fopen(a_name, "rb")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }
    if(fseek(fp, h.dict_offset, SEEK_SET) < 0)
    {
        perror("fseek error");
        exit(EXIT_FAILURE);
    }

    //create all the directories based on archive
    while(fread(&d, sizeof(DATA), 1, fp) == 1)
    {
        if(d.type == 'd')
        {
            status = mkdir(d.f_name, d.perms);
            if(status == -1)
            {
                perror("mkdir error");
            }
            printf("%s\n", d.f_name); //debug printf
        }
    }
    fclose(fp);

    //need to recreate files
    if((fp = fopen(a_name, "rb")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }
    if(fseek(fp, h.dict_offset, SEEK_SET) < 0)
    {
        perror("fseek error");
        exit(EXIT_FAILURE);
    }
    //create all the files based on archive, extracting when necesarry
    while(fread(&d, sizeof(DATA), 1, fp) == 1)
    {
        if(d.type == '-')
        {
            printf("extract file %s with cflag: %d\n", d.f_name, d.c_flag); //debug printf
            if ((read = fopen(a_name, "rb")) == NULL)
            {
                perror("file I/O");
                exit(EXIT_FAILURE);
            }
            if ((write = fopen(d.f_name, "ab")) == NULL)
            {
                perror("file I/O");
                exit(EXIT_FAILURE);
            }

            if(fseek(read, d.file_offset, SEEK_SET) < 0)
            {
                perror("fseek error");
                exit(EXIT_FAILURE);
            }

            for(n = 0; n < d.size; n++)
            {
                if(fread(buffer, sizeof(char), 1, read) > 0)
                {
                    if(fwrite(buffer, sizeof(char), 1, write) != 1)
                    {
                        perror("write failed");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            fclose(write);
            fclose(read);

            if(d.c_flag == 1)
            {
                extract_file(d.f_name);
            }
        }
    }
    fclose(fp);
}

void print_metadata(const char *a_name)
{
    struct header h;
    char perms[10];
    int i = 0;
    int j = 0;
    DATA d;
    FILE *fp;

    if ((fp = fopen(a_name, "rb")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }
    if(fread(&h, sizeof(h), 1, fp) < 1)
    {
        perror("file reading");
        exit(EXIT_FAILURE);
    }
    fclose(fp);
    if ((fp = fopen(a_name, "rb")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }
    if(fseek(fp, h.dict_offset, SEEK_SET) < 0)
    {
        perror("fseek error");
        exit(EXIT_FAILURE);
    }
    printf("\n");
    while(fread(&d, sizeof(DATA), 1, fp) == 1)
    {
        *perms = '\0';
        for (i=2; i>=0; i--)
        {
            j = (d.perms >> (i*3)) & 07;
            strcat(perms,modes[j]);
        }

        printf("%c%s %d/%d%8d %s %s %ld\n", d.type, perms, d.uid, d.gid, d.size, d.mtime, d.f_name, d.file_offset);
    }
    printf("\n");
    fclose(fp);
    return;
}

void print_hierarchy(const char *a_name)
{
    struct header h;
    DATA d;
    FILE *fp;

    if ((fp = fopen(a_name, "rb")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }
    if(fread(&h, sizeof(h), 1, fp) < 1)
    {
        perror("file reading");
        exit(EXIT_FAILURE);
    }
    fclose(fp);
    if ((fp = fopen(a_name, "rb")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }
    if(fseek(fp, h.dict_offset, SEEK_SET) < 0)
    {
        perror("fseek error");
        exit(EXIT_FAILURE);
    }
    while(fread(&d, sizeof(DATA), 1, fp) == 1)
    {
        printf("\n");
        process_path(d.f_name, 0, d.type);
    }
    printf("\n");
    fclose(fp);
    return;
}

int process_c_args(int argc, char *argv[], args *list)
{
    int c = 0;
    int count = 0;
    int i = 0;
    list->c = 0;
    list->a = 0;
    list->x = 0;
    list->m = 0;
    list->p = 0;
    list->j = 0;
    list->name = 0;
    if(argc == 1)
    {
        printf("\nNo arguments provided\n");
        print_usage();
        return -1;
    }

    //process command line args
    while((c = getopt (argc, argv, "caxmpj")) != -1)
    {
        switch (c)
        {
            case 'c':
                list->c = 1;
                break;
            case 'a':
                list->a = 1;
                break;
            case 'x':
                list->x = 1;
                break;
            case 'm':
                list->m = 1;
                break;
            case 'p':
                list->p = 1;
                break;
            case 'j':
                list->j = 1;
                break;
            case '?':
                if (isprint(optopt))
                {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                    return -1;
                } else
                {
                     fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                     return -1;
                }
            default:
                return -1;
       }
    }
    if((list->j == 1))
    {
        if(!(list->c != list->a))
        {
            printf("The flag -j can be only used in conjunction with the flags -c or -a\n");
            return -1;
        }
    }

    if(c == -1)
    {
        //assume first non option is archive name
        list->name = argv[optind];
        if(list->name == 0)
        {
            printf("No files specified\n");
            return -1;
        }
        if(check_ext(list->name, "pitt") == 0)
        {
            printf("Pitt Archiver only works with .pitt files\n");
            return -1;
        }
        optind++;
        count = argc - optind;
        fileList = (struct string *)malloc(count*sizeof(struct string));

        for(i = 0; i < count; i++)
        {
            strncpy(fileList[i].name, argv[optind], 200);
            fileList[i].name[199] = '\0';
            optind++;
        }
    }
    return count;
}

void compress_file(const char *file_name)
{
    pid_t pid = 0;
    char name[50];
    char *arg[3];
    arg[0] = "./compress";
    snprintf(name, 50, "%s", file_name);
    printf("compress_file: %s\n", name);
    arg[1] = name;
    arg[2] = NULL;
    if((pid = fork()) < 0)
    {
        perror("Fork failed!");
        exit(-1);
    }
    else if(pid == 0)
    {
        //in child process
        execvp(arg[0], arg);
        //execvp only returns on error
        exit(-1);
    } else
    {
        //in parent process
        wait(NULL);
    }
    return;
}

void extract_file(const char *file_name)
{
    pid_t pid = 0;
    char name[50];
    char par[3];
    char *arg[4];
    arg[0] = "./compress";
    snprintf(name, 50, "%s", file_name);
    printf("decompress_file: %s\n", name);
    snprintf(par, 3, "%s", "-d");
    arg[1] = par;
    arg[2] = name;
    arg[3] = NULL;
    if((pid = fork()) < 0)
    {
        perror("Fork failed!");
        exit(-1);
    }
    else if(pid == 0)
    {
        //in child process
        execvp(arg[0], arg);
        //execvp only returns on error
        exit(-1);
    } else
    {
        //in parent process
        wait(NULL);
    }
    return;
}

void get_file_stat(const char *name, DATA *dict)
{
    struct stat filestat;
    dict->file_offset = 0;

    if(stat(name, &filestat))
    {
        perror("Failed to get file status");
        exit(EXIT_FAILURE);
    } else
    {
        switch(filestat.st_mode & S_IFMT)
        {
            case S_IFREG:
                dict->type = '-';
                break;
            case S_IFDIR:
                dict->type = 'd';
                break;
            default:
                dict->type = '?';
                break;
        }
        strncpy(dict->mtime, ctime(&filestat.st_mtime)+4, 13);
        dict->mtime[12] = '\0';
        strncpy(dict->f_name, name, 100);
        dict->f_name[100] = '\0';
        dict->uid = filestat.st_uid;
        dict->gid = filestat.st_gid;
        dict->size = filestat.st_size;
        dict->perms = filestat.st_mode;
    }
}

void process_path(const char *p_name, int level, const char type)
{
    char name[200];
    char n_name[200];
    int i = 0;
    char *pos = strchr(p_name, '/');
    if(pos != NULL)
    {
        strncpy(name, p_name, (pos-p_name)+1);
        name[(pos-p_name)+1] = '\0';
        if(level > 0)
        {
            printf(" |");
        }
        for(i = 0; i < level; i++)
        {
            putchar('-');
        }
        printf("%s\n", name);
        memmove (n_name,p_name + (pos-p_name) + 1,(strlen(p_name) - strlen(name)) + 1);
        name[(pos-p_name)+1] = '\0';
        process_path(n_name, level+1, type);
    } else
    {
        if(level > 0)
        {
            printf(" |");
        }
        for(i = 0; i < level; i++)
        {
            printf("-");
        }
        if(type == 'd')
        {
            strncpy(name, p_name, strlen(p_name));
            name[strlen(p_name)] = '/';
            name[strlen(p_name)+1] = '\0';
            printf("%s\n", name);
        } else {
            printf("%s\n", p_name);
        }
        return;
    }
}

void copy_to_archive(const char *f_name, const char *a_name, DATA *dict)
{
    FILE *fp_to;
    FILE *fp_from;
    char buffer[512];
    int n = 0;

    if ((fp_from = fopen(f_name, "rb")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }

    if ((fp_to = fopen(a_name, "ab")) == NULL)
    {
        perror("file I/O");
        exit(EXIT_FAILURE);
    }

    dict->file_offset = ftell(fp_to);
    push(&stack,dict);

    while ((n = fread(buffer, sizeof(char), sizeof(buffer), fp_from)) > 0)
    {
        if(fwrite(buffer, sizeof(char), n, fp_to) != n)
        {
            perror("write failed");
            exit(EXIT_FAILURE);
        }
    }

    fclose(fp_to);
    fclose(fp_from);
}

int check_ext(const char *f_name, const char *extn)
{
    const char *d = strrchr(f_name, '.');
    if(!d || d == f_name)
    {
        return 0;
    }
    if(strncmp(d+1, extn, sizeof(extn)) == 0)
    {
        return 1;
    }
    return 0;
}

void print_usage()
{
    printf("Usage:\n");
    printf("pittar {-c|-a|-x|-m|-p|-j} <archive-file> <file/directory list>\n");
    printf("If <file/directory list> is empty, all files in current directory are archived\n");
    printf("<archive-file> must have .pitt extension\n");
    printf("Command line option descriptions:\n");
    printf("     -c\n");
    printf("\tarchive files from <file/directory list> into .pitt archive\n");
    printf("     -a\n");
    printf("\tappend files from <file/directory list> into  existing .pitt archive\n");
    printf("     -x\n");
    printf("\textract files and directories from existing .pitt archive file\n");
    printf("     -m\n");
    printf("\t-print metadata of existing .pitt archive\n");
    printf("     -p\n");
    printf("\tprint hierchies of existing .pitt archive\n");
    printf("     -j\n");
    printf("\tcompress files as .Z, can be only used with -a and -c options\n");
    printf("\n");
    return;
}
