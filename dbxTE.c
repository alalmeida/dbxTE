/* 
 * File:   dbxTE.c
 * Author: Andre Almeida
 *
 * Created on July 11, 2015, 6:48 PM
 * 
 * usage: dbdump [-hcdfa] dbf_file table_name

    Dump dBase III .dbf files into SQL commands. Records marked for
    delete are left out from the dump.

	-h only the header of the .dbf file
	-c only the structure of the table (CREATE commands)
	-d only data records of the table (INSERT commands)
	-f list the field names in INSERT commands
	-a dump into text file suitable to import by MS Access
	default: structure and records without field names

 * 
 * https://msdn.microsoft.com/en-us/library/aa975386(v=vs.71).aspx
 * https://en.wikipedia.org/wiki/.dbf
 * http://www.dbase.com/Knowledgebase/INT/db7_file_fmt.htm
 * http://www.clicketyclick.dk/databases/xbase/format/dbf.html
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "xbase.h"

struct dbf_header hdr;
struct field_descriptor fld[MAX_NO_FIELDS];
int no_fields;
int no_records;
short rec_len;
char buf[MAX_REC_LEN];
char usage[] = "usage: dbdump [-hcdfa] dbf_file table_name\nDump dBase III .dbf files into SQL commands. Records marked for\ndelete are left out from the dump.\n-h only the header of the .dbf file\n-c only the structure of the table (CREATE commands)\n-d only data records of the table (INSERT commands)\n-f list the field names in INSERT commands\n-a dump into text file suitable to import by MS Access\ndefault: structure and records without field names";

/*
 * Exibe a mensagem de erro e aborta o programa com o errorlevel 1
 */
void Error(char *message, int error_code) {
    if (message != NULL) {
	fprintf(stderr, "%s\n", message);
    }
    if (error_code >= 0) {
	fprintf(stderr, "%s\n", strerror(error_code));
    }
    exit(1);
}

/*
 * Exibe a mensagem de alerta
 */
void Warning(char *message) {
    fprintf(stderr, "%s\n", message);
    fflush(stderr);
}

/*
 * Verifica o formato de montagem da mensagem
 * https://pt.wikipedia.org/wiki/Extremidade_(ordena%C3%A7%C3%A3o)
 */
int big_endian() {
int x = 1;

    if(*(char *)&x == 1) {
	return 0;
    } else {
	return 1;
    }
}

/*
 * Leitura do header do arquivo dbf
 * 
 */
void get_header(int fd)
{
int n, i, j;
unsigned char *byte;
unsigned char tmp[4];
int hdr_len;

    n = read(fd, &hdr, sizeof(struct dbf_header));

    if (big_endian()) {
	byte = (unsigned char *) &(hdr.no_records);
	tmp[0] = byte[3];
	tmp[1] = byte[2];
	tmp[2] = byte[1];
	tmp[3] = byte[0];
	no_records = *((int *) tmp);
	byte = (unsigned char *) &(hdr.hdr_len);
	tmp[0] = byte[1];
	tmp[1] = byte[0];
	tmp[2] = tmp[3] = 0;
	hdr_len = *((short *) tmp);
	byte = (unsigned char *) &(hdr.rec_len);
	tmp[0] = byte[1];
	tmp[1] = byte[0];
	tmp[2] = tmp[3] = 0;
	rec_len = *((short *) tmp);
    } else {
	no_records = hdr.no_records;
	hdr_len = hdr.hdr_len;
	rec_len = hdr.rec_len;
    }

    no_fields = (hdr_len - sizeof(struct dbf_header))
		/ sizeof(struct field_descriptor);
    j = (hdr_len - sizeof(struct dbf_header))
	% sizeof(struct field_descriptor);

    for (i = 0; i < no_fields; i++) {
	n = read(fd, &fld[i], sizeof(struct field_descriptor));
    }
    read(fd, buf, j);
}

/*
 * 
 */
void main(int argc, char *argv[]) {
    int c;
    extern char *optarg;
    extern int optind;
    int header_flag = 0;
    int create_flag = 1;
    int data_flag = 1;
    int field_names = 0;
    int msaccess_flag = 0;
    int error_flag = 0;
    char *fname = NULL;
    char *tname = NULL;
    int fd = 0;
    
    printf("xBase Tools Exporter v0.1\n");
    printf("Andre Luiz de Almeida - alalmeida@gmail.com\n");
    while ((c = getopt(argc, argv, "hcdfa")) != EOF) {
	switch (c) {
	    case 'h':
		header_flag = 1;
		break;
	    case 'c':
		data_flag = 0;
		break;
	    case 'd':
		create_flag = 0;
		break;
	    case 'f':
		field_names = 1;
		break;
	    case 'a':
		msaccess_flag = 1;
		create_flag = 0;
		data_flag = 0;
		break;
	    case '?':
		error_flag++;
		break;
	}
    }
    if (error_flag || argc < 2) {
	Error(usage, -1);
    }
    if (optind == argc) {
	Warning("Arquivo de entrada não fornecido.");
	Error(usage, -1);
    } else {
	fname = argv[optind++];
    }
    if (optind == argc && !header_flag && !msaccess_flag) {
	Warning("Faltando o nome da tabela.");
	Error(usage, -1);
    } else {
	tname = argv[optind];
    }
    if ((fd = open(fname, O_RDONLY)) < 0) {
	Error("Não foi possivel abrir o arquivo!", errno);
    }
    
    get_header(fd);

    
}

