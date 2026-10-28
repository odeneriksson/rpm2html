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

int create_gz_source(char *filename) {
    char outputfile[MAX_PATH_LENGHT];
    Bytef *input = NULL;
    int size;
    Bytef *output = NULL;
    uLongf outsize;
    int res;
    struct stat st;
    int fd = -1;
    FILE *result = NULL;
    int status = 1;
    const char *output_path = data_file;
    int i;
    
    /*
     * open and read the input file in a buffer.
     */
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
	    perror("mkdep: open");
	    goto cleanup;
    }
    if (fstat(fd, &st) < 0) {
        perror("fstat failed");
        goto cleanup;
    }
    size = st.st_size;
    input = malloc(size);
    if (input == NULL) {
	perror("malloc failed");
	goto cleanup;
    }
    res = read(fd, input, size);
    if (res < size) {
	perror("read didn't complete");
	goto cleanup;
    }
    if (close(fd) < 0) {
        perror("close failed");
        fd = -1;
        goto cleanup;
    }
    fd = -1;

    /*
     * allocate an output buffer for the compressed result.
     */
    outsize = size + (size / 8) + 12;
    output = malloc(outsize);
    if (output == NULL) {
	perror("malloc failed");
	goto cleanup;
    }

    /*
     * compress the whole stuff in one pass.
     */
    if (compress(output, (uLongf *) &outsize, input, size) != Z_OK) {
        fprintf(stderr, "compress failed\n");
	goto cleanup;
    }

    /*
     * save the whole stuff as a C source file.
     */
    if (output_path == NULL) {
        int len = snprintf(outputfile, sizeof(outputfile), "%s.h", filename);
        if (len < 0 || (size_t) len >= sizeof(outputfile)) {
            fprintf(stderr, "output header filename too long: %s\n", filename);
            goto cleanup;
        }
        output_path = outputfile;
    }
    result = fopen(output_path, "w");
    if (result == NULL) {
        fprintf(stderr, "fopen(%s)", output_path);
	perror("failed");
	goto cleanup;
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

    if (ferror(result)) {
        fprintf(stderr, "write failed: %s\n", output_path);
        goto cleanup;
    }
    if (fclose(result) != 0) {
        result = NULL;
        perror("fclose failed");
        goto cleanup;
    }
    result = NULL;
    status = 0;

cleanup:
    if (result != NULL)
        fclose(result);
    if (fd >= 0)
        close(fd);
    free(output);
    free(input);
    return status;
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
    if (res == NULL)
        return NULL;
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
                char *generated_name = NULL;
                int path_len;

                if (name[0] == '-') {
		    switch (name[1]) {
		        case 'n':
			    if (argc < 2) {
			        fprintf(stderr, "missing data name after -n\n");
			        return 1;
			    }
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
                   path_len = snprintf(fullname, sizeof(fullname),
                                       "%s/%s", pwd, name);
		} else {
                   path_len = snprintf(fullname, sizeof(fullname),
                                       "%s", name);
		}
                if (path_len < 0 || (size_t) path_len >= sizeof(fullname)) {
                    fprintf(stderr, "input filename too long: %s\n", name);
                    return 1;
                }

		/*
		 * do the work !
		 */
		if (data_name == NULL) {
		    generated_name = get_data_name(name);
                    if (generated_name == NULL) {
                        perror("malloc failed");
                        return 1;
                    }
		    data_name = generated_name;
		}
		if (create_gz_source(fullname) != 0) {
                    free(generated_name);
                    return 1;
                }
		free(generated_name);
		/* Apply an explicit -n name only to the next input file. */
		data_name = NULL;

	}
	return 0;
}

