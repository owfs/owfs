/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

*/

// encapsulation or parameters for read, write and directory

#ifndef OW_ONEWIREQUERY_H		/* tedious wrapper */
#define OW_ONEWIREQUERY_H

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
	size_t size;
	off_t offset;
	struct parsedname pn;
	union value_object val;
};

#define OWQ_pn(owq)		((owq)->pn)
#define OWQ_buffer(owq) ((owq)->buffer)
#define OWQ_size(owq)   ((owq)->size)
#define OWQ_offset(owq)	((owq)->offset)
#define OWQ_val(owq)	((owq)->val)
#define OWQ_array(owq)	(((owq)->val).array)
#define PN(owq)         (&OWQ_pn(owq))

#define OWQ_I(owq)      ((OWQ_val(owq)).I)
#define OWQ_U(owq)      ((OWQ_val(owq)).U)
#define OWQ_F(owq)      ((OWQ_val(owq)).F)
#define OWQ_D(owq)      ((OWQ_val(owq)).D)
#define OWQ_Y(owq)      ((OWQ_val(owq)).Y)
#define OWQ_length(owq) ((OWQ_val(owq)).length)

#define OWQ_array_I(owq,i)      ((OWQ_array(owq)[i]).I)
#define OWQ_array_U(owq,i)      ((OWQ_array(owq)[i]).U)
#define OWQ_array_F(owq,i)      ((OWQ_array(owq)[i]).F)
#define OWQ_array_D(owq,i)      ((OWQ_array(owq)[i]).D)
#define OWQ_array_Y(owq,i)      ((OWQ_array(owq)[i]).Y)
#define OWQ_array_length(owq,i) ((OWQ_array(owq)[i]).length)

#define OWQ_explode(owq)	(BYTE *)OWQ_buffer(owq),OWQ_size(owq),OWQ_offset(owq),PN(owq)

//#define OWQ_allocate_struct_and_pointer( owq_name )	struct one_wire_query struct_##owq_name ; struct one_wire_query * owq_name = & struct_##owq_name
// perhaps it would be nice to clear the memory try trace errors. OWQ_allocate_struct_and_pointer() needs to be defined as the last local variable now...
#define OWQ_allocate_struct_and_pointer( owq_name )	struct one_wire_query struct_##owq_name ; struct one_wire_query * owq_name = & struct_##owq_name; if (Globals.error_level>=e_err_debug) { memset(&struct_##owq_name, 0, sizeof(struct one_wire_query)); }

int FS_OWQ_create(const char *path, char *buffer, size_t size, off_t offset, struct one_wire_query *owq);
int FS_OWQ_create_plus(const char *path, const char *file, char *buffer, size_t size, off_t offset, struct one_wire_query *owq);
void FS_OWQ_destroy(struct one_wire_query *owq);

struct one_wire_query *FS_OWQ_create_sibling(const char *sibling, struct one_wire_query *owq_original) ;
void FS_OWQ_destroy_sibling(struct one_wire_query *owq) ;

void OWQ_create_shallow_single(struct one_wire_query *owq_shallow, struct one_wire_query *owq_original);
void OWQ_create_shallow_bitfield(struct one_wire_query *owq_shallow, struct one_wire_query *owq_original);
int OWQ_create_shallow_aggregate(struct one_wire_query *owq_shallow, struct one_wire_query *owq_original);
void OWQ_destroy_shallow_aggregate(struct one_wire_query *owq_shallow);
void OWQ_create_temporary(struct one_wire_query *owq_temporary, char *buffer, size_t size, off_t offset, struct parsedname *pn);

int Fowq_output_offset_and_size(char *string, size_t length, struct one_wire_query *owq);
int Fowq_output_offset_and_size_z(char *string, struct one_wire_query *owq);

struct one_wire_query *FS_OWQ_from_pn(const struct parsedname *pn);

int FS_input_owq(struct one_wire_query *owq);
int FS_output_owq(struct one_wire_query *owq);
void _print_owq(struct one_wire_query *owq);

#endif							/* OW_ONEWIREQUERY_H */
