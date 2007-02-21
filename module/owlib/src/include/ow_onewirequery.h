/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

*/

// encapsulation or parameters for read, write and directory

#ifndef OW_ONEWIREQUERY_H			/* tedious wrapper */
#define OW_ONEWIREQUERY_H

struct owq_memory {
	void * mem ;
	size_t size ;
}  ;

union value_object {
	int          I ;
	unsigned int U ;
	_FLOAT       F ;
	_DATE       D ;
    int	         Y ;//boolean
	struct owq_memory M ;
	union value_object * array ;
} ;

struct one_wire_query {
	size_t size ;
	off_t offset ;
	struct parsedname pn ;
	union value_object val ;
} ;

#define OWQ_pn(owq)		((owq)->pn)
#define OWQ_size(owq)	((owq)->size)
#define OWQ_offset(owq)	((owq)->offset)
#define OWQ_val(owq)	((owq)->val)
#define OWQ_array(owq)	(((owq)->val).array)

#define OWQ_I(owq)		(OWQ_val(owq).I

#endif							/* OW_ONEWIREQUERY_H */
