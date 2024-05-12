#include "filemanager.h"

bool_t
print_operation_1_svc(USERNAME username, OPERATION operation, DATETIME datetime, int *result,  struct svc_req *rqstp)
{
	printf("%s\t%s\t%s\n", username, operation, datetime);
    *result = 0;
    
	return TRUE;
}

bool_t
print_file_operation_1_svc(USERNAME username, OPERATION operation, FILENAME filename, DATETIME datetime, int *result,  struct svc_req *rqstp)
{
	printf("%s\t%s\t%s\t%s\n", username, operation, filename, datetime);
    *result = 0;

	return TRUE;
}

int
filemanager_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	xdr_free (xdr_result, result);

	/*
	 * Insert additional freeing code here, if needed
	 */

	return 1;
}
