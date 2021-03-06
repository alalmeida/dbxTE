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
 * 

00h /   0| Version number      *1|  ^
         |-----------------------|  |
01h /   1| Date of last update   |  |
02h /   2|      YYMMDD        *21|  |
03h /   3|                    *14|  |
         |-----------------------|  |
04h /   4| Number of records     | Record
05h /   5| in data file          | header
06h /   6| ( 32 bits )        *14|  |
07h /   7|                       |  |
         |-----------------------|  |
08h /   8| Length of header   *14|  |
09h /   9| structure ( 16 bits ) |  |
         |-----------------------|  |
0Ah /  10| Length of each record |  |
0Bh /  11| ( 16 bits )     *2 *14|  |
         |-----------------------|  |
0Ch /  12| ( Reserved )        *3|  |
0Dh /  13|                       |  |
         |-----------------------|  |
0Eh /  14| Incomplete transac.*12|  |
         |-----------------------|  |
0Fh /  15| Encryption flag    *13|  |
         |-----------------------|  |
10h /  16| Free record thread    |  |
11h /  17| (reserved for LAN     |  |
12h /  18|  only )               |  |
13h /  19|                       |  |
         |-----------------------|  |
14h /  20| ( Reserved for        |  |            _        |=======================| ______
         |   multi-user dBASE )  |  |           / 00h /  0| Field name in ASCII   |  ^
         : ( dBASE III+ - )      :  |          /          : (terminated by 00h)   :  |
         :                       :  |         |           |                       |  |
1Bh /  27|                       |  |         |   0Ah / 10|                       |  |
         |-----------------------|  |         |           |-----------------------| For
1Ch /  28| MDX flag (dBASE IV)*14|  |         |   0Bh / 11| Field type (ASCII) *20| each
         |-----------------------|  |         |           |-----------------------| field
1Dh /  29| Language driver     *5|  |        /    0Ch / 12| Field data address    |  |
         |-----------------------|  |       /             |                     *6|  |
1Eh /  30| ( Reserved )          |  |      /              | (in memory !!!)       |  |
1Fh /  31|                     *3|  |     /       0Fh / 15| (dBASE III+)          |  |
         |=======================|__|____/                |-----------------------|  | <-
20h /  32|                       |  |  ^          10h / 16| Field length       *22|  |   |
         |- - - - - - - - - - - -|  |  |                  |-----------------------|  |   | *7
         |                    *19|  |  |          11h / 17| Decimal count      *23|  |   |
         |- - - - - - - - - - - -|  |  Field              |-----------------------|  | <-
         |                       |  | Descriptor  12h / 18| ( Reserved for        |  |
         :. . . . . . . . . . . .:  |  |array     13h / 19|   multi-user dBASE)*18|  |
         :                       :  |  |                  |-----------------------|  |
      n  |                       |__|__v_         14h / 20| Work area ID       *16|  |
         |-----------------------|  |    \                |-----------------------|  |
      n+1| Terminator (0Dh)      |  |     \       15h / 21| ( Reserved for        |  |
         |=======================|  |      \      16h / 22|   multi-user dBASE )  |  |
      m  | Database Container    |  |       \             |-----------------------|  |
         :                    *15:  |        \    17h / 23| Flag for SET FIELDS   |  |
         :                       :  |         |           |-----------------------|  |
    / m+263                      |  |         |   18h / 24| ( Reserved )          |  |
         |=======================|__v_ ___    |           :                       :  |
         :                       :    ^       |           :                       :  |
         :                       :    |       |           :                       :  |
         :                       :    |       |   1Eh / 30|                       |  |
         | Record structure      |    |       |           |-----------------------|  |
         |                       |    |        \  1Fh / 31| Index field flag    *8|  |
         |                       |    |         \_        |=======================| _v_____
         |                       | Records
         |-----------------------|    |
         |                       |    |          _        |=======================| _______
         |                       |    |         / 00h /  0| Record deleted flag *9|  ^
         |                       |    |        /          |-----------------------|  |
         |                       |    |       /           | Data               *10|  One
         |                       |    |      /            : (ASCII)            *17: record
         |                       |____|_____/             |                       |  |
         :                       :    |                   |                       | _v_____
         :                       :____|_____              |=======================|
         :                       :    |
         |                       |    |
         |                       |    |
         |                       |    |
         |                       |    |
         |                       |    |
         |=======================|    |
         |__End_of_File__________| ___v____  End of file ( 1Ah )  *11

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
int dbf_version;
char *dbf_version_s;
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
    
    dbf_version = hdr.version;
    no_fields = (hdr_len - sizeof(struct dbf_header))
		/ sizeof(struct field_descriptor);
    j = (hdr_len - sizeof(struct dbf_header))
	% sizeof(struct field_descriptor);

    for (i = 0; i < no_fields; i++) {
	n = read(fd, &fld[i], sizeof(struct field_descriptor));
    }
    read(fd, buf, j);
}

