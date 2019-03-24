#include "../util/so_stdio.h"
#include <sys/types.h>	/* open */
#include <sys/stat.h>	/* open */
#include <fcntl.h>	/* O_RDWR, O_CREAT, O_TRUNC, O_WRONLY */
#include <unistd.h>	/* close */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define DEFAULT_BUF_SIZE 4096

 typedef struct _so_file{
    int fileDescriptor;
    char buffer[DEFAULT_BUF_SIZE];
    char filePath[2000];
    char mode[2];
    int buff_len;
    int err_flag;
    int buff_curr;
    int offset_pos;
    int curr_pos;
    int prev_fnct;     //1 pentru write 0 pentru read;
    int last_pos_write;
    int status_child;

 } SO_FILE;
#define SO_EOF (-1)


int
so_ferror(SO_FILE *stream){
    if(stream->err_flag == SO_EOF)
        return SO_EOF;
    return 0;
}
int
so_fileno(SO_FILE *stream){
    if(stream != NULL){
        stream->err_flag = 0;
        return stream->fileDescriptor;
    }
    else{
        stream->err_flag = SO_EOF;
        return SO_EOF;
    }
}


long so_ftell(SO_FILE *stream){
    long rc;
    rc = lseek(stream->fileDescriptor, 0, SEEK_CUR);
    if(rc < 0){
        stream->err_flag = SO_EOF;
        return SO_EOF;
    }
    rc += stream->offset_pos;
    stream->err_flag = 0;
    return rc;
}

int
so_fseek(SO_FILE *stream, long offset, int whence){
    if(stream == NULL){
        stream->err_flag = SO_EOF;
        return SO_EOF;
    }
    int rc;
    if(stream->prev_fnct == 1){
        rc = write(stream->fileDescriptor,stream->buffer,stream->buff_curr);
        if(rc == -1){
            stream->err_flag = SO_EOF;
        return SO_EOF;
        }
        stream->buff_curr = 0;
        stream->offset_pos = 0;
        stream->err_flag = 0;
    }

    stream->prev_fnct = -2;

    rc = lseek(stream->fileDescriptor, offset, whence); 
    if(rc == -1){
        stream->err_flag = SO_EOF;
        return SO_EOF;
    }

    stream->err_flag = 0;
    return 0;
}


int
so_feof(SO_FILE *stream){
    long rc,rd;
    rc = lseek(stream->fileDescriptor,0,SEEK_CUR);
    rd = lseek(stream->fileDescriptor, 0, SEEK_END);
    rc = lseek(stream->fileDescriptor, rc, SEEK_SET);
    if(rd + 1 == rc) 
        return SO_EOF;
    else
        return 0;
}

