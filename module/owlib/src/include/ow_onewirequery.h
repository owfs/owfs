/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

*/

// encapsulation or parameters for read, write and directory

#ifndef OW_ONEWIREQUERY_H		/* tedious wrapper */
#define OW_ONEWIREQUERY_H

enum owq_cleanup {
	owq_cleanup_none    = 0,
	owq_cleanup_owq     = 0x01,
	owq_cleanup_pn      = 0x02,
	owq_cleanup_buffer  = 0x04,
	owq_cleanup_rbuffer = 0x08,
	owq_cleanup_array   = 0x10,

	// unrelated flag
	owq_simultaneous    = 0x1000,
	} ;

union value_object {
	int I;
	unsigned int U;
	_FLOAT F;
	_DATE D;
	int Y;						//boolean
	size_t length;
	union value_object *array;
};

struct one_wire_query {
	char *buffer;
	char *read_buffer;
	const char *write_buffer;
	size_t size;
	off_t offset;
	struct parsedname pn;
	enum owq_cleanup cleanup;
	union value_object val;
};

#define OWQ_pn(owq)	      ((owq)->pn)
#define OWQ_buffer(owq)       ((owq)->buffer)
#define OWQ_read_buffer(owq)  ((owq)->read_buffer)
#define OWQ_write_buffer(owq) ((owq)->write_buffer)
#define OWQ_size(owq)         ((owq)->size)
#define OWQ_offset(owq)	      ((owq)->offset)
#define OWQ_cleanup(owq)      ((owq)->cleanup)
#define OWQ_val(owq)	      ((owq)->val)
#define OWQ_array(owq)	      (((owq)->val).array)
#define PN(owq)               (&OWQ_pn(owq))

#define OWQ_I(owq)            ((OWQ_val(owq)).I)
#define OWQ_U(owq)            ((OWQ_val(owq)).U)
#define OWQ_F(owq)            ((OWQ_val(owq)).F)
#define OWQ_D(owq)            ((OWQ_val(owq)).D)
#define OWQ_Y(owq)            ((OWQ_val(owq)).Y)
#define OWQ_length(owq)       ((OWQ_val(owq)).length)

#define OWQ_array_I(owq,i)    ((OWQ_array(owq)[i]).I)
#define OWQ_array_U(owq,i)    ((OWQ_array(owq)[i]).U)
#define OWQ_array_F(owq,i)    ((OWQ_array(owq)[i]).F)
#define OWQ_array_D(owq,i)    ((OWQ_array(owq)[i]).D)
#define OWQ_array_Y(owq,i)    ((OWQ_array(owq)[i]).Y)
#define OWQ_array_length(owq,i) ((OWQ_array(owq)[i]).length)

#define OWQ_explode(owq)	(BYTE *)OWQ_buffer(owq),OWQ_size(owq),OWQ_offset(owq),PN(owq)

#define OWQ_SIMUL_SET(owq)    (((owq)->cleanup) |= owq_simultaneous )
#define OWQ_SIMUL_CLR(owq)    (((owq)->cleanup) &= (~owq_simultaneous) )
#define OWQ_SIMUL_TEST(owq)   ((((owq)->cleanup) & owq_simultaneous) != 0 )

//#define OWQ_allocate_struct_and_pointer( owq_name )	struct one_wire_query struct_##owq_name ; struct one_wire_query * owq_name = & struct_##owq_name
// perhaps it would be nice to clear the memory try trace errors. OWQ_allocate_struct_and_pointer() needs to be defined as the last local variable now...

// "owq_name->cleanup = owq_cleanup_none" is needed at least... but why not clear the whole struct just to make sure it never happens again.
#define OWQ_allocate_struct_and_pointer( owq_name )	struct one_wire_query struct_##owq_name ; struct one_wire_query * owq_name = & struct_##owq_name; memset(&struct_##owq_name, 0, sizeof(struct one_wire_query));

GOOD_OR_BAD OWQ_create(const char *path, struct one_wire_query *owq);
GOOD_OR_BAD OWQ_create_plus(const char *path, const char *file, struct one_wire_query *owq);

void OWQ_destroy(struct one_wire_query *owq);

struct one_wire_query * OWQ_create_from_path(const char *path) ;
struct one_wire_query * OWQ_create_sibling(const char *sibling, struct one_wire_query *owq_original) ;

GOOD_OR_BAD OWQ_allocate_read_buffer(struct one_wire_query * owq ) ;
GOOD_OR_BAD OWQ_allocate_write_buffer( const char * write_buffer, size_t buffer_length, off_t offset, struct one_wire_query * owq ) ;

void OWQ_assign_read_buffer(char *buffer, size_t size, off_t offset, struct one_wire_query *owq) ;
void OWQ_assign_write_buffer(const char *buffer, size_t size, off_t offset, struct one_wire_query *owq) ;

struct one_wire_query * OWQ_create_separate( int extension, struct one_wire_query * owq_aggregate ) ;
struct one_wire_query * OWQ_create_aggregate( struct one_wire_query * owq_single );

void OWQ_create_shallow_single(struct one_wire_query *owq_shallow, struct one_wire_query *owq_original);
void OWQ_create_shallow_bitfield(struct one_wire_query *owq_shallow, struct one_wire_query *owq_original);
int OWQ_create_shallow_aggregate(struct one_wire_query *owq_shallow, struct one_wire_query *owq_original);
void OWQ_destroy_shallow_aggregate(struct one_wire_query *owq_shallow);
void OWQ_create_temporary(struct one_wire_query *owq_temporary, char *buffer, size_t size, off_t offset, struct parsedname *pn);

ZERO_OR_ERROR OWQ_format_output_offset_and_size(const char *string, size_t length, struct one_wire_query *owq);
ZERO_OR_ERROR OWQ_format_output_offset_and_size_z(const char *string, struct one_wire_query *owq);

struct one_wire_query * ALLtoBYTE( struct one_wire_query *owq_all  );
struct one_wire_query * BYTEtoALL( struct one_wire_query *owq_byte );

int OWQ_parse_input(struct one_wire_query *owq);
SIZE_OR_ERROR OWQ_parse_output(struct one_wire_query *owq);
void _print_owq(struct one_wire_query *owq);

#endif							/* OW_ONEWIREQUERY_H */
