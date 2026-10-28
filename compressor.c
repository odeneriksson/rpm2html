/*
 * Compressor.c : read an input file and store it compressed in
 *                a C source file containing a char array containing
 *                the compressed file.
 *
 * See Copyright for the status of this software.
 *
 * $Id: compressor.c,v 1.5 1998/05/25 23:57:58 veillard Exp $
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>

#define MAX_PATH_LENGHT 512

char *data_name;
char *data_file = NULL;
int main_argc;
char **main_argv;
char pwd[MAX_PATH_LENGHT];

void create_gz_source(char *filename) {
    char outputfile[MAX_PATH_LENGHT];
    Bytef *input;
    int size;
    Bytef *output;
    uLongf outsize;
    int res;
    struct stat st;
    int fd;
    FILE *result;
    int i;
    
    /*
     * open and read the input file in a buffer.
     */
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
	    perror("mkdep: open");
	    return;
    }
    fstat(fd, &st);
    size = st.st_size;
    input = malloc(size);
    if (input == NULL) {
	perror("malloc failed");
	exit(1);
    }
    res = read(fd, input, size);
    if (res < size) {
	perror("read didn't complete");
	exit(1);
    }
    close(fd);

    /*
     * allocate an output buffer for the compressed result.
     */
    outsize = size + (size / 8) + 12;
    output = malloc(outsize);
    if (output == NULL) {
	perror("malloc failed");
	exit(1);
    }

    /*
     * compress the whole stuff in one pass.
     */
    if (compress(output, (uLongf *) &outsize, input, size) != Z_OK) {
        fprintf(stderr, "compress failed\n");
	exit(1);
    }

    /*
     * save the whole stuff as a C source file.
     */
    if (data_file == NULL) {
        sprintf(outputfile, "%s.h", filename);
	data_file = &outputfile[0];
    }
    result = fopen(data_file, "w");
    if (result == NULL) {
        fprintf(stderr, "fopen(%s)", data_file);
	perror("failed");
	exit(1);
    }
    fprintf(result, "/* output from compressor, need #include <zlib.h>\" */\n");
    fprintf(result, "#define %s_size %d\n", data_name, size);
    fprintf(result, "unsigned char %s[%d] = {", data_name, (int) outsize);
    for (i = 0;i < (outsize - 1);i++) {
        if (!(i % 15)) fprintf(result, "\n");
	fprintf(result, "0x%02x,", output[i]);
    }
    fprintf(result, "0x%02x\n};\n\n", output[outsize - 1]);
    fprintf(result, "char *read_%s(void) {\n", data_name);
    fprintf(result, "    uLongf size = %d;\n", size + 20);
    fprintf(result, "    Bytef *buffer = malloc(%d);\n\n", size + 20);
    fprintf(result, "    if (buffer == NULL) return(NULL);\n");
    fprintf(result, "    if (uncompress(buffer, &size, %s, %d) != Z_OK) {\n",
            data_name, (int) outsize);
    fprintf(result, "        fprintf(stderr, \"uncompress failed\");\n");
    fprintf(result, "        free(buffer);\n");
    fprintf(result, "        return(NULL);\n");
    fprintf(result, "    }\n");
    fprintf(result, "    if (size != %d) {\n", size);
    fprintf(result, "        fprintf(stderr, \"uncompress failed\");\n");
    fprintf(result, "        free(buffer);\n");
    fprintf(result, "        return(NULL);\n");
    fprintf(result, "    }\n");
    fprintf(result, "    buffer[%d] = '\\0';\n", size);
    /* The generated API returns char *, while zlib uses Bytef *. */
    fprintf(result, "    return((char *) buffer);\n}\n");

    fclose(result);
}

void message(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

char *get_data_name(char *filename) {
    char *res;
    char *cur;

    res = (char *) malloc((strlen(filename) + 20) * sizeof(char));
    sprintf(res, "data_buffer_%s", filename);
    cur = res;
    while (*cur != '\0') {
        if (*cur == '.') *cur = '_';
        else if (*cur == ' ') *cur = '_';
        else if (*cur == '/') *cur = '_';
        else if (*cur == '\t') *cur = '_';
	cur++;
    }
    return(res);
}

int main(int argc, char **argv)
{
	char fullname[MAX_PATH_LENGHT];

	main_argc = argc;
	main_argv = argv;
        if (getcwd(pwd, sizeof(pwd)) == NULL) {
            perror("getcwd");
            return 1;
        }

	while (--argc > 0) {
		char *name = *++argv;

                if (name[0] == '-') {
		    switch (name[1]) {
		        case 'n':
			    data_name = *++argv;
			    argc--;
			    continue;
		        case 'o':
			    data_file = *++argv;
			    argc--;
			    continue;
			    
			default:
			    /*
			     * NOTE : dump_h has to be modified accordingly
			     *   if new options are added !
			     */
			    fprintf(stderr,"%s [options]  includes ...\n",
			            argv[0]);
		            fprintf(stderr,"Options :\n");
		            fprintf(stderr,"\t-n data_name\n");
		            fprintf(stderr,"\t-o file_name\n");
		            exit(1);
		    }
		    continue;
		}
		if (name[0] != '/') {
		   strcpy(fullname, pwd);
                   strcat(fullname, "/");
                   strcat(fullname, name);
		} else {
		   strcpy(fullname, name);
		}

		/*
		 * do the work !
		 */
		data_name = get_data_name(name);
		create_gz_source(fullname);

	}
	return 0;
}

