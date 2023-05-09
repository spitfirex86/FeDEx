#include "framework.h"
#include "fedex.h"
#include "stage.h"


tdstStage * fn_p_stAllocStage( char const *szName )
{
	tdstStage *p_stStage = calloc(1, sizeof(tdstStage));
	strcpy(p_stStage->szName, szName);

	return p_stStage;
}

void fn_vFreeStage( tdstStage *p_stStage )
{
	free(p_stStage->szWork);
	free(p_stStage->szFrom);
	free(p_stStage->szTo);
	free(p_stStage->szExec);

	for ( int i = 0; i < p_stStage->lNbFiles; ++i )
		free(p_stStage->d_szFiles[i]);

	free(p_stStage->d_szFiles);

	free(p_stStage);
}

char * fn_p_szMakePath( char const *szPath, bool bAbsolute )
{
	char szBuffer[MAX_PATH];
	char szBuffer2[MAX_PATH];
	char *pUseBuf = szBuffer;

	if ( strlen(szPath) > MAX_PATH - 1 )
		return NULL;

	if ( ExpandEnvironmentStringsA(szPath, pUseBuf, MAX_PATH) > MAX_PATH - 1 )
		return NULL;

	if ( bAbsolute )
	{
		if ( !GetFullPathNameA(pUseBuf, MAX_PATH, szBuffer2, NULL) )
			return NULL;
		pUseBuf = szBuffer2;
	}

	PathRemoveBackslashA(pUseBuf);

	char *szOut = malloc(strlen(pUseBuf) + 1);
	strcpy(szOut, pUseBuf);

	return szOut;
}

int fn_lParseFileList( char const *szFiles, tdstStage *pStage )
{
	char **a_szFiles = NULL;
	int lNbFiles = 0;

	char const *pCh = szFiles;
	while ( *pCh )
	{
		unsigned int ulLength = strcspn(pCh, ",");

		char *szOneFile = malloc(ulLength + 1);
		strncpy(szOneFile, pCh, ulLength);
		szOneFile[ulLength] = '\0';

		int i = lNbFiles++;
		a_szFiles = realloc(a_szFiles, lNbFiles * sizeof(char *));
		a_szFiles[i] = szOneFile;

		pCh += ulLength;
		pCh += strspn(pCh, ",");
	}

	pStage->d_szFiles = a_szFiles;
	pStage->lNbFiles = lNbFiles;
	return lNbFiles;
}

int fn_lParseStageCfg( tdstStage *pStage )
{
	char *szName = pStage->szName;
	char szBuffer[512];

	M_GetIniString(szName, "Comment", szBuffer, sizeof(szBuffer));
	if ( *szBuffer )
		printf("(%s) %s\n", szName, szBuffer);
	else
		printf("(%s) Running %s\n", szName, szName);

	M_GetIniString(szName, "WorkDir", szBuffer, sizeof(szBuffer));
	pStage->szWork = fn_p_szMakePath(szBuffer, true);
	if ( !pStage->szWork )
	{
		eprintf("(%s) Error: working directory path is invalid\n", szName);
		return Err_BadPath;
	}
	M_VerbosePrintf("\tWorkDir: '%s'\n", pStage->szWork);

	if ( !SetCurrentDirectoryA(pStage->szWork) )
	{
		eprintf("(%s) Error: could not set working directory\n", szName);
		return Err_BadPath;
	}

	M_GetIniString(szName, "CopyFrom", szBuffer, sizeof(szBuffer));
	pStage->szFrom = fn_p_szMakePath(szBuffer, false);
	if ( !pStage->szFrom )
	{
		eprintf("(%s) Error: CopyFrom: invalid path\n", szName);
		return Err_BadPath;
	}
	M_VerbosePrintf("\tCopyFrom: '%s'\n", pStage->szFrom);


	M_GetIniString(szName, "CopyTo", szBuffer, sizeof(szBuffer));
	pStage->szTo = fn_p_szMakePath(szBuffer, false);
	if ( !pStage->szTo )
	{
		eprintf("(%s) Error: CopyTo: invalid path\n", szName);
		return Err_BadPath;
	}
	M_VerbosePrintf("\tCopyTo: '%s'\n", pStage->szTo);

	M_GetIniString(szName, "Exec", szBuffer, sizeof(szBuffer));
	if ( *szBuffer )
	{
		char *szExec = malloc(strlen(szBuffer) + 1);
		strcpy(szExec, szBuffer);
		pStage->szExec = szExec;
	}
	M_VerbosePrintf("\tExec: '%s'\n", pStage->szExec);

	M_GetIniString(szName, "Files", szBuffer, sizeof(szBuffer));
	if ( *szBuffer )
	{
		if ( fn_lParseFileList(szBuffer, pStage) < 0 )
			return Err_BadConfig;
	}
	M_VerbosePrintf("\tFiles: %d\n", pStage->lNbFiles);

	return Err_OK;
}

