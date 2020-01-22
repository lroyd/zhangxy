#include <stdio.h>
#include "aki_buffer.h"

T_AkiBuffer	aki = {0};


int test_buffer(void)
{
	char print[512] = {0};
	int size = 1024;
	char *p = (char *)malloc(size);
	char pool_id[5] = {0};
	
	AKI_BufferInit(&aki);
	if (AKI_BufferCreatePool(&aki, p, size, 128, &pool_id[0])){
		printf("error\r\n");
		return 0;
	}
	
	//查看分包个数
	printf("total = %d\r\n", AKI_PoolGetTotal(&aki, pool_id[0]));	
	
	//获取包
	void *pack[10] = {0};
	int i;
	for(i = 0; i < 10; ++i){
		if (pack[i] = AKI_GetPackage(&aki, 100)){
			printf("[%2d].get pack address:%p\r\n", i, pack[i]);
		}
	}
	
	AKI_PutPackage(&aki, pack[2]);
	
	//printf("%s\n", AKI_BufferPrint(&aki, print, 512));
	
	printf("%s\n", AKI_BufferPrintUsage(&aki, print, 512));
	
	printf("%s\n", AKI_PoolStatistics(&aki, pool_id[0], print, 512));

	
	if (p = AKI_BufferDestroyPool(&aki, pool_id[0])){
		printf("free %p\r\n", p);
		free(p);
	}
	
	return 0;
}