void print_header(char *file) {
    switch(dbf_version){
        case 2:
            dbf_version_s = "FoxBase";
            break;
        case 3:
            dbf_version_s = "dBase III sem campo memo";
            break;
        case 4:
            dbf_version_s = "dBase IV sem campo memo";
            break;
        case 5:
            dbf_version_s = "dBase V sem campo memo";
            break;
        default:
            dbf_version_s = "Não catalogado";  
    }
    printf("Arquivo xBase:          %s\n", file);
    printf("Versão do arquivo       %s\n", dbf_version_s);
    printf("Data do último:         %04d/%02d/%02d\n", 1900 + hdr.year, hdr.month, hdr.day);
    printf("Número de registros:    %d (deleted incluidos)\n", no_records);
    printf("Tamanho do registro:    %d\n", rec_len);
    printf("Número de Campos:       %d\n", no_fields);
}    

void print_info(char *file) {
    int i;

    printf("field definitions:\n");
    for (i = 0; i < no_fields; i++) {
	printf("\t%-*s  %c %d", FIELD_NAME_LENGTH, fld[i].name,
	    fld[i].type, fld[i].length);
	if (fld[i].decimals > 0) {
	    printf(".%d\n", fld[i].decimals);
	} else {
	    printf("\n");
	}
    }
    printf("\n");
}


void print_create(int fd, char *tname)
{
int i, j;

    printf("CREATE TABLE %s (\n", tname);
    for (i = 0; i < no_fields; i++) {
	if (i > 0) {
	    printf(",\n");
	}
	for (j = 0; j < FIELD_NAME_LENGTH; j++) {
	    fld[i].name[j] = (char) tolower(fld[i].name[j]);
	}
	printf("\t%-15s", fld[i].name);
	if (fld[i].type == 'C') {
	    printf("char(%d)", fld[i].length);
	} else if (fld[i].type == 'N') {
	    if (fld[i].decimals <= 0) {
		printf("int");
	    } else {
		printf("real");
	    }
	} else if (fld[i].type == 'L') {
	     printf("int");
	} else {
	    printf("unknown type");
	}
    }
    printf("\n)\n\n");
}


void print_fieldnames(int fd)
{
int i, j;

    for (i = 0; i < no_fields; i++) {
	if (i > 0) {
	    printf(",");
	}
	for (j = 0; j < FIELD_NAME_LENGTH; j++) {
	    fld[i].name[j] = (char) tolower(fld[i].name[j]);
	}
	printf("'%s'", fld[i].name);
    }
    printf("\r\n");
}


void print_field(char *buf, int length, char type)
{
int from, to;
int i;

    // trim spaces
    for (from = 0; buf[from] == ' ' && from < length; from++) ;
    for (to = length; buf[to - 1] == ' ' && to > 0; to--) ;

    switch (type) {
	case 'L':
	    if (toupper(buf[from]) == 'Y' || toupper(buf[from]) == 'T') {
		printf("1");
	    } else {
		printf("0");
	    }
	    break;
	case 'N':
	    if (from >= to) {
		printf("0");
	    } else {
		for (i = from; i < to; i++) {
		    printf("%c", buf[i]);
		}
	    }
	    break;
	case 'C':
	    printf("'");
	    for (i = from; i < to; i++) {
		if (buf[i] == '\'') {
		    buf[i] = '`';
		} else if (buf[i] == ',') {
		    buf[i] = ';';
		}
		printf("%c", buf[i]);
	    }
	    printf("'");
	    break;
	default:
	    Error("unknown field type", -1);
    }
}


void print_data(int fd, char *tname, int field_names)
{
int n, i, j;

    for (i = 0; i < no_records; i++) {
	n = read(fd, buf, rec_len);
	buf[rec_len] = '\0';
	if (buf[0] == DELETED) {
	    continue;
	}

	if (field_names) {
	    printf("INSERT INTO %s (", tname);
	    for (j = 0, n = 1; j < no_fields; j++) {
		if (j > 0) {
		    printf(",");
		}
		printf("%s", fld[j].name);
	    }
	    printf(")\n");
	} else {
	    printf("INSERT INTO %s", tname);
	}

	printf(" VALUES (");
	for (j = 0, n = 1; j < no_fields; j++) {
	    if (j > 0) {
		printf(",");
	    }
	    print_field(&(buf[n]), fld[j].length, fld[j].type);
	    n += fld[j].length;
	}
	printf(")\n");
    }
}


void print_msaccess_txt(int fd)
{
int n, i, j;

    for (i = 0; i < no_records; i++) {
	n = read(fd, buf, rec_len);
	buf[rec_len] = '\0';
	if (buf[0] == DELETED) {
	    continue;
	}

	for (j = 0, n = 1; j < no_fields; j++) {
	    if (j > 0) {
		printf(",");
	    }
	    print_field(&(buf[n]), fld[j].length, fld[j].type);
	    n += fld[j].length;
	}
	printf("\r\n");
    }
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

    if (header_flag) {
	print_header(fname);
	exit(0);
    }
    
}

