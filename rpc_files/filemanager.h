/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _FILEMANAGER_H_RPCGEN
#define _FILEMANAGER_H_RPCGEN

#include <rpc/rpc.h>

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VERNUM 1
#define PRINTOPERATIONVER 1
#define PRINTFILEOPERATIONVER 2
#define OPERATION_SIZE 256
#define USERNAME_SIZE 256
#define DATETIME_SIZE 20
#define FILENAME_SIZE 256

typedef char *OPERATION;

typedef char *USERNAME;

typedef char *DATETIME;

typedef char *FILENAME;

struct print_operation_1_argument {
	USERNAME username;
	OPERATION operation;
	DATETIME datetime;
};
typedef struct print_operation_1_argument print_operation_1_argument;

struct print_file_operation_1_argument {
	USERNAME username;
	OPERATION operation;
	FILENAME filename;
	DATETIME datetime;
};
typedef struct print_file_operation_1_argument print_file_operation_1_argument;

#define filemanager 1
#define VERNUM 1

#if defined(__STDC__) || defined(__cplusplus)
#define print_operation PRINTOPERATIONVER
extern  enum clnt_stat print_operation_1(USERNAME , OPERATION , DATETIME , int *, CLIENT *);
extern  bool_t print_operation_1_svc(USERNAME , OPERATION , DATETIME , int *, struct svc_req *);
#define print_file_operation PRINTFILEOPERATIONVER
extern  enum clnt_stat print_file_operation_1(USERNAME , OPERATION , FILENAME , DATETIME , int *, CLIENT *);
extern  bool_t print_file_operation_1_svc(USERNAME , OPERATION , FILENAME , DATETIME , int *, struct svc_req *);
extern int filemanager_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define print_operation PRINTOPERATIONVER
extern  enum clnt_stat print_operation_1();
extern  bool_t print_operation_1_svc();
#define print_file_operation PRINTFILEOPERATIONVER
extern  enum clnt_stat print_file_operation_1();
extern  bool_t print_file_operation_1_svc();
extern int filemanager_1_freeresult ();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_OPERATION (XDR *, OPERATION*);
extern  bool_t xdr_USERNAME (XDR *, USERNAME*);
extern  bool_t xdr_DATETIME (XDR *, DATETIME*);
extern  bool_t xdr_FILENAME (XDR *, FILENAME*);
extern  bool_t xdr_print_operation_1_argument (XDR *, print_operation_1_argument*);
extern  bool_t xdr_print_file_operation_1_argument (XDR *, print_file_operation_1_argument*);

#else /* K&R C */
extern bool_t xdr_OPERATION ();
extern bool_t xdr_USERNAME ();
extern bool_t xdr_DATETIME ();
extern bool_t xdr_FILENAME ();
extern bool_t xdr_print_operation_1_argument ();
extern bool_t xdr_print_file_operation_1_argument ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_FILEMANAGER_H_RPCGEN */