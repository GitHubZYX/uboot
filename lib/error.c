#include <common.h>

#define ERROR_STR_LEN 60
char error_string[ERROR_STR_LEN] __attribute__ ((section(".data")));

void put_error(char * err_str)
{
	int count=0;


	printf("%s\n",err_str);

	count=strlen(err_str);
	if (count >= ERROR_STR_LEN) {
		count = ERROR_STR_LEN -1;
	}

	memset(error_string,0x0,ERROR_STR_LEN);
	strncpy(error_string, err_str, count);

	error_string[count]= '\0';
}
