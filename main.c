#include "framework.h"
#include "fedex.h"
#include "stage.h"
#include "perf.h"


char g_szFileName[MAX_PATH] = "";
char g_szSaveWorkDir[MAX_PATH] = "";

bool g_bVerbose = FALSE;


void fn_vPrintVersionInfo( void )
{
	printf("FeDEx - Fetch that Damn Executable, Version %s [%s]\n\n",
		   C_szVersion, C_szDate);
}

void fn_vPrintUsage( void )
{
	printf(
		"Usage: fedex [options] [configPath]\n"
		"\tconfigPath - optional path to the configuration file.\n"
		"\tDefault is 'fedex.ini' in the current directory.\n"
		"Options:\n"
		"\t/? - show usage\n"
		"\t/v - verbose output\n\n"
	);
}

void fn_vSaveCurrentWorkDir( void )
{
	GetCurrentDirectoryA(MAX_PATH, g_szSaveWorkDir);
}

void fn_vRestoreWorkDir( void )
{
	if ( *g_szSaveWorkDir )
		SetCurrentDirectoryA(g_szSaveWorkDir);
}

bool fn_bSetIniPath( char const *szNewPath )
{
	char szTmp[MAX_PATH];

	if ( strlen(szNewPath) > MAX_PATH - 1 )
		return false;

	if ( !PathCombineA(szTmp, g_szSaveWorkDir, szNewPath) )
		return false;

	strcpy(g_szFileName, szTmp);
	return true;
}

void fn_vParseArgs( int argc, char **argv )
{
	if ( argc <= 1 ) /* no args */
		return;

	bool bGotPath = false;

	for ( int i = 1; i < argc; ++i )
	{
		/* usage */
		if ( !_stricmp(argv[i], "/?") || !_stricmp(argv[i], "-h") )
		{
			fn_vPrintUsage();
			exit(0);
		}
		/* verbose switch */
		else if ( !_stricmp(argv[i], "/v") || !_stricmp(argv[i], "-v") )
		{
			g_bVerbose = true;
		}
		/* config path */
		else if ( !bGotPath )
		{
			if ( fn_bSetIniPath(argv[i]) )
				bGotPath = true;
			else
			{
				fn_vPrintUsage();
				eprintf("Error: invalid path\n");
				exit(Err_CfgPathError);
			}
		}
		else
			eprintf("Warning: unknown option '%s'\n", argv[i]);
	}
}

int main( int argc, char **argv )
{
	fn_vPrintVersionInfo();

	fn_vStartTimer();
	fn_vSaveCurrentWorkDir();

	fn_vParseArgs(argc, argv);

	if ( !*g_szFileName )
		fn_bSetIniPath("fedex.ini");
	
	M_VerbosePrintf("Using config file '%s'\n", g_szFileName);

	if ( GetFileAttributesA(g_szFileName) == INVALID_FILE_ATTRIBUTES )
	{
		fn_vPrintUsage();
		eprintf("Error: cannot open configuration file '%s'\n", g_szFileName);
		return Err_CfgAccessError;
	}

	int i;
	for ( i = 0; i < 99; ++i )
	{
		char szTmp[16];
		char szSection[16];
		sprintf(szSection, "Stage%d", i);

		if ( M_GetIniString(szSection, NULL, szTmp, sizeof(szTmp)) == 0 )
			break; /* no more stages */

		int lResult = fn_lParseStage(szSection);
		fn_vRestoreWorkDir();

		if ( lResult != Err_OK )
		{
			float xTime = fn_xStopTimer();
			eprintf("Error: %s failed, processing stopped (%.3f ms)\n", szSection, xTime);
			return lResult;
		}
	}

	if ( !i )
	{
		fn_vPrintUsage();
		eprintf("Error: no stages defined in configuration file or Stage0 missing\n");
		return Err_BadConfig;
	}

	float xTime = fn_xStopTimer();
	printf("Done (%.3f ms)\n", xTime);

	return Err_OK;
}
