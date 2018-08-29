#include "postgres.h"

#include "fmgr.h"
# include <stdio.h>
# include <string.h>
# include <stdlib.h>
#include "libpq/pqformat.h"		/* needed for send/recv functions */


PG_MODULE_MAGIC;

typedef struct intSet
{
	int32	length;
	int32	data[FLEXIBLE_ARRAY_MEMBER];
} intSet;

bool intset_contains_internal(int value, intSet *set);
intSet intset_sort_internal(intSet *set);
bool intset_containset_internal(intSet *setA, intSet *setB);
bool intset_equal_internal(intSet *setA, intSet *setB);
intSet* intset_intersection_internal(intSet *setA, intSet *setB);
intSet* intset_union_internal(intSet *setA, intSet *setB);





/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/
PG_FUNCTION_INFO_V1(intset_in);

Datum
intset_in(PG_FUNCTION_ARGS)
{
	char  *str = PG_GETARG_CSTRING(0);

	int length=0;
	int i,j=0,k=0,m=0,t=0;
	int index =strlen(str);
	
	char *temp=malloc(index);
	char *token;
	int *res;
	int *distinct;
	intSet  *result ;
	
	for(i =0;i<index;i++){                  //strip the whitespaces and count the nb of element 
		if(str[i]!=' '){
			if(str[i]==',') length++;   //count fot ','
			temp[j]=str[i];
			j++;
		}
		temp[j]=0;
	}
 
         
	if (length>=1) length++;
	if (strlen(temp)==3) length =1; 
	j=0;


	res = (int*)calloc(length,VARHDRSZ);
	token= strtok(temp,",");
	if(temp[0]!='{')  printf("invalid input");        //check for '{'
	token++;
	while(token!=NULL){		                  //get the value				
		res[j]= atoi(token);
		j++;
		if (token[strlen(token)-1]=='}') break;
		
		token=strtok(NULL,",");	
		
	}
	if(token==NULL) printf("invalid\n");


	if(length==2){
		m=2;
		if(res[0]==res[1]) m=1;
		else if (res[0]>res[1]) {
			t = res[0];
			res[0]=res[1];
			res[1]=t;
		}
	}
	else{
		for(i=0;i<length;i++){		//bubble sort
			for(j=i+1;j<length;j++){
				if(res[i]>res[j]){
					t=res[i];
					res[i]=res[j];
					res[j]=t;
				}
			}
		}
	
	   	for (k=0; k<=length-1; k++) //remove duplicated items & count
	   	{
			while (res[k] == res[k+1]) 
			{
				k++;
			}
			res[m++] = res[k];
		}
	}
	distinct = (int*)calloc(m,VARHDRSZ);
	for(i=0;i<m;i++) {
		distinct[i] = res[i];
		//printf("%d ",res[i]);
	}
	
	length =m;
	result =(intSet *)palloc(VARHDRSZ+length*VARHDRSZ);
	SET_VARSIZE(result,VARHDRSZ+length*VARHDRSZ);

	result->length = length;
	memcpy(result->data,distinct,length*VARHDRSZ);
	//memcpy(result->data,res,i);
	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(intset_out);

Datum
intset_out(PG_FUNCTION_ARGS)
{
	intSet    *intset = (intSet *) PG_GETARG_POINTER(0);
	int	 i,offset=1;
	char *out;
	//int *res = (int*)palloc(intset->length);
	int *res = intset->data;
	
	out = (char*)calloc(2*intset->length+1,sizeof(char));
	
	//offset+=sprintf(out,"%d,{",res[4]);
	out[0]='{';
	for(i =0;i< intset->length;i++) {
		offset+=sprintf(out+offset,"%d,",res[i]);
	}
	if (intset->length==0) sprintf(out+1,"}");
	else sprintf(out+offset-1,"}");
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

//------------------1------------OK----------//
bool
intset_contains_internal(int value, intSet *set)
{
	int i = 0;
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
	int32 value = PG_GETARG_INT32(0);
	intSet *set = (intSet *) PG_GETARG_POINTER(1);
	
	bool res;
	
	res = intset_contains_internal(value, set);
	PG_RETURN_BOOL(res);
}
//-----------------------------------------//

//-------------------2----------OK-----------//
intSet
intset_sort_internal(intSet *set) //sort the array
{
	int32 count = set->length;
	int i,j;
	for(i=0;i<count-1;i++){		//bubble sort
		for(j=0;j<count-i;j++){
			if(set->data[j]>set->data[j+1]){
				int32 temp=set->data[j];
				set->data[j]=set->data[j+1];
				set->data[j+1]=temp;
			}
		}
	}
	
	return *set;
}


PG_FUNCTION_INFO_V1(intset_cardinality);

Datum
intset_cardinality(PG_FUNCTION_ARGS)
{
	int32 counter;
	intSet *set = (intSet *) PG_GETARG_POINTER(0);
	//intset_sort_internal(set);
	counter = set->length;
	PG_RETURN_INT32(counter);
}
//-------------------------------------------//

//----------------------3----------OK----------//
//referenced from _int_tool.c from build-in type intarray
//already sorted & uniqueified
bool
intset_containset_internal(intSet *setA, intSet *setB)
{
	int na, nb;
	int i, j, n;
	int *da, *db;
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
			break;	
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


//---------------------4----------OK----------//
//referenced from _int_op.c from build-in type intarray
bool
intset_equal_internal(intSet *setA, intSet *setB)
{
	int na, nb;
	int n;
	int *da,*db;
	bool result;

	intset_sort_internal(setA);
	intset_sort_internal(setB);	

	na = setA->length;
	nb = setB->length;
	da = setA->data;
	db = setB->data;

	result = FALSE;

	if (na == nb)
	{
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

//---------------------5-------OK---------------//
intSet*
intset_intersection_internal(intSet *setA, intSet *setB)
{	
	intSet  *r;
	int na, nb;
	int *da, *db, *dr;
	int i, j, k;
	//intset_sort_internal(setA);
	//intset_sort_internal(setB);	

	na = setA->length;
	nb = setB->length;
	da = setA->data;
	db = setB->data;
	//if (na>nb){
	
	//}else{
	//r = (intSet *)palloc0(VARHDRSZ+na);
	//}

	if (na == 0 || nb == 0){
		//return new_intArrayType(0);
		r = (intSet *)palloc(VARHDRSZ);
		r->length=0;
		SET_VARSIZE(r,r->length);
		return r;
		}

	//r->length = Min(na, nb);
	dr = (int*)calloc(na+nb,sizeof(int));

	i = j = k = 0;
	while (i < na && j < nb)
	{
		if (da[i] < db[j])
			i++;
		else if (da[i] == db[j])
		{
			if (k == 0 || dr[k - 1] != db[j])
				dr[k++] = db[j];
			i++;
			j++;
		}
		else
			j++;
	}

	if (k == 0)
	{
		//pfree(r);
		//return new_intArrayType(0);
		r = (intSet *)palloc(VARHDRSZ);
		r->length=0;
		SET_VARSIZE(r,r->length);
		//r->length=0;
		return r;
	}
	else
		//return resize_intArrayType(r, k);
		r = (intSet *)palloc(VARHDRSZ+k*VARHDRSZ);
		
		//r = (intSet *) repalloc(r, k);
		dr = realloc(dr,k);
		r->length = k;
		SET_VARSIZE(r, VARHDRSZ+k*VARHDRSZ);
		r->length = k;
		memcpy(r->data,dr,k*VARHDRSZ);
			
		//ereport(ERROR,
		//		(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
		//		 errmsg("invalid input syntax for complex: \"%d,%d,%d\"",
		//				dr[2],k,r->length)));	
		return r;
}

PG_FUNCTION_INFO_V1(intset_intersection);

Datum
intset_intersection(PG_FUNCTION_ARGS){
	intSet *setA = (intSet *) PG_GETARG_POINTER(0);
	intSet *setB = (intSet *) PG_GETARG_POINTER(1);
	intSet *result;
	
	result = intset_intersection_internal(setA, setB);
	
	//pfree(setA);
	//pfree(setB);
	PG_RETURN_POINTER(result);

}

//---------------------------------------------//

//----------------------6---------wrong-------------//

intSet*
intset_union_internal(intSet *setA, intSet *setB){
	intSet  *r = NULL;
	int na, nb;
	int i, j;
	int *da, *db, *dr;
	intset_sort_internal(setA);
	intset_sort_internal(setB);
	
	na = setA->length;
	nb = setB->length;
	da = setA->data;
	db = setB->data;

	if (na == 0 && nb == 0){
		r = (intSet *)palloc(VARHDRSZ);
		r->length=0;
		SET_VARSIZE(r,r->length);
		return r;
	}
	else if (na == 0)
		return setB;
			
	else if (nb == 0)
		return setB;
	
	else
	{	
		dr = (int*)calloc(na+nb,sizeof(int));
		
		/* union */
		i = j =k= 0;
		while (i < na && j < nb)
		{
			if (da[i] == db[j])
			{
				dr[k++] = da[i++];
				j++;
			}
			else if (da[i] < db[j])
				dr[k++] = da[i++];
			else
				dr[k++] = db[j++];
		}

		while (i < na)
			dr[k++] = da[i++];
		while (j < nb)
			dr[k++] = db[j++];

		r = (intSet *)palloc(VARHDRSZ+VARHDRSZ*k);
		SET_VARSIZE(r,VARHDRSZ+VARHDRSZ*k);
		r->length = k;
		memcpy(r->data,dr,k*VARHDRSZ);
	}
	//if (r->length > 1)		//REMOVE DUPLICATE
	//	r = _int_unique(r);

	return r;
}

PG_FUNCTION_INFO_V1(intset_union);

Datum
intset_union(PG_FUNCTION_ARGS)
{
	intSet *setA = (intSet *) PG_GETARG_POINTER(0);
	intSet *setB = (intSet *) PG_GETARG_POINTER(1);
	intSet *result;

	result = intset_union_internal(setA, setB);
	//pfree(setA);
	//pfree(setB);
	PG_RETURN_POINTER(result);
}

//----------------------------------------------//


//-------------------------7--------------------//
//A !! B takes the set disjunction, and produces an intSet containing elements 
//that are in A and not in B, or that are in B and not in A.
/*intSet*
intset_disjunction_internal(intSet *setA, intSet *setB){


}
PG_FUNCTION_INFO_V1(intset_disjunction);
Datum
intset_disjunction(PG_FUNCTION_ARGS)
{
	intSet *setA = (intSet *) PG_GETARG_POINTER(0);
	intSet *setB = (intSet *) PG_GETARG_POINTER(1);
	intSet *result;
	result = intset_disjunction_internal(setA, setB);
	//pfree(setA);
	//pfree(setB);
	PG_RETURN_POINTER(result);
}
*/
//----------------------------------------------//



//------------------------8---------------------//
//A - B takes the set difference, and produces an intSet containing elements that 
//are in A and not in B. Note that this is not the same as A !! B.
/*intSet*
intset_difference_internal(intSet *setA, intSet *setB){



}
PG_FUNCTION_INFO_V1(intset_difference);
Datum
intset_difference(PG_FUNCTION_ARGS)
{
	intSet *setA = (intSet *) PG_GETARG_POINTER(0);
	intSet *setB = (intSet *) PG_GETARG_POINTER(1);
	intSet *result;
	result = intset_difference_internal(setA, setB);
	//pfree(setA);
	//pfree(setB);
	PG_RETURN_POINTER(result);
}
*/
//----------------------------------------------//

