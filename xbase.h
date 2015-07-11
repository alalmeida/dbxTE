/* 
 * File:   xbase.h
 * Author: Andre Almeida
 *
 * Created on July 11, 2015, 6:49 PM
 * 
 * Classe derivada de:
 * 
 */

#ifndef XBASE_H
#define	XBASE_H

#ifdef	__cplusplus
extern "C" {
#endif


#define	TERMINATOR	'\r'
#define	LIVE		' '
#define	DELETED		'*'
#define MAX_REC_LEN	2048
#define MAX_NO_FIELDS	256

// .DBF header
struct dbf_header {
    char	version;	// dBASE version number
    char	year;
    char	month;
    char	day;
    int		no_records;	// number of records in file
    short	hdr_len;	// length of header
    short	rec_len;	// number of bytes in record
    char	misc[20];	// reserved bytes
};


// field descriptor
#define FIELD_NAME_LENGTH	11
struct field_descriptor {
    char	name[FIELD_NAME_LENGTH];
    char	type;
    int		address;
    char	length;
    char	decimals;
    char	misc[14];
};


#ifdef	__cplusplus
}
#endif

#endif	/* XBASE_H */

