#include <ruby.h>
#include <CNearTree.h>
#include <limits.h>
#include <stdbool.h>

VALUE neartree_module;
VALUE neartree_class;

typedef struct {
	int              dimension;   /* dimensionality of the space            */
//	bool             has_radius;  /* is radius used for searchs ?           */
//	double           radius;      /* radius for the searchs                 */
	CNearTreeHandle* tree_handle; /* pointer to the actual CNearTree struct */
	VALUE            points;      /* ruby array of the points inserted      */
	VALUE            values;      /* ruby array of the values inserted      */
} NearTreeRTree;

/* aux: error handling */
void check_error_code(int error_code) {
	if (error_code != CNEARTREE_SUCCESS) {
		char* message      = "NearTree error: %s";  /* message           */
		char* error_string = "unkown error";        /* default error     */
		VALUE exception    = rb_eRuntimeError;      /* default exception */

		switch(error_code) {
		case CNEARTREE_MALLOC_FAILED:
			error_string = "malloc failed";
			break;
		case CNEARTREE_BAD_ARGUMENT:
			error_string = "bad argument";
			exception    = rb_eArgError;
			break;
		case CNEARTREE_NOT_FOUND:
			error_string = "not found";
			exception    = rb_eKeyError;
			break;
		case CNEARTREE_FREE_FAILED:
			error_string = "free failed";
			break;
		case CNEARTREE_CVECTOR_FAILED:
			error_string = "CVector failed";
			break;
		}

		rb_raise(exception, message, error_string);
	}
}

/* aux: check CVector error */
void check_cvector_error(int error_code) {
	if (error_code < 0) {
		rb_raise(rb_eRuntimeError, "CVector error %d", error_code);
	}
}

/* aux: get struct handle for an object */
NearTreeRTree* get_object_handle(VALUE self) {
	NearTreeRTree* handle;
	Data_Get_Struct(self, NearTreeRTree, handle);
	return handle;
}

/* aux: get C dobule array from ruby index array, must be xfree()d when done with */
double* get_array_from_index(int dimension, VALUE index) {
	/* check index is Array */
	if (TYPE(index) != T_ARRAY) {
		rb_raise(rb_eTypeError, "Index is not an Array");
	}

	/* check index array length */
	int index_length = RARRAY_LEN(index);
	if (index_length != dimension) {
		rb_raise(rb_eArgError,
			"Invalid index array, should be %i, is %i", dimension, index_length);
	}

	/* copy to double array */
	VALUE*  index_ptr   = RARRAY_PTR(index);
	double* array_index = ALLOC_N(double, dimension);
	for(int i = 0; i != dimension; ++i) { array_index[i] = NUM2DBL(index_ptr[i]); }

	return array_index;
}

/* aux: extract the radius parameter */
double get_radius_from_value(int argc, VALUE* argv, int index) {
	if (index < argc && argv[index] != Qnil) {
		double radius = NUM2DBL(argv[index]);
		if (radius < 0.0) {
			rb_raise(rb_eArgError, "Radius must not be negative.");
		}

		return radius;
	} else { // no radius parameter : default
		return DBL_MAX;
	}
}

/* NearTree#initialize */
// FEATURE add support for other types of vectors (integer, string)
// FEATURE add support for NearTree norms
VALUE method_initialize(VALUE self, VALUE dimension) {
	NearTreeRTree* handle = get_object_handle(self);            /* handle         */

	/* dimension */
	if (!FIXNUM_P(dimension)) {
		rb_raise(rb_eTypeError, "Invalid dimension");
	}
	int d = NUM2INT(dimension);
	if (d <= 0) { rb_raise(rb_eArgError, "Invalid dimension %i", d); }
	handle->dimension = d;

	/* create CNearTree */
	check_error_code(CNearTreeCreate(handle->tree_handle, d, CNEARTREE_TYPE_DOUBLE));

	return self;
}

/* NearTree#insert */
VALUE method_insert(VALUE self, VALUE index, VALUE value) {
	NearTreeRTree* handle = get_object_handle(self);            /* handle          */

	/* array index, check for bad parameter */
	double* array_index = get_array_from_index(handle->dimension, index);

	/* insert point/value */
	rb_ary_push(handle->points, index);
	rb_ary_push(handle->values, value);

	/* insert element */
	check_error_code(
		CNearTreeImmediateInsert(*(handle->tree_handle), (void*)array_index, (void*)value));

	xfree(array_index);                                         /* free array index */

	return Qnil;
}

/* NearTree#find_nearest(index, radius = nil)    */
VALUE method_find_nearest(int argc, VALUE* argv, VALUE self) {
	if (!(argc == 1 || argc == 2)) {
		rb_raise(rb_eArgError, "Invalid parameter number: %d", argc);
	}

	NearTreeRTree* handle = get_object_handle(self);            /* handle           */

	/* array index */
	double* array_index = get_array_from_index(handle->dimension, argv[0]);

	/* radius      */
	double radius = get_radius_from_value(argc, argv, 1);

	VALUE*  value;                                              /* nearest value    */
	double* nearest_point;                                      /* nearest point    */

	check_error_code(CNearTreeNearestNeighbor(*(handle->tree_handle),
		radius, (void*)&nearest_point, (void*)&value, array_index));

	xfree(array_index);                                         /* free array index */

	/* nearest_point to ruby array */
	VALUE point_array = rb_ary_new();
	for(int i = 0; i != handle->dimension; ++i) {
		rb_ary_push(point_array, DBL2NUM(nearest_point[i]));
	}

	/* result array */
	VALUE result_array = rb_ary_new();
	rb_ary_push(result_array, point_array);
	rb_ary_push(result_array, *value);

	return result_array;
}

