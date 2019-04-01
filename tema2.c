#if defined __linux__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#elif defined(_WIN32)

#include <windows.h>
#define DLL_EXPORTS

#else 
#error "Unknown OS!"
#endif

#include <string.h>
#include <stdlib.h>
#include "../util/so_stdio.h"


#define DEFAULT_BUF_SIZE 4096
#define SO_EOF (-1)



typedef struct _so_file {
#if defined __linux__
	int fileDescriptor;
#elif defined(_WIN32)
	HANDLE fileDescriptor;
#endif
	char filePath[2000];
	char mode[2];
	int buff_len;
	int err_flag;
	int buff_curr;
	int offset_pos;
	int curr_pos;
	char buffer[DEFAULT_BUF_SIZE];
	int prev_fnct;     //1 pentru write 0 pentru read;
	int last_pos_write;
	int status_child;

} SO_FILE;

FUNC_DECL_PREFIX int
so_ferror(SO_FILE *stream)
{
	if (stream->err_flag == SO_EOF)
		return SO_EOF;
	return 0;
}
#if defined(__linux__)
FUNC_DECL_PREFIX  int
so_fileno(SO_FILE *stream)
{
	if (stream != NULL) {
		stream->err_flag = 0;
		return stream->fileDescriptor;
	}
	stream->err_flag = SO_EOF;
	return SO_EOF;
}
#elif defined(_WIN32)

FUNC_DECL_PREFIX  HANDLE
so_fileno(SO_FILE *stream)
{
	if (stream != NULL) {
		stream->err_flag = 0;
		return stream->fileDescriptor;
	}
	stream->err_flag = SO_EOF;
	return INVALID_HANDLE_VALUE;
}
#endif

FUNC_DECL_PREFIX  long
so_ftell(SO_FILE *stream)
{
	long rc;
#if defined(__linux__)

	rc = lseek(stream->fileDescriptor, 0, SEEK_CUR);
	if (rc < 0) {
		stream->err_flag = SO_EOF;
		return SO_EOF;
	}
#elif defined(_WIN32)

	rc = SetFilePointer(stream->fileDescriptor, 0, NULL, FILE_CURRENT);
	if (rc == INVALID_SET_FILE_POINTER) {
		stream->err_flag = SO_EOF;
		return SO_EOF;
	}
#endif
	rc += stream->offset_pos;
	stream->err_flag = 0;
	return rc;
}

FUNC_DECL_PREFIX  int
so_fseek(SO_FILE *stream, long offset, int whence)
{
	int rc;
#if defined(_WIN32)
	int written;
#endif

	if (stream == NULL) {
		stream->err_flag = SO_EOF;
		return SO_EOF;
	}

	if (stream->prev_fnct == 1) {
#if defined(__linux__)
		rc = write(stream->fileDescriptor, stream->buffer,
				stream->buff_curr);
		if (rc == -1) {
			stream->err_flag = SO_EOF;
			return SO_EOF;
		}
#elif defined(_WIN32)
		rc = WriteFile(stream->fileDescriptor,
				stream->buffer, stream->buff_curr,
				&written, NULL);
		if (rc == 0) {
			stream->err_flag = SO_EOF;
			return SO_EOF;
		}
#endif
		stream->buff_curr = 0;
		stream->offset_pos = 0;
		stream->err_flag = 0;
	}

	if (stream->prev_fnct == 0)
		stream->curr_pos = 0;
	stream->prev_fnct = -2;
#if defined(__linux__)
	rc = lseek(stream->fileDescriptor, offset, whence);
	if (rc == -1) {
		stream->err_flag = SO_EOF;
		return SO_EOF;
	}
#elif defined(_WIN32)
	rc = SetFilePointer(stream->fileDescriptor, offset, NULL, whence);
	if (rc == INVALID_SET_FILE_POINTER) {
		stream->err_flag = SO_EOF;
		return SO_EOF;
	}
#endif
	stream->err_flag = 0;
	return 0;
}

FUNC_DECL_PREFIX  int
so_feof(SO_FILE *stream)
{
	long rc, rd;
#if defined(__linux__)

	rc = lseek(stream->fileDescriptor, 0, SEEK_CUR);
	rd = lseek(stream->fileDescriptor, 0, SEEK_END);
	rc = lseek(stream->fileDescriptor, rc, SEEK_SET);
#elif defined(_WIN32)

	rc = SetFilePointer(stream->fileDescriptor, 0, NULL, FILE_CURRENT);
	rd = SetFilePointer(stream->fileDescriptor, 0, NULL, FILE_END);
	rc = SetFilePointer(stream->fileDescriptor, rc, NULL, FILE_BEGIN);
#endif
	//printf(" UNDE SUNT %d UNDE E FINALUL %d\n", rc,rd);
	if (rd + 1 == rc)
		return SO_EOF;
	return 0;
}

