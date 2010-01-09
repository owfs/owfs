/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_specialcase.h"

/* Special cases -- a particular bus master has a special way to work with a specific slave */
/* Make a table of bus-master / slave / property /method(function) */
/* The key will be 
 * adapter-type (from ow_connection.h)
 * family code
 * filetype pointer
 * */
/* The methods will be a pair of read/write functions */

static void * SpecialCase_look( struct one_wire_query * owq ) ;
static void free_node(void *nodep) ;
static int specialcase_compare(const void *a, const void *b) ;

void * SpecialCase = NULL ;

static void free_node(void *nodep)
{
	(void) nodep;
	/* nothing to free */
	return;
}

void SpecialCase_close(void)
{
	tdestroy(SpecialCase, free_node);
}


static int specialcase_compare(const void *a, const void *b)
{
	return memcmp(a, b, sizeof(struct specialcase_key));
}

void SpecialCase_add( struct connection_in * in, unsigned char family_code, const char * property, int (*read_func) (struct one_wire_query *), int (*write_func) (struct one_wire_query *))
{
	/* Search for known 1-wire device */
	struct parsedname pn ; // carrier for finding device -- not actually formally created. No deallocation needed.
	pn.type = ePN_real ;
	FS_devicefindhex( family_code, &pn);
	if ( pn.selected_device != &NoDevice ) {
		/* Search for known device filetype */
		enum parse_enum pe = parse_error ;
		char * filename = owstrdup(property) ;
		if ( filename != NULL ) {
			pe = Parse_Property(filename, &pn) ;
			owfree(filename) ;
		}
		if (pe == parse_done ) {
			/* Create the item */
			struct specialcase * sc = owmalloc( sizeof(struct specialcase) ) ;
			if ( sc != NULL ) {
				sc->key.adapter = in->Adapter ;
				sc->key.family_code = family_code ;
				sc->key.filetype = pn.selected_filetype ;
				sc->key.extension = pn.extension ;
				sc->read = read_func ;
				sc->write = write_func ;
				// Add into the red/black tree
				tsearch(sc, &SpecialCase, specialcase_compare);
			} else {
				LEVEL_DEBUG("Attempt to add special handling ran out of memory. Family type %s. Property %s\n",family_code,property) ;
			}
		} else {
			LEVEL_DEBUG("Attempt to add special handling for unrecognized file type. Family type %s. Property %s\n",family_code,property) ;
		}
	} else {
		LEVEL_DEBUG("Attempt to add special handling for unrecognized family type %s. Property %s\n",family_code,property) ;
	}
	return ;	
}

static void * SpecialCase_look( struct one_wire_query * owq )
{
	struct specialcase sc ;
	struct parsedname * pn = PN(owq) ;
	sc.key.adapter = pn->selected_connection->Adapter ;
	sc.key.family_code = pn->sn[0] ;
	sc.key.filetype =  pn->selected_filetype ;
	sc.key.extension = pn->extension ;

	// find in the red/black tree
	return tfind(&sc, &SpecialCase, specialcase_compare);
}

int SpecialCase_read( struct one_wire_query * owq )
{
	struct specialcase * sc = SpecialCase_look(owq) ;
	
	if ( sc == NULL ) {
		return -ENOENT ;
	}
	if ( sc->read == NO_READ_FUNCTION ) {
		return -ENOENT ;
	}
	return (sc->read)(owq) ;
}

int SpecialCase_write( struct one_wire_query * owq )
{
	struct specialcase * sc = SpecialCase_look(owq) ;
	
	if ( sc == NULL ) {
		return -ENOENT ;
	}
	if ( sc->read == NO_WRITE_FUNCTION ) {
		return -ENOENT ;
	}
	return (sc->write)(owq) ;
}
