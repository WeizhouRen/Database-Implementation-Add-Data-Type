/*
 * src/tutorial/intset.c
 *
 ******************************************************************************
 This file contains routines that can be bound to a Postgres backend and
 called by the backend in the process of processing queries.  The calling
 format for these routines is dictated by Postgres architecture.
 ******************************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include "stdint.h"

PG_MODULE_MAGIC;

typedef struct IntSet
{
	int32		length; // struct length
	int32		size;	// array size
	int32		data[FLEXIBLE_ARRAY_MEMBER]; // actual length of the data part is not specified
} IntSet;

// FUNCTION DECLARATIONS

bool check_nums(char *str); 
bool is_valid_input(char *str);
char *extract_nums(char *tmp);
int32 *get_num_array(char *nums, int32 *size);
char *to_string(int32 *data, int32 size); 
int32 find_insert_pos(int32 *data, int32 target, int32 size);
bool num_exist(int32 *data, int32 target, int32 size);
int32 *insert_num(int32 *data, int32 size, int32 num, int32 pos);
bool is_subset(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB);
bool is_equal(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB);
int32 *get_intersection(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize);
int32 *get_union(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize);
int32 *get_disjunction(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize);
int32 *get_difference(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize);

/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/


PG_FUNCTION_INFO_V1(intset_in);

	Datum
intset_in(PG_FUNCTION_ARGS)
{
	char	*str = PG_GETARG_CSTRING(0);
	// int32	length;			// length of struct
	int32 	size = 0;			// size of array
	int32 	*data;
	// char 	*tmp;	// copy input string to tmp 
	char	*nums;				// string of numbers with comma
	IntSet	*result;			// result of IntSet to be stored
	//	 char 	*debug;

	// tmp = palloc(strlen(str));
	// strcpy(tmp, str);
	
	if (!is_valid_input(str))
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				errmsg("invalid input syntax for type %s: \"%s\"",
					"intset", str)));

	// Remove brackets and spaces
	nums = extract_nums(str);

	// Get list of numbers
	data = get_num_array(nums, &size);
	// ereport(ERROR,
	// 		(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
	// 			errmsg("size = %d", size)));
	// Remove duplicates
	// Sort in asc
	// Memory allocation  VARHDRSZ is sizeof(int32)
	// length = (size + 2) * sizeof(int32);
	result = (IntSet *) palloc((size + 2) * sizeof(int32));
	SET_VARSIZE(result, (size + 2) * sizeof(int32));

	result->size = size;
	memcpy(result->data, data, size * sizeof(int32));

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(intset_out);

	Datum
intset_out(PG_FUNCTION_ARGS)
{
	IntSet    *intSet = (IntSet *) PG_GETARG_POINTER(0);
	char	  *result;
	result = to_string(intSet->data, intSet->size);

	//	result = psprintf("{%d}", intSet->data[0]);
	//	result = psprintf("{%d,%d}", intSet->length, intSet->size);
	PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * New Operators
 *
 * A practical intset datatype would provide much more than this, of course.
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_contains);

Datum
intset_contains(PG_FUNCTION_ARGS)
{
	int32	  num = PG_GETARG_INT32(0);
	IntSet    *intSet = (IntSet *) PG_GETARG_POINTER(1);
	int32	  *data = intSet->data;
	int32 	  size = intSet->size;
	bool 	  result = num_exist(data, num, size);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(get_cardinality);

Datum
get_cardinality(PG_FUNCTION_ARGS)
{
	IntSet	  *intSet = (IntSet *) PG_GETARG_POINTER(0);
	int32	  result = intSet->size;

	PG_RETURN_INT32(result);
}

PG_FUNCTION_INFO_V1(contains_all);

Datum
contains_all(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);

	bool 	  result = is_subset(setA->data, setA->size, setB->data, setB->size);
	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(contains_only);

Datum
contains_only(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);

	bool 	  result = is_subset(setB->data, setB->size, setA->data, setA->size);
	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(equal);

Datum
equal(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);

	bool 	  result = is_equal(setB->data, setB->size, setA->data, setA->size);
	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(not_equal);

Datum
not_equal(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);

	bool 	  result = is_equal(setB->data, setB->size, setA->data, setA->size);
	PG_RETURN_BOOL(!result);
}


PG_FUNCTION_INFO_V1(intersection);

Datum
intersection(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);
	int32     size = 0;
	int32 	  *data;
	IntSet	  *result;

	data = get_intersection(setB->data, setB->size, setA->data, setA->size, &size);

	result = (IntSet *) palloc((size + 2) * sizeof(int32));
	SET_VARSIZE(result, (size + 2) * sizeof(int32));
	result->size = size;
	memcpy(result->data, data, size * sizeof(int32));

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(union_set);

Datum
union_set(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);
	int32     size = 0;
	int32 	  *data;
	IntSet	  *result;

	data = get_union(setA->data, setA->size, setB->data, setB->size, &size);

	result = (IntSet *) palloc((size + 2) * sizeof(int32));
	SET_VARSIZE(result, (size + 2) * sizeof(int32));
	result->size = size;
	memcpy(result->data, data, size * sizeof(int32));

	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(disjunction);

Datum
disjunction(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);
	int32     size = 0;
	int32 	  *data;
	IntSet	  *result;

	data = get_disjunction(setA->data, setA->size, setB->data, setB->size, &size);

	result = (IntSet *) palloc((size + 2) * sizeof(int32));
	SET_VARSIZE(result, (size + 2) * sizeof(int32));
	result->size = size;
	memcpy(result->data, data, size * sizeof(int32));

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(difference);

Datum
difference(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);
	int32     size = 0;
	int32 	  *data;
	IntSet	  *result;

	data = get_difference(setA->data, setA->size, setB->data, setB->size, &size);

	result = (IntSet *) palloc((size + 2) * sizeof(int32));
	SET_VARSIZE(result, (size + 2) * sizeof(int32));
	result->size = size;
	memcpy(result->data, data, size * sizeof(int32));

	PG_RETURN_POINTER(result);
}


/*****************************************************************************
 * Helper functions
 *****************************************************************************/


