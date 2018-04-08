#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <pthread.h>
#include "ip_over_spi.h"

/**Common function**/
int array2byte_2_int(unsigned char *array)
{
	return ((int)array[0]<<8 | (int)array[1]);
}
int int_2_array2byte(int num, unsigned char *array)
{
	array[0] = (num>>8) & 0xff;
	array[1] = num & 0xff;
	return 0;
}
void buffer_debug(char *a, int len)
{
	int i;
	for(i=0; i<len; i++)
	{
		printf("%x ", a[i]);
	}
	printf("\n");
}
int ios_packet_create(IPOVERSPI_TRANSFER * data, char type, char id, char control, char err_code, char direction, int len, char fragment, char *buffer)
{
	data->transport_type = type;;
        data->transport_id = id;
        data->transport_control = control;
        data->transport_err_code = err_code;
	data->transport_direction = direction;
        int_2_array2byte(len, (unsigned char *)data->data_length);
        data->fragment_flag = fragment;
	if((len>0)&&(buffer))
		memcpy(data->transport_data, buffer, (len>TRANSPORT_DATA_LENGTH)?TRANSPORT_DATA_LENGTH:len);
	return 0;
}
int ios_packet_parse(IPOVERSPI_TRANSFER *data, char *type, char *id, char *control, char *err_code, char *direction, int *len, char *fragment, char **buffer)
{
	*type = data->transport_type;
	*id = data->transport_id;
	*control = data->transport_control;
	*err_code = data->transport_err_code;
	*direction = data->transport_direction;
	//printf(" [%x][%x]\n", data->data_length[0], data->data_length[1]);
	*len = array2byte_2_int((unsigned char *)data->data_length);
	//printf("    %d\n", *len);
	*fragment = data->fragment_flag;
	*buffer = data->transport_data;
	return 0;
}
