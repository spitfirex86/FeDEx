#pragma once


#define C_szVersion		"1.0.1"
#define C_szDate		__DATE__


extern char g_szFileName[MAX_PATH];

#define M_GetIniString(szSection, szKey, szOut, ulSize) \
	GetPrivateProfileStringA(szSection, szKey, NULL, szOut, ulSize, g_szFileName)

#define eprintf(...) fprintf(stderr, __VA_ARGS__)


extern bool g_bVerbose;

#define M_VerbosePrintf(...) do {	\
	if ( g_bVerbose )				\
		printf(__VA_ARGS__);		\
} while ( 0 )


enum tdeExitCode
{
	Err_OK = 0,
	Err_GenericError,

	Err_CfgAccessError,
	Err_CfgPathError,

	Err_BadConfig,
	Err_BadPath,
	Err_DirError,
	Err_ExecError,
	Err_CopyError,
	Err_NothingToDo
};
