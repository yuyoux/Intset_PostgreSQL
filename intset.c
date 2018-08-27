#include "postgres.h"

#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */

PG_MODULE_MAGIC;

typedef struct intSet
{
	int32	length;
	int32	data[FLEXIBLE_ARRAY_MEMBER];
} intSet;

bool intset_contains_internal(intSet *set, int32 value);
intSet intset_sort_internal(intSet *set);
bool intset_containset_internal(intSet *setA, intSet *setB);
bool intset_equal_internal(intSet *setA, intSet *setB);
bool intset_intersection_internal(intSet *setA, intSet *setB);
//intSet intset_union_internal(intSet *setA, intSet *setB);





/*****************************************************************************
 * Input/Output functions
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
	int *res;
	intSet  *result ;
	
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
	
	//memset(res,0,length);
	res = (int*)calloc(length,sizeof(int));

	
	
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
	
	result =(intSet *)palloc(VARHDRSZ +length);
	SET_VARSIZE(result,VARHDRSZ+length);

	result->length = length;
	memcpy(result->data,res,length);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(intset_out);

Datum
intset_out(PG_FUNCTION_ARGS)
{
	intSet    *intset = (intSet *) PG_GETARG_POINTER(0);
	int	 i,offset=1;
	char *out;
	int *res = intset->data;
	out = (char*)calloc(2*intset->length+2,sizeof(char));
	//memset(out,0,2*intset->length+2);
	out[0]='{';
	for(i =0;i< intset->length;i++) {
		offset+=sprintf(out+offset,"%d,",res[i]);

	}
	psprintf(out+offset-1,"}\n\0");

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
intset_contains_internal(intSet *set, int32 value)
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
	intSet *set = (intSet *) PG_GETARG_POINTER(0);
	int32 value=0;
	bool res;
	
	res = intset_contains_internal(set, value);
	PG_RETURN_BOOL(res);
}
//-----------------------------------------//

//-------------------2---------------------//
intSet
intset_sort_internal(intSet *set) //sort the array and remove duplicated item
{
	int32 count = set->length;
	int i,j,k=0,m=0;
	for(i=0;i<count-1;i++){		//bubble sort
		for(j=0;j<count-i;j++){
			if(set->data[j]>set->data[j+1]){
				int32 temp=set->data[j];
				set->data[j]=set->data[j+1];
				set->data[j+1]=temp;
			}
		}
	}
	/*for(k=0;k<set->length;k++){	//remove duplicated items & count
		for(m=k+1;m<set->length;m++){
			if(set->data[k] == set->data[m]){
				set->data.splice(m,1);
				m--;
				c--;
			}
		}
	}*/
	
   	for (k=0; k<set->length-1; k++) //remove duplicated items & count
   	{
		while (set->data[k] == set->data[k+1]) 
		{
			k++;
	        }
		set->data[m++] = set->data[k];
	}

	set->length = m;
	return *set;
}


PG_FUNCTION_INFO_V1(intset_cardinality);

Datum
intset_cardinality(PG_FUNCTION_ARGS)
{
	int32 counter;
	intSet *set = (intSet *) PG_GETARG_POINTER(0);
	intset_sort_internal(set);
	counter = set->length;
	PG_RETURN_INT32(counter);
}
//-------------------------------------------//

//----------------------3--------------------//
//referenced from _int_tool.c from intarray
//arguments are assumed sorted & uniqueified
bool
intset_containset_internal(intSet *setA, intSet *setB)
{
	int			na,
				nb;
	int			i,
				j,
				n;
	int		   *da,
			   *db;
	intset_sort_internal(setA);
	intset_sort_internal(setB);	

	na = setA->length;
	nb = setB->length;
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
	intSet *setA = (intSet *) PG_GETARG_POINTER(0);
	intSet *setB = (intSet *) PG_GETARG_POINTER(1);
	bool res;
	
	res = intset_containset_internal(setA, setB);
	PG_RETURN_BOOL(res);
}

//------------------------------------------//


//---------------------4--------------------//
//referenced from _int_op.c from intarray
bool
intset_equal_internal(intSet *setA, intSet *setB)
{
	int			na,
				nb;
	int			n;
	int		   *da,
			   *db;
	bool		result;

	intset_sort_internal(setA);
	intset_sort_internal(setB);	

	na = setA->length;
	nb = setB->length;
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
	intSet *setA = (intSet *) PG_GETARG_POINTER(0);
	intSet *setB = (intSet *) PG_GETARG_POINTER(1);
	bool res;
	
	res = intset_equal_internal(setA, setB);
	PG_RETURN_BOOL(res);

}
//---------------------------------------------//

//---------------------5----------------------//
bool
intset_intersection_internal(intSet *setA, intSet *setB)
{
	int			na,
				nb;
	int			i,
				j;
	int		   *da,
			   *db;

	intset_sort_internal(setA);
	intset_sort_internal(setB);	

	na = setA->length;
	nb = setB->length;
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
	intSet *setA = (intSet *) PG_GETARG_POINTER(0);
	intSet *setB = (intSet *) PG_GETARG_POINTER(1);
	bool res;
	
	res = intset_intersection_internal(setA, setB);
	PG_RETURN_BOOL(res);

}

//---------------------------------------------//

//----------------------6----------------------//
/*
intSet
intset_union_internal(intSet *setA, intSet *setB){
	intset_sort_internal(setA);
	intset_sort_internal(setB);
	
	int i = 0;
	int j = 0;
	int k = 0;
	intSet *r = NULL;
	na = setA->length;
	nb = setB->length;

	if (na == 0 && nb == 0)
		return new_intArrayType(0);
	if (na)
		r = copy_intArrayType(b);
	if (nb)
		r = copy_intArrayType(a);

	

}

PG_FUNCTION_INFO_V1(intset_union);

Datum
intset_union(PG_FUNCTION_ARGS)
{
	intSet *setA = (intSet *) PG_GETARG_POINTER(0);
	intSet *setB = (intSet *) PG_GETARG_POINTER(1);
	intSet *result;


	result = intset_union_internal(setA, setB);

	//pfree(a);
	//pfree(b);

	PG_RETURN_POINTER(result);
}
*/