FUNC_DECL_PREFIX  SO_FILE
*so_fopen(const char *pathname, const char *mode)
{

	int rc;
#if defined(__linux__)
	int fd;
#elif defined(_WIN32)
	HANDLE fd;
#endif
	SO_FILE *file = NULL;

	if (strcmp(mode, "r") == 0) {
#if defined(__linux__)
		fd = open(pathname, O_RDONLY);
		if (fd < 0)
			return NULL;
#elif defined(_WIN32)
		fd = CreateFile(
				pathname,
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL
				);
		if (fd == INVALID_HANDLE_VALUE)
			return NULL;
#endif
	file = (SO_FILE *) malloc(sizeof(SO_FILE));
	(*file).fileDescriptor = fd;
	(*file).buff_len = -1;
	(*file).curr_pos = 0;
	(*file).err_flag = 0;
	(*file).buff_curr = 0;
	(*file).offset_pos = 0;
	strcpy((*file).mode, "r");
	strcpy((*file).filePath, pathname);
	file->prev_fnct = -1;
	file->last_pos_write = -1;
	return file;
	}

	if (strcmp(mode, "r+") == 0) {
#if defined(__linux__)
		fd = open(pathname, O_RDWR);
			if (fd < 0)
				return NULL;
#elif defined(_WIN32)
			fd = CreateFile(
					pathname,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL
					);
			if (fd == INVALID_HANDLE_VALUE)
				return NULL;
#endif
			file = (SO_FILE *) malloc(sizeof(SO_FILE));
			(*file).fileDescriptor = fd;
			(*file).buff_len = -1;
			(*file).err_flag = 0;
			(*file).curr_pos = 0;
			(*file).buff_curr = 0;
			(*file).offset_pos = 0;
			strcpy((*file).mode, "r+");
			strcpy((*file).filePath, pathname);
			file->prev_fnct = -1;
			file->last_pos_write = -1;
			return file;
	}

	if (strcmp(mode, "w") == 0) {
#if defined(__linux__)
		fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd < 0)
			return NULL;
#elif defined(_WIN32)
		fd = CreateFile(
				pathname,
				GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
				);
		if (fd == INVALID_HANDLE_VALUE)
			return NULL;
#endif
		file = (SO_FILE *) malloc(sizeof(SO_FILE));
		(*file).fileDescriptor = fd;
		(*file).buff_len = -1;
		(*file).err_flag = 0;
		(*file).curr_pos = 0;
		(*file).buff_curr = 0;
		(*file).offset_pos = 0;
		strcpy((*file).mode, "w");
		strcpy((*file).filePath, pathname);
		file->prev_fnct = -1;
		file->last_pos_write = -1;
		return file;
	}

	if (strcmp(mode, "w+") == 0) {
#if defined(__linux__)
		fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC, 0644);
		if (fd < 0)
			return NULL;
#elif defined(_WIN32)
		fd = CreateFile(
				pathname,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
				);
		if (fd == INVALID_HANDLE_VALUE)
			return NULL;
