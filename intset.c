#include "postgres.h"
#include "fmgr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "libpq/pqformat.h"


PG_MODULE_MAGIC;

typedef struct intSet
{
	int32	length;
	int	data[FLEXIBLE_ARRAY_MEMBER];
} intSet;

/*****************************************************************************
 * Declaration
 *****************************************************************************/

bool intset_contains_internal(int value, intSet *set);
intSet intset_sort_internal(intSet *set);
bool intset_containset_internal(intSet *setA, intSet *setB);
bool intset_equal_internal(intSet *setA, intSet *setB);
intSet* intset_intersection_internal(intSet *setA, intSet *setB);
intSet* intset_union_internal(intSet *setA, intSet *setB);
intSet* intset_difference_internal(intSet *setA, intSet *setB);
intSet* intset_disjunction_internal(intSet *setA, intSet *setB);

/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/
PG_FUNCTION_INFO_V1(intset_in);

Datum
intset_in(PG_FUNCTION_ARGS)
{
	char  *str = PG_GETARG_CSTRING(0);
	int quo_flag=1;
	int length=0;
	int i,j=0,k=0,m=0,t=0;
	int index =strlen(str);
	char *temp=calloc(index+1,sizeof(char));
	char *token;
	int *res;
	intSet  *result ;
	
	for(i =0;i<index;i++){                  //strip the whitespaces and count the nb of element 
		if(str[i]!=' ') {
			if(str[i]=='{' ){	//check for duplicate '{' 
				if(i!=0)  ereport(ERROR,
						(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
						 errmsg("invalid input syntax for intSet")));
				
			}
			else if(str[i]==',') {
				length++;       //count fot ','
				quo_flag=1;

			}
			else if(str[i]=='}'){   //check for duplicate '}' 
				if(i!=index-1){
					ereport(ERROR,
						(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
						  errmsg("invalid input syntax for intSet")));
				}
			}
			else{
				if(isdigit(str[i])==0) {	//check whether is digit or not (float or not)
					ereport(ERROR,
						(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
						 errmsg("invalid input syntax for intSet")));     
				}
				else if(quo_flag==0 && str[i-1]==' '){		//check if lack the ',' between two digits
					ereport(ERROR,
						(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
						 errmsg("invalid input syntax for intSet")));    
				}
				else quo_flag=0;
				
			}
			
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
	if(temp[0]!='{')  ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for intSet")));            //check for '{'
	token++;
	while(token!=NULL){		                  //get the value				
		res[j]= atoi(token);
		j++;
		if (token[strlen(token)-1]=='}') break;
		
		token=strtok(NULL,",");	
		
	}
	if(token==NULL) ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for intSet")));    


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
	
	
	
	result =(intSet *)palloc(VARHDRSZ+m*VARHDRSZ);
	
	SET_VARSIZE(result,VARHDRSZ+m*VARHDRSZ);
	 	
	memcpy((void*)VARDATA(result),res,m*VARHDRSZ);
	
	PG_RETURN_POINTER(result);
}




PG_FUNCTION_INFO_V1(intset_out);

Datum
intset_out(PG_FUNCTION_ARGS)
{	
	intSet    *intset = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(0));
	int	 i,offset=1,length=0;
	char *out;
	char *new;
	int *res = (int*)VARDATA(intset);
	int max_mem = 200;
	length = VARSIZE_ANY_EXHDR(intset)/4;   //get the number of element
	
	if(length==0) out = (char*)palloc0(3*sizeof(char));
	else out=calloc(200,sizeof(char));
	
	out[0]='{';
	for(i =0;i< length;i++) {
		offset+=sprintf(out+offset,"%d,",res[i]);
		if(max_mem-offset<=10) {        //since we don't know the number of characters when convert interger to string, 
			max_mem+=200;		//add the space of memory when needed
			new =calloc(offset+1,sizeof(char));
			memcpy(new,out,offset+1);			
			free(out);			
			out = calloc(max_mem,sizeof(char));
			memcpy(out,new,max_mem);
			free(new);
		}	
	}
	if (length==0) offset+=sprintf(out+1,"}");
	else sprintf(out+offset-1,"}");
	PG_RETURN_CSTRING(out);	
	
}

/*****************************************************************************
 * New Operators
 *****************************************************************************/

//i <@ S  intSet set contains the integer value; that is, value ∈ set.
bool
intset_contains_internal(int value, intSet *set)
{
	int i = 0;
	int* data = (int*)VARDATA(set);
	int count = VARSIZE_ANY_EXHDR(set)/4;
	for (i=0; i < count; ++i){
		if (data[i] == value){
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
	intSet *set = (intSet *)PG_DETOAST_DATUM(PG_GETARG_POINTER(1));
	bool res;
	res = intset_contains_internal(value, set);
	PG_RETURN_BOOL(res);
}

//@ S  give the cardinality, or number of distinct elements in, intSet S; that is, |S|.
intSet
intset_sort_internal(intSet *set) //sort the array
{
	int count = VARSIZE_ANY_EXHDR(set)/4;
	int i,j,t;
	int *data =(int*) VARDATA(set);
	for(i=0;i<count;i++){		//bubble sort
		for(j=i+1;j<count;j++){
			if(data[j]>data[j+1]){
				t=data[j];
				data[j]=data[j+1];
				data[j+1]=t;
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
	intSet *set = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(0));
	counter = VARSIZE_ANY_EXHDR(set)/4;
	PG_RETURN_INT32(counter);
}

//A @> B    does intSet A contain only values in intSet B? that is, for every element of A, is it an element of B? (A⊂B)
//referenced from _int_tool.c from build-in type intarray
//already sorted & uniqueified
bool
intset_containset_internal(intSet *setA, intSet *setB)
{
	int na, nb;
	int i, j, n;
	int *da, *db;
	na = VARSIZE_ANY_EXHDR(setA)/4;
	nb = VARSIZE_ANY_EXHDR(setB)/4;	
	da =(int*) VARDATA(setA);
	db = (int*)VARDATA(setB);

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
	intSet *setA = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(0));
	intSet *setB = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(1));
	bool res;
	res = intset_containset_internal(setB, setA);
	PG_RETURN_BOOL(res);
}


//A = B   intSets A and B are equal; that is, intSet A contains all the values of intSet B and intSet B contains all the 
//values of intSet A, or, every element in A can be found in B, and vice versa.
//referenced from _int_op.c from build-in type intarray
bool
intset_equal_internal(intSet *setA, intSet *setB)
{
	int na, nb;
	int n;
	int *da,*db;
	bool result;
	na = VARSIZE_ANY_EXHDR(setA)/4;
	nb = VARSIZE_ANY_EXHDR(setB)/4;	
	da =(int*) VARDATA(setA);
	db = (int*)VARDATA(setB);	
	result = FALSE;
	if (na == nb)			//check length
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
	intSet *setA = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(0));
	intSet *setB = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(1));
	bool res;	
	res = intset_equal_internal(setA, setB);
	PG_RETURN_BOOL(res);

}


//A && B  takes the set intersection, and produces an intSet containing the elements common to A and B; that is, A∩B.
//referenced from _int_tool.c from build-in type intarray
intSet*
intset_intersection_internal(intSet *setA, intSet *setB)
{	
	intSet  *r;
	int na, nb;
	int *da, *db, *dr;
	int i, j, k;

	na = VARSIZE_ANY_EXHDR(setA)/4;
	nb = VARSIZE_ANY_EXHDR(setB)/4;	
	da =(int*) VARDATA(setA);
	db = (int*)VARDATA(setB);

	if (na == 0 || nb == 0){
		r = (intSet *)palloc(VARHDRSZ);
		SET_VARSIZE(r,VARHDRSZ);
		return r;
		}

	dr = (int*)calloc(na+nb,sizeof(int));

	i = j = k = 0;
	while (i < na && j < nb)		//get the intersection
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
	
	if (k == 0)				//empty set
	{
		r = (intSet *)palloc(VARHDRSZ);
		SET_VARSIZE(r,VARHDRSZ);
		return r;
	}
	else
		r = (intSet *)palloc(VARHDRSZ+k*VARHDRSZ);	//VARHDRSZ = sizeof(int32)
		SET_VARSIZE(r, VARHDRSZ+k*VARHDRSZ);
		memcpy(VARDATA(r),dr,k*VARHDRSZ);
		return r;
}

PG_FUNCTION_INFO_V1(intset_intersection);

Datum
intset_intersection(PG_FUNCTION_ARGS){
	intSet *setA = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(0));
	intSet *setB = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(1));
	intSet *result;
	
	result = intset_intersection_internal(setA, setB);
	PG_RETURN_POINTER(result);

}