/**
 * Check if every token is an integer after removing heading and tailing space
 * @return bool
 */
bool check_nums(char *str) {
	char *token = strtok(str, ",");

	while (token != NULL) {
		int i = 0, j = strlen(token) - 1;
		while (isspace(token[i])) i++;
		while (isspace(token[j])) j--;
		if (i > j) return false;
		for (int x = i; x < j; x++) {
			if (!isdigit(token[x])) return false;
		}
		token = strtok(NULL, ",");
	}
	return true;
}


/**
 * Check if the input is valid
 * @param string
 * @return boolean
 */
bool is_valid_input(char *str) {
	int i = 0, j = strlen(str) - 1, s = 0, x = 0;
	char *numList, *token;
	while (isspace(str[i])) i++;
	while (isspace(str[j])) j--;
	if (str[i] != '{' || str[j] != '}') return false;
	i++;
	j--;
	for (;x < strlen(str); x++) {
		if (str[x] != ' ') str[s++] = str[x];
	}
	str[s] = '\0';

	if (strstr(str, ",,")) return false;
	if (strcmp(str, "{}") == 0) return true;
	numList = malloc((s - 2) * sizeof(char));
	memcpy(numList, &str[1], s - 2);
	numList[s - 2] = '\0';
	if (!isdigit(numList[0]) || !isdigit(numList[s - 3])) return false;

	token = strtok(numList, ",");
	while (token != NULL) {
		printf("%s\n", token);
		for (int x = 0; x < strlen(token); x++) {
			if (!isdigit(token[x])) return false;
		}
		token = strtok(NULL, ",");
	}
	return true;
}


char *extract_nums(char *tmp) {

	int i = 0, j = strlen(tmp) - 1, s = 0, subLen = 0;
	char *nums = NULL, *subStr = NULL;
	// Remove leading and tailing spaces
	while (isspace(tmp[i])) i++;
	while (isspace(tmp[j])) j--;

	// Remove brackets
	i++;
	j--;
	subLen = j - i + 2;
	subStr = palloc(subLen * sizeof(char));
	memcpy(subStr, &tmp[i], subLen - 1);
	subStr[subLen - 1] = '\0';

	// Remove internal spaces
	
	for (i = 0; i < subLen - 1; i++) {
		if (subStr[i] != ' ')
			subStr[s++] = subStr[i];
	}
	
	nums = palloc((s + 1) * sizeof(char));
	memcpy(nums, &subStr[0], s);
	nums[s] = '\0';
	return nums;
}

bool num_exist(int32 *data, int32 target, int32 size) {
	int32 l = 0, r = size - 1, m;

	while (l <= r) {
		m = l + (r - l) / 2;
		if (data[m] == target) return true;
		else if (data[m] < target) l = m + 1;
		else r = m - 1;
	}
	return false;
}

int32 find_insert_pos(int32 *data, int32 target, int32 size) {
	int32 l = 0, r = size - 1, m;

	while (l <= r) {
		m = l + (r - l) / 2;
		if (data[m] < target) l = m + 1;
		else r = m - 1;
	}
	return l;
}


int32 *insert_num(int32 *data, int32 size, int32 num, int32 pos) {
	
	int32 *newData;
	newData = repalloc(data, sizeof(int32) * size);

	for (int32 i = size - 1; i > pos; i--) {
		newData[i] = data[i - 1];
	}
	newData[pos] = num;
	return newData;
}

