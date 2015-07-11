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
 */

#include <stdio.h>
#include <stdlib.h>
#include "xbase.h"

struct dbf_header hdr;
struct field_descriptor fld[MAX_NO_FIELDS];
int no_fields;
int no_records;
short rec_len;
char buf[MAX_REC_LEN];
char usage[] = "usage: dbdump [-hcdfa] dbf_file table_name\nDump dBase III .dbf files into SQL commands. Records marked for\ndelete are left out from the dump.\n-h only the header of the .dbf file\n-c only the structure of the table (CREATE commands)\n-d only data records of the table (INSERT commands)\n-f list the field names in INSERT commands\n-a dump into text file suitable to import by MS Access\ndefault: structure and records without field names";


void Error(char *message, int error_code) {
    if (message != NULL) {
	fprintf(stderr, "%s\n", message);
    }
    if (error_code >= 0) {
	fprintf(stderr, "%s\n", strerror(error_code));
    }
    exit(1);
}


void Warning(char *message)
{
    fprintf(stderr, "%s\n", message);
    fflush(stderr);
}


int big_endian()
{
int x = 1;

    if(*(char *)&x == 1) {
	return 0;
    } else {
	return 1;
    }
}
/*
 * 
 */
int main(int argc, char** argv) {

    return (EXIT_SUCCESS);
}

