#include "common.h"

/* test RAISERROR in a store procedure, from Tom Rogers tests */

/* TODO add support for Sybase */

static char software_version[] = "$Id: raiserror.c,v 1.1.2.1 2004/04/13 17:16:22 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#define SP_TEXT "{?=call #tmp1(?,?,?)}"
#define OUTSTRING_LEN 20

static const char create_proc[] =
	"CREATE PROCEDURE #tmp1\n"
	"    @InParam int,\n"
	"    @OutParam int OUTPUT,\n"
	"    @OutString varchar(20) OUTPUT\n"
	"AS\n"
	"     SET @OutParam = @InParam\n"
	"     SET @OutString = 'This is bogus!'\n" "     RAISERROR('An error occurred.', @InParam, 1)\n" "     RETURN (0)";

static SQLSMALLINT ReturnCode;

static void
TestResult(SQLRETURN result, int level, const char *func)
{
	SQLCHAR SqlState[6];
	SQLINTEGER NativeError;
	SQLCHAR MessageText[1000];
	SQLSMALLINT TextLength;

	if ((level <= 10 && result != SQL_SUCCESS_WITH_INFO) || (level > 10 && result != SQL_ERROR) || ReturnCode != 0) {
		fprintf(stderr, "%s failed!\n", func);
		exit(1);
	}

	SqlState[0] = 0;
	MessageText[0] = 0;
	NativeError = 0;
	/* result = SQLError(SQL_NULL_HENV, SQL_NULL_HDBC, Statement, SqlState, &NativeError, MessageText, 1000, &TextLength); */
	result = SQLGetDiagRec(SQL_HANDLE_STMT, Statement, 1, SqlState, &NativeError, MessageText, 1000, &TextLength);
	printf("Result=%d DIAG REC 1: State=%s Error=%d: %s\n", (int) result, SqlState, (int) NativeError, MessageText);
	if (!SQL_SUCCEEDED(result)) {
		fprintf(stderr, "SQLGetDiagRec error!\n");
		exit(1);
	}

	if (strstr(MessageText, "An error occurred") == NULL) {
		fprintf(stderr, "Wrong error returned!\n");
		fprintf(stderr, "Error returned: %s\n", MessageText);
		exit(1);
	}
}

static void
Test(int level)
{
	SQLRETURN result;
	SQLSMALLINT InParam = level;
	SQLSMALLINT OutParam = 1;
	SQLCHAR OutString[OUTSTRING_LEN];
	SQLINTEGER cbReturnCode = 0, cbInParam = 0, cbOutParam = 0;
	SQLINTEGER cbOutString = SQL_NTS;

	char sql[80];

	ReturnCode = 0;

	/* test with SQLExecDirect */
	sprintf(sql, "RAISERROR('An error occurred.', %d, 1)", level);
	result = CommandWithResult(Statement, sql);

	TestResult(result, level, "SQLExecDirect");

	/* test with SQLPrepare/SQLExecute */
	if (!SQL_SUCCEEDED(SQLPrepare(Statement, (SQLCHAR *) SP_TEXT, strlen(SP_TEXT)))) {
		fprintf(stderr, "SQLPrepare failure!\n");
		exit(1);
	}

	SQLBindParameter(Statement, 1, SQL_PARAM_OUTPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0, &ReturnCode, 0, &cbReturnCode);
	SQLBindParameter(Statement, 2, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0, &InParam, 0, &cbInParam);
	SQLBindParameter(Statement, 3, SQL_PARAM_OUTPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0, &OutParam, 0, &cbOutParam);
	strcpy((char *) OutString, "Test");
	SQLBindParameter(Statement, 4, SQL_PARAM_OUTPUT, SQL_C_CHAR, SQL_VARCHAR, OUTSTRING_LEN, 0, OutString, OUTSTRING_LEN,
			 &cbOutString);

	result = SQLExecute(Statement);

	printf("SpDateTest Output:\n");
	printf("   Result = %d\n", (int) result);
	printf("   Return Code = %d\n", (int) ReturnCode);

	TestResult(result, level, "SQLExecute");
}

int
main(int argc, char *argv[])
{
	Connect();

	if (CommandWithResult(Statement, create_proc) != SQL_SUCCESS) {
		fprintf(stderr, "Unable to create temporary store, probably not mssql (required for this test)\n");
		exit(0);
	}

	Test(5);

	Test(11);

	Disconnect();

	printf("Done.\n");
	return 0;
}