/* NearTree#find_k_nearest(index, k, radius = nil)    */
VALUE method_find_k_nearest(int argc, VALUE* argv, VALUE self) {
	if (!(argc == 2 || argc == 3)) {
		rb_raise(rb_eArgError, "Invalid parameter number: %d", argc);
	}

	NearTreeRTree* handle = get_object_handle(self);            /* handle           */

	/* array index */
	double* array_index = get_array_from_index(handle->dimension, argv[0]);

	/* k */
	if (!FIXNUM_P(argv[1])) {
		rb_raise(rb_eTypeError, "Invalid k");
	}
	long k = FIX2INT(argv[1]);
//	if (INT2FIX((int)k) != argv[1]) {
//	}
	if (k <= 0) {
		rb_raise(rb_eArgError, "k must be superior to 0");
	}

	/* radius      */
	double radius = get_radius_from_value(argc, argv, 2);

	CVectorHandle nearest_points;
	CVectorHandle nearest_values;
	check_cvector_error(CVectorCreate(&nearest_points, sizeof(void*), k));
	check_cvector_error(CVectorCreate(&nearest_values, sizeof(void*), k));

	check_error_code(CNearTreeFindKNearest(*(handle->tree_handle), (size_t)k, radius,
		nearest_points, nearest_values, array_index, 0));

	xfree(array_index);                                         /* free array index */

	/* result array */
	VALUE result_array = rb_ary_new();
	size_t points_size;
	size_t values_size;
	check_cvector_error(CVectorGetSize(nearest_points, &points_size));
	check_cvector_error(CVectorGetSize(nearest_values, &values_size));

	if (points_size != values_size) {
		rb_raise(rb_eRuntimeError, "Vector sizes error");
	}

	for(int i = 0; i != points_size; ++i) {
		VALUE result = rb_ary_new();
		/* coord */
		VALUE result_coord = rb_ary_new();
		for(int j = 0; j != handle->dimension; ++j) {
			rb_ary_push(result_coord,
				DBL2NUM(((double**)(nearest_points->array))[i][j]));
		}
		rb_ary_push(result, result_coord);
		rb_ary_push(result, *(((VALUE**)nearest_values->array)[i]));

		rb_ary_push(result_array, result);
	}

	return result_array;
}

/* writer : radius */
/*VALUE method_radius_equal(VALUE self, VALUE radius) {
	NearTreeRTree* handle = get_object_handle(self);
	if (radius == Qnil) {
		handle->has_radius = false;
		handle->radius     = DBL_MAX;
	} else {
		handle->has_radius = true;
		handle->radius     = NUM2DBL(radius);
	}
	return DBL2NUM(handle->radius); 
} */

/* readers: dimension/points/radius/values */
VALUE method_dimension(VALUE self) { return INT2NUM(get_object_handle(self)->dimension); }
VALUE method_points(   VALUE self) { return get_object_handle(self)->points;             }
VALUE method_values(   VALUE self) { return get_object_handle(self)->values;             }

VALUE method_empty(VALUE self) {
	return RARRAY_LEN(get_object_handle(self)->points) == 0 ? Qtrue : Qfalse;
}

/*VALUE method_radius(VALUE self) {
	NearTreeRTree* handle = get_object_handle(self);
	if (handle->has_radius) {
		return DBL2NUM(handle->radius);
	} else {
		return Qnil;
	}
} */

/* called by GC on marking of the object */
void neartree_gc_mark(NearTreeRTree* ptr) {
	rb_gc_mark(ptr->points); /* points */
	rb_gc_mark(ptr->values); /* values */
}

/* called by GC on collection of the object */
void neartree_gc_free(NearTreeRTree* ptr) {
	xfree(ptr->tree_handle); /* free CNearTreeHandle */
	xfree(ptr);              /* free NearTreeRTree   */
}

/* allocate */
VALUE neartree_allocate(VALUE class) {
	NearTreeRTree* handle = ALLOC(NearTreeRTree);   /* alloc NearTreeRTree          */
	handle->tree_handle   = ALLOC(CNearTreeHandle); /* alloc CNearTreeHandle        */
//	handle->has_radius    = false;                  /* by default, don't use radius */
//	handle->radius        = DBL_MAX;
	handle->points        = rb_ary_new();           /* points                       */
	handle->values        = rb_ary_new();           /* values                       */

	return Data_Wrap_Struct(class, neartree_gc_mark, neartree_gc_free, handle);
}

/* extension init */
void Init_neartree() {
	/* module/class */
	neartree_module = rb_define_module("NearTree");
	neartree_class  = rb_define_class_under(neartree_module, "RTree", rb_cObject);

	/* methods */
	rb_define_method(neartree_class, "initialize"    , method_initialize    ,  1);
	rb_define_method(neartree_class, "insert"        , method_insert        ,  2);
	rb_define_method(neartree_class, "find_nearest"  , method_find_nearest  , -1);
	rb_define_method(neartree_class, "find_k_nearest", method_find_k_nearest, -1);

	rb_define_method(neartree_class, "dimension", method_dimension, 0);
	rb_define_method(neartree_class, "points"   , method_points   , 0);
	rb_define_method(neartree_class, "values"   , method_values  , 0);
	rb_define_method(neartree_class, "empty?"   , method_empty  , 0);
//	rb_define_method(neartree_class, "radius",       method_radius,       0);

//	rb_define_method(neartree_class, "radius=",      method_radius_equal, 1);


	rb_define_alloc_func(neartree_class, neartree_allocate); /* register allocate */
}