#endif
		file = (SO_FILE *) malloc(sizeof(SO_FILE));
		(*file).fileDescriptor = fd;
		(*file).buff_len = -1;
		(*file).curr_pos = 0;
		(*file).err_flag = 0;
		(*file).buff_curr = 0;
		(*file).offset_pos = 0;
		strcpy((*file).filePath, pathname);
		strcpy((*file).mode, "w+");
		file->prev_fnct = -1;
		file->last_pos_write = -1;
		return file;
	}

	if (strcmp(mode, "a") == 0) {
#if defined(__linux__)
		fd = open(pathname, O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (fd < 0)
			return NULL;
#elif defined(_WIN32)
		fd = CreateFile(
				pathname,
				GENERIC_WRITE,
				FILE_APPEND_DATA,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
				);
		if (fd == INVALID_HANDLE_VALUE)
			return NULL;
#endif
		file = (SO_FILE *) malloc(sizeof(SO_FILE));
		(*file).fileDescriptor = fd;
		(*file).buff_len = -1;
		(*file).err_flag = 0;
		(*file).curr_pos = 0;
		(*file).buff_curr = 0;
		(*file).offset_pos = 0;
		strcpy((*file).mode, "a");
		strcpy((*file).filePath, pathname);
		file->prev_fnct = -1;
		file->last_pos_write = -1;
		return file;
	}

	if (strcmp(mode, "a+") == 0) {
#if defined(__linux__)
		fd = open(pathname, O_RDWR | O_CREAT | O_APPEND, 0644);
		if (fd < 0)
			return NULL;
#elif defined(_WIN32)
		fd = CreateFile(
				pathname,
				GENERIC_WRITE | GENERIC_READ,
				FILE_APPEND_DATA,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
				);
		if (fd == INVALID_HANDLE_VALUE)
			return NULL;
#endif
		file = (SO_FILE *) malloc(sizeof(SO_FILE));
		(*file).fileDescriptor = fd;
		(*file).buff_len = -1;
		(*file).err_flag = 0;
		(*file).curr_pos = 0;
		(*file).buff_curr = 0;
		(*file).offset_pos = 0;
		strcpy((*file).mode, "a+");
		strcpy((*file).filePath, pathname);
#if defined(__linux__)
		lseek((*file).fileDescriptor, 0, SEEK_SET);
#elif defined(_WIN32)
#endif
		file->prev_fnct = -1;
		file->last_pos_write = -1;
		return file;
	}

	return NULL;
}
FUNC_DECL_PREFIX  int
so_fflush(SO_FILE *stream)
{
	int rc;
#if defined(_WIN32)
	int written;
#endif

	if (stream == NULL) {
		stream->err_flag = SO_EOF;
		return SO_EOF;
	}
	if (stream->prev_fnct == 1) {
#if defined(__linux__)
		rc = write(stream->fileDescriptor, stream->buffer,
				stream->buff_curr);
		if (rc == -1 && rc != stream->buff_curr) {
			stream->err_flag = SO_EOF;
			return SO_EOF;
		}
#elif defined(_WIN32)
		rc = WriteFile(stream->fileDescriptor, stream->buffer,
				stream->buff_curr, &written, NULL);
		if (rc == 0 && written != stream->buff_curr) {
			stream->err_flag = SO_EOF;
			return SO_EOF;
		}
#endif
	}
	stream->prev_fnct = -2;
	stream->buff_curr = 0;
	stream->err_flag = 0;
	return 0;
}

FUNC_DECL_PREFIX  int
so_fputc(int c, SO_FILE *stream)
{
	int rc;
#if defined(_WIN32)
	int written;
#endif

	if (stream->buff_curr == sizeof(stream->buffer)) {
#if defined(__linux__)
		rc = write(stream->fileDescriptor, stream->buffer,
				sizeof(stream->buffer));
		if (rc == -1 && rc != sizeof(stream->buffer)) {
			stream->err_flag = SO_EOF;
			return SO_EOF;
		}
#elif defined(_WIN32)
		rc = WriteFile(stream->fileDescriptor, stream->buffer,
				stream->buff_curr, &written, NULL);
		if (rc == 0 && written != sizeof(stream->buffer)) {
			stream->err_flag = SO_EOF;
			return SO_EOF;
		}
#endif
		stream->buff_curr = 0;
		stream->offset_pos = 0;
		stream->err_flag = 0;
	}
	stream->buffer[stream->buff_curr++] = c;
	stream->offset_pos = stream->buff_curr;
	stream->err_flag = 0;
	stream->prev_fnct = 1;
#if defined(__linux__)
	stream->last_pos_write = lseek(stream->fileDescriptor, 0, SEEK_CUR);
#elif defined(_WIN32)
	stream->last_pos_write = SetFilePointer(stream->fileDescriptor, 0,
			NULL, FILE_CURRENT);
#endif
	return c;
}

FUNC_DECL_PREFIX  int
so_fclose(SO_FILE *stream)
{
	int rc;
	int test;

	if (stream == NULL) {
		stream->err_flag = SO_EOF;
		return SO_EOF;
	}
	test = so_fflush(stream);
#if defined(__linux__)
	rc = close((*stream).fileDescriptor);
#elif defined(_WIN32)
	rc = CloseHandle((*stream).fileDescriptor);
#endif
	if (rc < 0 || test == SO_EOF) {
		stream->err_flag = SO_EOF;
		free(stream);
		return SO_EOF;
	}
	free(stream);
	return 0;
}

