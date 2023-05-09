#pragma once


typedef struct tdstStage
{
	char szName[16];
	char *szWork;
	char *szExec;
	char *szFrom;
	char *szTo;
	char **d_szFiles;
	int lNbFiles;
}
tdstStage;


int fn_lParseStage( char const *szName );