int fn_lPreRunStage( tdstStage *pStage )
{
	if ( GetFileAttributesA(pStage->szTo) == INVALID_FILE_ATTRIBUTES
		&& !CreateDirectoryA(pStage->szTo, NULL) )
	{
		M_VerbosePrintf("LastError: %lu\n", GetLastError());
		eprintf("(%s) Error: could not create directory '%s'\n", pStage->szName, pStage->szTo);
		return Err_DirError;
	}

	if ( !pStage->szExec && !pStage->d_szFiles )
	{
		eprintf("(%s) Error: stage has no actions\n", pStage->szName);
		return Err_NothingToDo;
	}

	return Err_OK;
}

int fn_lRunStage( tdstStage *pStage )
{
	STARTUPINFO stStartupInfo = { 0 };
	PROCESS_INFORMATION stProcessInfo = { 0 };

	if ( pStage->szExec )
	{
		printf("(%s) Executing '%s'\n", pStage->szName, pStage->szExec);

		BOOL bResult = CreateProcessA(
			NULL, pStage->szExec,
			NULL, NULL,
			FALSE, 0,
			NULL,
			pStage->szWork,
			&stStartupInfo, &stProcessInfo
		);

		if ( !bResult )
		{
			M_VerbosePrintf("LastError: %lu\n", GetLastError());
			eprintf("(%s) Error: exec command failed to launch\n", pStage->szName);
			return Err_ExecError;
		}

		WaitForSingleObject(stProcessInfo.hProcess, INFINITE);

		CloseHandle(stProcessInfo.hProcess);
		CloseHandle(stProcessInfo.hThread);
	}

	if ( pStage->d_szFiles )
	{
		char szFromPath[512];
		char szToPath[512];
		int lResult = Err_OK;

		printf("(%s) Copying files...\n", pStage->szName);

		for ( int i = 0; i < pStage->lNbFiles; ++i )
		{
			char *szFile = pStage->d_szFiles[i];
			char *pBackSlash = strrchr(szFile, '\\');
			char *pOutName = (pBackSlash) ? pBackSlash + 1 : szFile;

			sprintf(szFromPath, "%s\\%s", pStage->szFrom, szFile);
			sprintf(szToPath, "%s\\%s", pStage->szTo, pOutName);

			printf("\t'%s' -> '%s'\n", szFromPath, szToPath);

			if ( strlen(szFromPath) > MAX_PATH - 1
				|| strlen(szToPath) > MAX_PATH - 1 )
			{
				eprintf("(%s) Error: file '%s': path too long\n", pStage->szName, szFile);
				lResult = Err_BadPath;
				continue;
			}

			if ( !CopyFileA(szFromPath, szToPath, false) )
			{
				M_VerbosePrintf("LastError: %lu\n", GetLastError());
				eprintf("(%s) Error: file '%s': copy operation failed\n", pStage->szName, szFile);
				lResult = Err_CopyError;
			}
		}

		return lResult;
	}

	return Err_OK;
}

int fn_lParseStage( char const *szName )
{
	tdstStage *pStage = fn_p_stAllocStage(szName);
	M_VerbosePrintf("Begin parsing '%s'\n", szName);

	int lResult = fn_lParseStageCfg(pStage);
	if ( lResult != Err_OK )
		goto CleanUp;

	lResult = fn_lPreRunStage(pStage);
	if ( lResult != Err_OK )
		goto CleanUp;

	lResult = fn_lRunStage(pStage);

CleanUp:
	fn_vFreeStage(pStage);
	return lResult;
}