FUNC_DECL_PREFIX  int
so_fgetc(SO_FILE *stream)
{
	unsigned char c;
	int rc;
	int old_pos;
	int i;
	int test;
#if defined(_WIN32)
	int test_test;
#endif

	if (stream == NULL)
		return SO_EOF;

	if (stream->curr_pos == DEFAULT_BUF_SIZE || stream->curr_pos == 0) {
		stream->curr_pos = 0;
		old_pos = so_ftell(stream);
		#if defined(__linux__)
		rc =  read(stream->fileDescriptor, stream->buffer,
				sizeof(stream->buffer));
		if (rc  == -1) {
			stream->err_flag = SO_EOF;
			return SO_EOF;
		}
#elif defined(_WIN32)
		rc = ReadFile(stream->fileDescriptor, stream->buffer,
				DEFAULT_BUF_SIZE, &test_test, NULL);
		if (rc  == 0) {
			stream->err_flag = SO_EOF;
			return SO_EOF;
		}
#endif
#if defined(__linux__)
		lseek(stream->fileDescriptor, old_pos, SEEK_SET);
#elif defined(_WIN32)
		SetFilePointer(stream->fileDescriptor, old_pos, NULL,
				FILE_BEGIN);
#endif
	}
	if (so_feof(stream) != 0) {
		stream->err_flag = SO_EOF;
		return SO_EOF;
	}

	c = (unsigned char) stream->buffer[stream->curr_pos++];
#if defined(__linux__)
		test = lseek(stream->fileDescriptor, 1, SEEK_CUR);
#elif defined(_WIN32)
		test = SetFilePointer(stream->fileDescriptor, 1, NULL,
				FILE_CURRENT);
#endif
	if (so_feof(stream) != 0) {
		stream->err_flag = SO_EOF;
		return SO_EOF;
	}
	stream->prev_fnct = 0;
	stream->err_flag = 0;
	return c;
}

FUNC_DECL_PREFIX  size_t
so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int i = 0, j;
	unsigned char aux;
	int t;
#if defined(_WIN32)
	int readed;
#endif
	unsigned char *test = ptr;

	if (ptr == NULL || stream == NULL) {
		return 0;
		stream->err_flag = SO_EOF;
	}

	for (i = 0; i < nmemb * size; i++) {
		aux = so_fgetc(stream);
		if (stream->err_flag == SO_EOF) {
#if defined(__linux__)
			t = lseek(stream->fileDescriptor, 0, SEEK_CUR);
#elif defined(_WIN32)
			t = SetFilePointer(stream->fileDescriptor, 0, NULL,
					FILE_CURRENT);
#endif
			if (t < nmemb*size) {
#if defined(__linux__)
				read(stream->fileDescriptor, stream->buffer,
						0);
#elif defined(_WIN32)
				ReadFile(stream->fileDescriptor,
						stream->buffer, 0, &readed,
						NULL);
#endif
			}
			stream->err_flag = SO_EOF;
			return i;
		}
		*(test + i) = aux;
	}
	stream->err_flag = 0;
	return i/size;
}

FUNC_DECL_PREFIX  size_t
so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int i;
	char aux;
	unsigned char *test = ptr;

	if (ptr == NULL || stream == NULL)
		return 0;
	if (strcmp(stream->mode, "a") == 0) {
#if defined(__linux__)
		lseek(stream->fileDescriptor, 0, SEEK_END);
#elif defined(_WIN32)
		SetFilePointer(stream->fileDescriptor, 0, NULL, FILE_END);
#endif
	}
	if (strcmp(stream->mode, "a+") == 0) {
#if defined(__linux__)
		lseek(stream->fileDescriptor, 0, SEEK_END);
#elif defined(_WIN32)
		SetFilePointer(stream->fileDescriptor, 0, NULL, FILE_END);
#endif
	}
	for (i = 0; i < nmemb * size; i++) {
		aux = so_fputc(*(test + i), stream);
		if (stream->err_flag == SO_EOF)
			return SO_EOF;
	}
	stream->err_flag = 0;
	return i/size;
}

FUNC_DECL_PREFIX  SO_FILE
*so_popen(const char *command, const char *type)
{
	return NULL;
}
FUNC_DECL_PREFIX  int
so_pclose(SO_FILE *stream)
{
	return NULL;
}