SO_FILE
*so_fopen(const char *pathname, const char *mode) {

        int rc;
	    int fd;
        SO_FILE *file = NULL;
        if(strcmp(mode, "r") == 0) {
            fd = open(pathname, O_RDONLY);
            if(fd  < 0 )
                return NULL;
            file = (SO_FILE *) malloc(sizeof(SO_FILE));
            (*file).fileDescriptor = fd;
            (*file).buff_len = -1;
            (*file).err_flag = 0;
            (*file).buff_curr = 0;
            (*file).offset_pos = 0;
            strcpy((*file).mode,"r");
            strcpy((*file).filePath,pathname);
            chmod((*file).filePath, S_IRWXO | S_IRWXG | S_IRWXU);
            file->prev_fnct = -1;
            file->last_pos_write = -1;
            return file;
        }

        if(strcmp(mode, "r+") == 0) {
            fd = open(pathname, O_RDWR); //cred ca e bine asa citisem pe net ca cica face request sa il deschida
            if(fd  < 0 )
                return NULL;
            file = (SO_FILE *) malloc(sizeof(SO_FILE));
            (*file).fileDescriptor = fd;
            (*file).buff_len = -1;
            (*file).err_flag = 0;
            (*file).buff_curr = 0;
            (*file).offset_pos = 0;
            strcpy((*file).mode,"r+");
            strcpy((*file).filePath,pathname);
            chmod((*file).filePath, S_IRWXO | S_IRWXG | S_IRWXU);
            file->prev_fnct = -1;
            file->last_pos_write = -1;
            return file;
        }

        if(strcmp(mode, "w") == 0) {
            fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC);
            if(fd  < 0 )
                return NULL;
            file = (SO_FILE *) malloc(sizeof(SO_FILE));
            (*file).fileDescriptor = fd;
            (*file).buff_len = -1;
            (*file).err_flag = 0;
            (*file).buff_curr = 0;
            (*file).offset_pos = 0;
            strcpy((*file).mode,"w");
            strcpy((*file).filePath,pathname);
            chmod((*file).filePath, S_IRWXO | S_IRWXG | S_IRWXU);
            file->prev_fnct = -1;
            file->last_pos_write = -1;
            return file;
        }

        if(strcmp(mode, "w+") == 0) {
            fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC);
            if(fd  < 0 )
                return NULL;
            file = (SO_FILE *) malloc(sizeof(SO_FILE));
            (*file).fileDescriptor = fd;
            (*file).buff_len = -1;
            (*file).err_flag = 0;
            (*file).buff_curr = 0;
            (*file).offset_pos = 0;
            strcpy((*file).filePath,pathname);
            strcpy((*file).mode,"w+");
            chmod((*file).filePath, S_IRWXO | S_IRWXG | S_IRWXU);
            file->prev_fnct = -1;
            file->last_pos_write = -1;
            return file;
        }

        if(strcmp(mode, "a") == 0) {
            fd = open(pathname, O_WRONLY|O_CREAT|O_APPEND);
            if(fd  < 0 )
                return NULL;
            file = (SO_FILE *) malloc(sizeof(SO_FILE));
            (*file).fileDescriptor = fd;
            (*file).buff_len = -1;
            (*file).err_flag = 0;
            (*file).buff_curr = 0;
            (*file).offset_pos = 0;
            strcpy((*file).mode,"a");
            strcpy((*file).filePath,pathname);
            chmod((*file).filePath, S_IRWXO | S_IRWXG | S_IRWXU);
            //lseek(file->fileDescriptor,0,SEEK_END);
            file->prev_fnct = -1;
            file->last_pos_write = -1;
            return file;
        }

        if(strcmp(mode, "a+") == 0) {
            fd = open(pathname, O_RDWR|O_CREAT|O_APPEND); //am dubii aici 
            if(fd  < 0 )
                return NULL;
            file = (SO_FILE *) malloc(sizeof(SO_FILE));
            (*file).fileDescriptor = fd;
            (*file).buff_len = -1;
            (*file).err_flag = 0;
            (*file).buff_curr = 0;
            (*file).offset_pos = 0;
            strcpy((*file).mode,"a+");
            strcpy((*file).filePath,pathname);
            chmod((*file).filePath, S_IRWXO | S_IRWXG | S_IRWXU);
            lseek((*file).fileDescriptor, 0, SEEK_SET);
            file->prev_fnct = -1;
            file->last_pos_write = -1;
            return file;
        }

        return NULL;

}
int
so_fflush(SO_FILE *stream){
    int rc;
    if(stream == NULL){
        stream->err_flag = SO_EOF;
        return SO_EOF;
    }
    if(stream->prev_fnct == 1){
    rc = write(stream->fileDescriptor,stream->buffer,stream->buff_curr);
            if(rc == -1 && rc != stream->buff_curr){
                stream->err_flag = SO_EOF;
                return SO_EOF;
            }
    }
    stream->prev_fnct = -2;
    stream->buff_curr = 0;
    stream->err_flag = 0;
    return 0;
}

int
so_fputc(int c, SO_FILE *stream){

    int rc;

        if(stream->buff_curr == sizeof(stream->buffer)){
            rc = write(stream->fileDescriptor,stream->buffer,sizeof(stream->buffer));
            if(rc == -1 && rc != sizeof(stream->buffer)){
                stream->err_flag = SO_EOF;
                return SO_EOF;
            }
            stream->buff_curr = 0;
            stream->offset_pos = 0;
            stream->err_flag = 0;
        }
        
        stream->buffer[stream->buff_curr++] = c;
        stream->offset_pos = stream->buff_curr;
        stream->err_flag = 0;
        stream->prev_fnct = 1;
        stream->last_pos_write = lseek(stream->fileDescriptor, 0, SEEK_CUR);
        return c;
}

int
so_fclose(SO_FILE *stream){
    int rc;

    if(stream == NULL){
        stream->err_flag = SO_EOF;
        return SO_EOF;
    }

    int test = so_fflush(stream);
    rc = close((*stream).fileDescriptor);
    if(rc < 0 || test == SO_EOF){
                stream->err_flag = SO_EOF;
                free(stream);
                return SO_EOF;
            }
    free(stream);
    return 0;
}

