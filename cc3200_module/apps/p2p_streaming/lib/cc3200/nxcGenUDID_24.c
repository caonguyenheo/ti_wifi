/*													*
 * @Name: nxcGenUDID.c								*
 * @Brief: Source file								*
 *  Created on: Dec 18, 2013						*
 ****************************************************
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "nxcGenUDID.h"
#define UDID_UNITTEST			0

/* 	UDID structure:


    24 characters long
    Vendor ID – 2 characters
    Type – 1 character (1 – Test, 0 – Commercial)
    Model Number – 3 characters Serial Number / Mac Address – 12 characters Checksum – 6 characters 
  	<2‐char Vendor ID ><1-char Type><3-char Model Number><12‐char MAC Address><6‐char Checksum>

	eg. 001001112233445566 FHTSVF
    Vendor ID: 00 - SR ? 
    Test type : 1 
    Model :    001 - Mebo / Skypviper ? 
    Mac :   112233445566 
    CS  :    

	Example for different models
	‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐
	Other		  :
	 */

/*@Name:
 *Brief:
 *@Input:
 *@Output:
 *@Return:
 **/

 
// Test vector |VendorID|Type|Model|MAC         |Checksum
//             |01      |1   |001  |E076D0171F8F|LNRASR
//             |01      |1   |001  |E076D0171FAD|DNRASR
//             |01      |1   |001  |E076D0171FC1|PNRASR
//             |01      |1   |001  |E076D0148FEB|JNKASI
//             |01      |1   |001  |E076D0171FA4|INRASR
int inxcGenUDID(unsigned short vendor_id, unsigned short chrTypeNum, char* usModelNum, char* strMAC, char* strUDID){
	int retVal  = 0;

    unsigned char output_hash[32] ={0};
    //Phung: mode the magic number table 
	//static unsigned int mnttable[] = {7,3,7,3,7,2,5,6};
	static unsigned int mnttable[] = {11,13,11,13,11,3,7,17};

	char result[9] = {0};
	char tmp[3] = {0};
	unsigned short a1,a2,a3,a4,a5,a6,m1,m2,a,b,c,d,e,f,g,h;

	if(strMAC == NULL || strUDID == NULL || usModelNum == NULL){
#ifdef DEBUG
		printf("Input data NULL\n");
#endif
		return -1;
	}

	tmp[2]=0;
	tmp[0]= usModelNum[0];
	tmp[1]= usModelNum[1];
	m1 = strtoul(tmp,NULL,16);

	tmp[0]= usModelNum[2];
	tmp[1]= usModelNum[3];
	m2 = strtoul(tmp,NULL,16);

	tmp[0]=strMAC[0];
	tmp[1]=strMAC[1];
	a6 = strtoul(tmp,NULL,16);

	tmp[0]=strMAC[2];
	tmp[1]=strMAC[3];
	a5 = strtoul(tmp,NULL,16);

	tmp[0]=strMAC[4];
	tmp[1]=strMAC[5];
	a4 = strtoul(tmp,NULL,16);

	tmp[0]=strMAC[6];
	tmp[1]=strMAC[7];
	a3 = strtoul(tmp,NULL,16);

	tmp[0]=strMAC[8];
	tmp[1]=strMAC[9];
	a2 = strtoul(tmp,NULL,16);

	tmp[0]=strMAC[10];
	tmp[1]=strMAC[11];
	a1 = strtoul(tmp,NULL,16);

	a = (unsigned short)(((a1+a5) * mnttable[0]) % 26) % 25 + 'A';
	b = (unsigned short)(((a2+a5) * mnttable[1]) % 26) % 25 + 'A';
	c = (unsigned short)(((a3+a5) * mnttable[2]) % 26) % 25 + 'A';
	d = (unsigned short)(((a4+a5) * mnttable[3]) % 26) % 25 + 'A';
	e = (unsigned short)(((a5+a6) * mnttable[4]) % 26) % 25 + 'A';
	f = (unsigned short)(((a3+a4) * mnttable[5]) % 26) % 25 + 'A';
	g = (unsigned short)(((a3+m2) * mnttable[6]) % 26) % 25 + 'A';
	h =	(unsigned short)(((chrTypeNum + m1 + m2 + a1 + a2) * mnttable[7]) % 26)  % 25  + 'A';

    //Phung: only use the 6 chars from here 
	sprintf(result,"%c%c%c%c%c%c",a,b,c,d,e,f);
	sprintf(strUDID,"%02d%01d", vendor_id, chrTypeNum);
	strcat(strUDID,usModelNum);
	strcat(strUDID,strMAC);
	strcat(strUDID,result);

/*
    getSHA265FromUDID(strUDID, 24, output_hash);
    //Print SHA256
    {
        int i = 0;  
        printf("UDID hash:"); 
        for (i = 0; i < 32; i++)
        {
            printf("%02x", output_hash[i]);
        }

        printf("\n"); 
    }*/



#ifdef DEBUG
	printf("a1: %d\na2: %d\na3: %d\na4: %d\na5: %d\na6: %d\nm1: %d\nm2: %d\n",a1,a2,a3,a4,a5,a6,m1,m2);
	printf("a: %d -b: %d -c: %d -d: %d -e: %d -f: %d -g: %d -h: %d\n",a,b,c,d,e,f,g,h);
	printf("Result: %s\nUDID string: %s\n",result,strUDID);
#endif
	return retVal;
}
#if (UDID_UNITTEST)
#define NUM_TEST	5
typedef struct test_uid_t {
	uint16_t vendor;
	uint16_t type;
	char model[4];
	char mac[13];
}test_uid_t;

test_uid_t test_suite[NUM_TEST] = {
             {01      ,1   ,"001"  ,"E076D0171F8F"}
            ,{01      ,1   ,"001"  ,"E076D0171FAD"}
            ,{01      ,1   ,"001"  ,"E076D0171FC1"}
            ,{01      ,1   ,"001"  ,"E076D0148FEB"}
            ,{01      ,1   ,"001"  ,"E076D0171FA4"}
};
char *expected_result[NUM_TEST] = {
	"LNRASR"
	,"DNRASR"
	,"PNRASR"
	,"JNKASI"
	,"INRASR"
};

int unit_test_udid(void) {
	int i;
	char ret_udid[25] = {0};
	for (i = 0; i < NUM_TEST; i++) {
		memset(ret_udid, 0, sizeof(ret_udid));
		if (inxcGenUDID(test_suite[i].vendor, test_suite[i].type, test_suite[i].model, test_suite[i].mac, ret_udid) != 0) {
			return -1;
		}
		if (memcmp(ret_udid + 18, expected_result[i], 6) != 0) {
			return -1;
		}
	}
	return 0;
}
#endif
