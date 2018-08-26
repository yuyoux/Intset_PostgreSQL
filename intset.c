/*
 * src/tutorial/complex.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"

#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */

PG_MODULE_MAGIC;

typedef struct Intset
{
	int32	length;
	int32	data[FLEXIBLE_ARRAY_MEMBER];
} Intset;


/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

 *****************************************************************************/
PG_FUNCTION_INFO_V1(intset_in);

Datum
intset_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);

	int length=0;
	int i,j=0;
	int index =strlen(str);
	
	char *temp=malloc(index);
	char *token;
	
	for(i =0;i<index;i++){                  //strip the whitespaces and count the nb of element 
		if(str[i]!=' '){
			if(str[i]==',') length++;
			temp[j]=str[i];
			j++;
		}
		temp[j]=0;
	}
 
	//printf("index: %d\n",strlen(temp));
         
	if (length>1) length++;
	//printf("length:%d\n",length);    
    
	j=0;
	int res[length];
	memset(res,0,length);

	
	
	token= strtok(temp,",");
	if(temp[0]!='{')  printf("invalid input");        //check for '{'
	token++;
	while(token!=NULL){		     //get the value				
		res[j]= atoi(token);
		//printf("res-%d\n",res[j]);
		j++;
		if (token[strlen(token)-1]=='}') {
			//printf("ok\n");
			break;}
		token=strtok(NULL,",");			
	}
	if(token==NULL) printf("invalid\n");          //check for '}'
//----------------------------------------------------
	intSet  *result =(intSet *)palloc(VARHDRSZ +length);
	SET_VARSIZE(result,VARSIZE+length);

	result->length = length;
	memcpy(result->data,res,length);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(intset_out);

Datum
intset_out(PG_FUNCTION_ARGS)
{
	Intset    *intset = (Intset *) PG_GETARG_POINTER(0);
	int	 i,offset=1;


	char out=[2*intset->length+2];
	out[0]='{';
	for(i =0;i< intset->length;i++) {
		offset+=sprintf(out+offset,"%d,",res[i]);

	}
	sprintf(out+offset-1,"}\n\0,");

	PG_RETURN_CSTRING(out);
}


/*****************************************************************************
 * New Operators
 *
 * A practical Complex datatype would provide much more than this, of course.
 *****************************************************************************/

//PG_FUNCTION_INFO_V1(complex_add);

//Datum
/**complex_add(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);
	Complex    *result;

	result = (Complex *) palloc(sizeof(Complex));
	result->x = a->x + b->x;
	result->y = a->y + b->y;
	PG_RETURN_POINTER(result);
}
**/

//------------------1----------------------//
bool
intset_contains_internal(Intset *set, int32 value)
{
	int32 i = 0;
	for (i=0; i < set->length; ++i){
		if (set->data[i] == value){
			return true;
		}
	}
	return false;
}

PG_FUNCTION_INFO_V1(intset_contains);

Datum
intset_contains(PG_FUNCTION_ARGS)
{
	Intset *set = (Intset *) PG_GETARG_POINTER(0);
	int32 value;
	bool res;
	
	res = intset_contains_internal(set, value);
	PG_RETURN_BOOL(res);
}
//-----------------------------------------//

//-------------------2---------------------//
Intset
intset_sort_internal(Intset *set) //sort the array and remove duplicated item
{
	int32 count = set->length;

	for(i=0;i<count-1;i++){		//bubble sort
		for(j=0;j<count-i;j++){
			if(set[j]>set[j+1]){
				int temp=set[j];
				set[j]=set[j+1];
				set[j+1]=temp;
			}
		}
	}
	for(i=0;i<set.length;i++){	//remove duplicated items
		for(j=i+1;j<set.length;j++){
			if(set[i] == set[j]){
				set.splice(j,1);
				j--;
			}
		}
	}
	return set;
}

int
intset_cardinality__internal(Intset *set) //cardinality
{
	set2 = intset_sort_internal(set);
	int32 count = set2.length;
	return count;
}

PG_FUNCTION_INFO_V1(intset_cardinality);

Datum
intset_cardinality(PG_FUNCTION_ARGS)
{
	Intset *set = (Intset *) PG_GETARG_POINTER(0);
	int32 count = intset_cardinality_internal(set);
	PG_RETURN_INT32(count);
}
//-------------------------------------------//

