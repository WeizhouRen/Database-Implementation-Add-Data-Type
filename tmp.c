// #include "postgres.h"
// #include "fmgr.h"
// #include "libpq/pqformat.h"		/* needed for send/recv functions */
#include "stdint.h"
#include "stdio.h";
int32 *getNumArray(char *nums, int32 size) {
	char *token, *subToken;
	int i, subTokenLen, di = 0;
	int32 *data;
	data = palloc(sizeof(int32) * size);
	token = strtok(nums, ",");
	while (token != NULL) {
		i = 0;
		while (token[i] == 0) i++;
		subTokenLen = strlen(token) - i + 1;
		subToken = palloc(sizeof(char) * subTokenLen);
		memcpy(subToken, &token[i], subTokenLen);
		subToken[subTokenLen] = '\0';
		*(data + di) = atoi(subToken);	
		di++;
		token = strtok(NULL, ",");
	}
	return data;
}

char *toString(int32 *data, int32 size) {
	char *output;
	output = palloc(sizeof(char) * (size + size - 1 + 2) + 1);
	strcpy(output, "{");			// starting with left bracket
	for (int i = 0; i < size; i++) {	// iterate data to concat string
		sprintf(output,"%s%d,",output,data[i]);
	}
	output[strlen(output) - 1] = '}';	// reaplce with right bracket
	return output;
}

void main(void) {
    char *nums, *output;
    int32 *data;
    nums = "4,5";
    data = getNumArray(nums, 2);
    output = toString(data, 2);
    printf("%s", output);
}