//A || B  takes the set union, and produces an intSet containing all the elements of A and B; that is, A∪B.
//referenced from _int_tool.c from build-in type intarray
intSet*
intset_union_internal(intSet *setA, intSet *setB){
	intSet  *r = NULL;
	int na, nb;
	int i, j,k;
	int *da, *db,*dr;
	
	na = VARSIZE_ANY_EXHDR(setA)/4;
	nb = VARSIZE_ANY_EXHDR(setB)/4;
	da =(int*) VARDATA(setA);
	db = (int*)VARDATA(setB);

	if (na == 0 && nb == 0){	//if two empty sets input
		return setA;
	}
	else if (na == 0)
		return setB;		//if set a is empty
			
	else if (nb == 0)		//if set b is empty
		return setA;
	
	else				//if both are not empty
	{	
		dr = (int*)calloc(na+nb,sizeof(int));
		
		// union 
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

		r = (intSet *)palloc(VARHDRSZ+sizeof(int32)*k);
	
		SET_VARSIZE(r,VARHDRSZ+sizeof(int32)*k);

			
		memcpy( VARDATA(r),dr,k*sizeof(int32));
	}
	return r;
}

PG_FUNCTION_INFO_V1(intset_union);

Datum
intset_union(PG_FUNCTION_ARGS)
{
	intSet *setA = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(0));
	intSet *setB = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(1));
	intSet *result;
	result = intset_union_internal(setA, setB);
	PG_RETURN_POINTER(result);
}