//----------------------3--------------------//
//referenced from _int_tool.c from intarray
//arguments are assumed sorted & uniqueified
bool
intset_containset_internal(Intset *setA, Intset *setB)
{
	int			na,
				nb;
	int			i,
				j,
				n;
	int		   *da,
			   *db;

	na = intset_cardinality_internal(setA);
	nb = intset_cardinality_internal(setB);
	da = setA->data;
	db = setB->data;

	i = j = n = 0;
	while (i < na && j < nb)
	{
		if (da[i] < db[j])
			i++;
		else if (da[i] == db[j])
		{
			n++;
			i++;
			j++;
		}
		else
			break;	/* db[j] is not in da */
	}

	return (n == nb) ? TRUE : FALSE;
}


PG_FUNCTION_INFO_V1(intset_containset);

Datum
intset_containset(PG_FUNCTION_ARGS)
{
	Intset *setA = (Intset *) PG_GETARG_POINTER(0);
	Intset *setB = (Intset *) PG_GETARG_POINTER(1);
	bool res;
	
	res = intset_containset_internal(setA, setB);
	PG_RETURN_BOOL(res);
}

//------------------------------------------//


//---------------------4--------------------//
//referenced from _int_op.c from intarray
bool
intset_equal_internal(Intset *setA, Intset *setB)
{
	int			na,
				nb;
	int			n;
	int		   *da,
			   *db;
	bool		result;

	na = intset_cardinality_internal(setA);
	nb = intset_cardinality_internal(setB);
	da = setA->data;
	db = setB->data;

	result = FALSE;

	if (na == nb)
	{
		//SORT(a);
		//SORT(b);
		result = TRUE;

		for (n = 0; n < na; n++)
		{
			if (da[n] != db[n])
			{
				result = FALSE;
				break;
			}
		}
	}
	return result;
}


PG_FUNCTION_INFO_V1(intset_equal);

Datum
intset_equal(PG_FUNCTION_ARGS){
	Intset *setA = (Intset *) PG_GETARG_POINTER(0);
	Intset *setB = (Intset *) PG_GETARG_POINTER(1);
	bool res;
	
	res = intset_equal_internal(setA, setB);
	PG_RETURN_BOOL(res);

}
//---------------------------------------------//

//---------------------5----------------------//
bool
intset_intersection_internal(Intset *setA, Intset *setB)
{
	int			na,
				nb;
	int			i,
				j;
	int		   *da,
			   *db;

	na = intset_cardinality_internal(setA);
	nb = intset_cardinality_internal(setB);
	da = setA->data;
	db = setB->data;

	i = j = 0;
	while (i < na && j < nb)
	{
		if (da[i] < db[j])
			i++;
		else if (da[i] == db[j])
			return TRUE;
		else
			j++;
	}

	return FALSE;
}

PG_FUNCTION_INFO_V1(intset_intersection);

Datum
intset_intersection(PG_FUNCTION_ARGS){
	Intset *setA = (Intset *) PG_GETARG_POINTER(0);
	Intset *setB = (Intset *) PG_GETARG_POINTER(1);
	bool res;
	
	res = intset_intersection_internal(setA, setB);
	PG_RETURN_BOOL(res);

}

//---------------------------------------------//

/*****************************************************************************
 * Operator class for defining B-tree index
 *
 * It's essential that the comparison operators and support function for a
 * B-tree index opclass always agree on the relative ordering of any two
 * data values.  Experience has shown that it's depressingly easy to write
 * unintentionally inconsistent functions.  One way to reduce the odds of
 * making a mistake is to make all the functions simple wrappers around
 * an internal three-way-comparison function, as we do here.
 *****************************************************************************/

#define Mag(c)	((c)->x*(c)->x + (c)->y*(c)->y)

static int
complex_abs_cmp_internal(Complex * a, Complex * b)
{
	double		amag = Mag(a),
				bmag = Mag(b);

	if (amag < bmag)
		return -1;
	if (amag > bmag)
		return 1;
	return 0;
}


PG_FUNCTION_INFO_V1(complex_abs_lt);

Datum
complex_abs_lt(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) < 0);
}

PG_FUNCTION_INFO_V1(complex_abs_le);

Datum
complex_abs_le(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) <= 0);
}

PG_FUNCTION_INFO_V1(complex_abs_eq);

Datum
complex_abs_eq(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) == 0);
}

PG_FUNCTION_INFO_V1(complex_abs_ge);

Datum
complex_abs_ge(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) >= 0);
}

PG_FUNCTION_INFO_V1(complex_abs_gt);

Datum
complex_abs_gt(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) > 0);
}

PG_FUNCTION_INFO_V1(complex_abs_cmp);

Datum
complex_abs_cmp(PG_FUNCTION_ARGS)
{
	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

	PG_RETURN_INT32(complex_abs_cmp_internal(a, b));
}