int
so_fgetc(SO_FILE *stream){
    unsigned char c;
    int rc;
    int old_pos;
    int check_pos;
    if(stream == NULL)
        return SO_EOF;
    check_pos = lseek(stream->fileDescriptor,0,SEEK_CUR);
    if(stream->buff_len == -1 || check_pos != stream->curr_pos){
            old_pos = so_ftell(stream);
            rc =  read(stream->fileDescriptor,stream->buffer, sizeof(stream->buffer));
            //printf("VALOARE LUI RC = %d SI APEL SISTEM\n",rc);
            if(rc  == -1){   
                stream->err_flag = SO_EOF;
                return SO_EOF;
            }
            stream->buff_len = sizeof(stream->buffer) - 1;
            //duc cursorul la pozitia initiala 
            lseek(stream->fileDescriptor, old_pos, SEEK_SET);
            int i;
            int test;
            c = stream->buffer[stream->buff_curr];
            for(i = 0; i < DEFAULT_BUF_SIZE; i++){
                    memcpy(&stream->buffer[i], &stream->buffer[i+1], 1);
            }
            test = lseek(stream->fileDescriptor,1,SEEK_CUR);
            //pe parcurs ce citesc mut si cursorul
            if(so_feof(stream) != 0){
                stream->err_flag = SO_EOF;
                return SO_EOF;
            }
            //scad lungime de buff
            stream->buff_len--;
            stream->curr_pos = lseek(stream->fileDescriptor, 0, SEEK_CUR);
            stream->err_flag = 0;
            stream->prev_fnct = 0;
            return c;
    }
    if(stream->buff_len != -1)
    { 
        int i;
        c = stream->buffer[stream->buff_curr];
        for(i = 0; i < DEFAULT_BUF_SIZE; i++){
                memcpy(&stream->buffer[i], &stream->buffer[i+1], 1);
        }
        int test = lseek(stream->fileDescriptor,1,SEEK_CUR);
        if(so_feof(stream) != 0){
            stream->err_flag = SO_EOF;
            return SO_EOF;
        }
        stream->buff_len--;
        stream->curr_pos = lseek(stream->fileDescriptor, 0, SEEK_CUR);
        stream->err_flag = 0;
        stream->prev_fnct = 0;
        return c;
    }
    stream->err_flag = SO_EOF;
    return SO_EOF;
}

size_t 
so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream){
    int i = 0,j;
    unsigned char aux;
    unsigned char *test = ptr;
    if( ptr == NULL || stream == NULL){
        return 0;
        stream->err_flag = SO_EOF;
    }
    for(i = 0; i < nmemb * size; i++){
        aux = so_fgetc(stream);
        if(stream->err_flag == SO_EOF){
            //printf("AM AJUNS AICI %d %d\n", i,aux);
            int t = lseek(stream->fileDescriptor,0,SEEK_CUR);
            if(t < nmemb*size){
                read(stream->fileDescriptor,stream->buffer,0);
            }
            stream->err_flag = SO_EOF;
            return i;
        }
        *(test + i) = aux; 
    }
    stream->err_flag = 0;
    return i/size;
    
}

 size_t 
 so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream){
    int i;
    char aux;
    unsigned char *test = ptr;
    if( ptr == NULL || stream == NULL)
        return 0;
    if(strcmp(stream->mode,"a") == 0 ){
        lseek(stream->fileDescriptor, 0, SEEK_END);
    }
    if(strcmp(stream->mode,"a+") == 0 ){
        lseek(stream->fileDescriptor, 0, SEEK_END);
    }
    for(i = 0; i < nmemb * size; i++){
        aux = so_fputc(*(test + i), stream);
        if(stream->err_flag == SO_EOF)
            return SO_EOF;
    }
    stream->err_flag = 0;
    return i/size;
 }

SO_FILE 
*so_popen(const char *command, const char *type){
    
        pid_t pid;
        int redirect[2];
 
        SO_FILE *file;
        int status;
        int fd; 
        fd= open("footr.txt", O_RDWR | O_CREAT | O_TRUNC);  
        if(fd  < 0 ){
            printf("PULS");
            return NULL;
            }
            file = (SO_FILE *) malloc(sizeof(SO_FILE));
            (*file).fileDescriptor = fd;
            chmod("footr.txt", S_IRWXO | S_IRWXG | S_IRWXU);
        
            
        pid = fork();

        switch (pid) {
	    case -1:
	 	return NULL;
	    case 0:
	 	/* child process */
        if(type[0] == 'r'){
        dup2(fd, STDOUT_FILENO); // replace stdout
        }
        else{
        dup2(fd, STDIN_FILENO);
        }
	 	execlp("sh", "sh", "-c", command, NULL);
	 	/* only if exec failed */
	 	return NULL;
	    default:
	 	/* parent process */
	 	break;
	 }

    wait(&status);
	if (WIFEXITED(status))
        file->status_child = status;
    (*file).fileDescriptor = fd;
    (*file).buff_len = -1;
    (*file).err_flag = 0;
    (*file).buff_curr = 0;
    (*file).offset_pos = 0;
    file->prev_fnct = -1;
    file->last_pos_write = -1;    
    so_fseek(file,0,SEEK_SET);
    
	return file;
}
int
so_pclose(SO_FILE *stream){
    
    int returnat = stream->status_child;
    so_fclose(stream);
    return returnat;
}