int32 *get_num_array(char *nums, int32 *size) {
	char *token, *subToken;
	int32 i = 0, 
		subTokenLen = 0,
		num = 0,
		pos = 0,
		tmpSize = 0,
		*data = NULL,
		*newData = NULL;

	token = strtok(nums, ",");
	while (token != NULL) {
		i = 0;
		while (token[i] == 0) i++;
		subTokenLen = strlen(token) - i + 1;
		subToken = palloc(sizeof(char) * subTokenLen);
		memcpy(subToken, &token[i], subTokenLen - 1);
		subToken[subTokenLen - 1] = '\0';
		num = atoi(subToken);

		if (tmpSize == 0) {	// add first num to array
			newData = palloc(sizeof(int32));
			newData[tmpSize++] = num;
		} else {
			// if num already exist in array, jump to next turn
			if (num_exist(data, num, tmpSize)) {
				token = strtok(NULL, ",");
				continue;
			}
			// otherwise find expected position to insert
			pos = find_insert_pos(newData, num, tmpSize);
			newData = insert_num(newData, ++tmpSize, num, pos);
		}
		data = newData;	
		token = strtok(NULL, ",");
	}
	*size = tmpSize;
	return data;
}

char *to_string(int32 *data, int32 size) {
	char *output;
	
	if (size == 0) {
		output = palloc(sizeof(char) * 3);
		strcpy(output, "{}");
		output[2] = '\0';
	} else {
		output = palloc(sizeof(char) * (size + size - 1 + 2 + 1));
		strcpy(output, "{");				// starting with left bracket
		for (int i = 0; i < size; i++) {	// iterate data to concat string
			sprintf(output,"%s%d,",output,data[i]);
		}
		output[strlen(output) - 1] = '}';	// reaplce with right bracket
		// output[2 * size] = '}';
		// output[2 * size + 1] = '\0';
	}
	return output;
}

/**
 * Check if intSet A contain all the values in intSet B
 * for every element of B, it is an element of A
 * i.e. A >@ B
 * 
 * @return bool
 */
bool is_subset(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB) {
	for (int i = 0; i < sizeB; i++) {
		// do binary search
		if (!num_exist(dataA, dataB[i], sizeA)) return false;
	}
	return true;
}

bool is_equal(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB) {
	if (sizeA != sizeB) return false;
	for (int i = 0; i < sizeA; i++) {
		if (dataA[i] != dataB[i]) return false;
	}
	return true;
}

int32 *get_intersection(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize) {
	int32 *intersection = NULL, size = 0;

	for (int i = 0; i < sizeA; i++) {
		if (!num_exist(dataB, dataA[i], sizeB)) continue;
		if (size == 0) {
			intersection = palloc(sizeof(int32));
			intersection[size++] = dataA[i];
		} else {
			size++;
			intersection = insert_num(intersection, size, dataA[i], size - 1);
		}
	}
	*newSize = size;
	return intersection;
}

int32 *get_union(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize) {
	int32 size = sizeB, *unionSet, pos;
	unionSet = palloc(sizeof(int32) * size);
	for (int j = 0; j < sizeB; j++) {
		unionSet[j] = dataB[j];
	}

	for (int i = 0; i < sizeA; i++) {
		if (num_exist(unionSet, dataA[i], size)) continue;
		pos = find_insert_pos(unionSet, dataA[i], size);
		unionSet = insert_num(unionSet, ++size, dataA[i], pos);
	}

	*newSize = size;
	return unionSet;
}

int32 *get_disjunction(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize) {
	int32 interSize = 0, unionSize = 0, size = 0;
	int32 *interSet, *unionSet, *disSet;
	char *uni;
	interSet = get_intersection(dataA, sizeA, dataB, sizeB, &interSize);
	
	unionSet = get_union(dataA, sizeA, dataB, sizeB, &unionSize);
	uni = to_string(unionSet, unionSize);
	ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("union: %s\n", uni)));

	for(int i = 0; i < unionSize; i++) {
		if (num_exist(interSet, unionSet[i], interSize)) continue;
		if (size == 0) {
			disSet = palloc(sizeof(int32));
			disSet[size++] = unionSet[i];
		} else {
			size++;
			disSet = insert_num(disSet, size, unionSet[i], size - 1);
		}
	}
	*newSize = size;
	return disSet;
}

int32 *get_difference(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize) {
	int32 size = 0, *diffSet = NULL;

	for(int i = 0; i < sizeA; i++) {
		if (num_exist(dataB, dataA[i], sizeB)) continue;
		if (size == 0) {
			diffSet = palloc(sizeof(int32));
			diffSet[size++] = dataA[i];
		} else {
			size++;
			diffSet = insert_num(diffSet, size, dataA[i], size - 1);
		}
	}
	*newSize = size;
	return diffSet;
}