//A !! B takes the set disjunction, and produces an intSet containing elements 
//that are in A and not in B, or that are in B and not in A.
intSet*
intset_disjunction_internal(intSet *setA, intSet *setB){
	intSet  *r;
	int na, nb;
	int *da, *db, *dr;
	int i, j, k, l,m,n;
	na = VARSIZE_ANY_EXHDR(setA)/4;
	nb = VARSIZE_ANY_EXHDR(setB)/4;
	da =(int*) VARDATA(setA);
	db = (int*)VARDATA(setB);
	
	if (na == 0){				// if set a is empty
		dr = (int*)calloc(na+nb,sizeof(int));
		i=0;
		for(i=0;i<nb;i++){
			dr[i]=db[i];
		}
		r = (intSet *)palloc(VARHDRSZ+nb*VARHDRSZ);
		SET_VARSIZE(r, VARHDRSZ+nb*VARHDRSZ);
		memcpy(VARDATA(r),dr,nb*VARHDRSZ);
		free(dr);
		return r;
	}
	else if (nb == 0){			//if set b is empty
		dr = (int*)calloc(na+nb,sizeof(int));
		i=0;
		for(i=0;i<na;i++){
			dr[i]=da[i];
		}
		r = (intSet *)palloc(VARHDRSZ+na*VARHDRSZ);
		SET_VARSIZE(r, VARHDRSZ+na*VARHDRSZ);
		memcpy(VARDATA(r),dr,na*VARHDRSZ);
		free(dr);
		return r;

	}else if (na == 0 && nb == 0){		//if two empty sets input
		r = (intSet *)palloc(VARHDRSZ);
		SET_VARSIZE(r,VARHDRSZ);
		return r;	
	
	}else{					//if both are not empty
		dr = (int*)calloc(na+nb,sizeof(int));
	
		i=j=k=m=n=0;
		for (j=0;j<na;j++){
			for(i=0;i<nb;i++){
				if(da[j]==db[i]){
					break;
				}
			}
			if (i == nb){
				dr[k++] = da[j];
			
			}
		}
		l = k;
		for (m=0;m<nb;m++){
			for(n=0;n<na;n++){
				if(db[m]==da[n]){
					break;
				}
			}
			if (n == na){
				dr[l++] = db[m];
			
			}
		}

		r = (intSet *)palloc(VARHDRSZ+l*VARHDRSZ);
		SET_VARSIZE(r, VARHDRSZ+l*VARHDRSZ);
		memcpy(VARDATA(r),dr,l*VARHDRSZ);
		free(dr);
		return r;
	}

}


PG_FUNCTION_INFO_V1(intset_disjunction);

Datum
intset_disjunction(PG_FUNCTION_ARGS)
{
	intSet *setA = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(0));
	intSet *setB = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(1));
	intSet *result;
	result = intset_disjunction_internal(setA, setB);
	PG_RETURN_POINTER(result);
}


//A - B takes the set difference, and produces an intSet containing elements that 
//are in A and not in B. Note that this is not the same as A !! B.
intSet*
intset_difference_internal(intSet *setA, intSet *setB){
	intSet  *r;
	int na, nb;
	int *da, *db, *dr;
	int i, j, k;
	na = VARSIZE_ANY_EXHDR(setA)/4;
	nb = VARSIZE_ANY_EXHDR(setB)/4;
	da =(int*) VARDATA(setA);
	db = (int*)VARDATA(setB);

	if (na == 0){				//if set a is empty
		r = (intSet *)palloc(VARHDRSZ);
		SET_VARSIZE(r,VARHDRSZ);
		return r;
	}

	else if (nb == 0){			//if set b is empty
		dr = (int*)calloc(na+nb,sizeof(int));
		i=0;
		for(i=0;i<na;i++){
			dr[i]=da[i];
		}
		r = (intSet *)palloc(VARHDRSZ+na*VARHDRSZ);
		SET_VARSIZE(r, VARHDRSZ+na*VARHDRSZ);
		memcpy(VARDATA(r),dr,na*VARHDRSZ);
		return r;
		
	}else{			
		dr = (int*)calloc(na+nb,sizeof(int));
		i=j=k=0;
		for (j=0;j<na;j++){
			for(i=0;i<nb;i++){
				if(da[j]==db[i]){
					break;
				}
			}
			if (i == nb){
				dr[k++] = da[j];
			
			}
		}

		r = (intSet *)palloc(VARHDRSZ+k*VARHDRSZ);
		SET_VARSIZE(r, VARHDRSZ+k*VARHDRSZ);
		memcpy(VARDATA(r),dr,k*VARHDRSZ);
		return r;
	}

}


PG_FUNCTION_INFO_V1(intset_difference);
Datum
intset_difference(PG_FUNCTION_ARGS)
{
	intSet *setA = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(0));
	intSet *setB = (intSet *) PG_DETOAST_DATUM(PG_GETARG_POINTER(1));
	intSet *result;
	result = intset_difference_internal(setA, setB);
	PG_RETURN_POINTER(result);
}
