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
    char * buffer ;
	size_t size ;
	off_t offset ;
	struct parsedname pn ;
	union value_object val ;
} ;

#define OWQ_pn(owq)		((owq)->pn)
#define OWQ_buffer(owq) ((owq)->buffer)
#define OWQ_size(owq)   ((owq)->size)
#define OWQ_offset(owq)	((owq)->offset)
#define OWQ_val(owq)	((owq)->val)
#define OWQ_array(owq)	(((owq)->val).array)

#define OWQ_I(owq)      ((OWQ_val(owq)).I)
#define OWQ_U(owq)      ((OWQ_val(owq)).U)
#define OWQ_F(owq)      ((OWQ_val(owq)).F)
#define OWQ_D(owq)      ((OWQ_val(owq)).D)
#define OWQ_Y(owq)      ((OWQ_val(owq)).Y)
#define OWQ_mem(owq)    ((OWQ_val(owq)).M.mem)
#define OWQ_length(owq) ((OWQ_val(owq)).M.size)

#define OWQ_array_I(owq,i)      ((OWQ_array(owq)[i]).I)
#define OWQ_array_U(owq,i)      ((OWQ_array(owq)[i]).U)
#define OWQ_array_F(owq,i)      ((OWQ_array(owq)[i]).F)
#define OWQ_array_D(owq,i)      ((OWQ_array(owq)[i]).D)
#define OWQ_array_Y(owq,i)      ((OWQ_array(owq)[i]).Y)
#define OWQ_array_mem(owq,i)    ((OWQ_array(owq)[i]).M.mem)
#define OWQ_array_length(owq,i) ((OWQ_array(owq)[i]).M.size)

int FS_input_owq( struct one_wire_query * owq) ;
int FS_output_owq( struct one_wire_query * owq) ;

#endif							/* OW_ONEWIREQUERY_H */
