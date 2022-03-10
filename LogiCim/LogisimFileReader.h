#ifndef __LOGISIM_FILE_READER_H__
#define __LOGISIM_FILE_READER_H__
#include "BaseLib/MapStringString.h"
#include "CoreLib/XMLFile.h"
#include "CoreLib/Files.h"
#include "LogisimFacing.h"
#include "LogisimCircuit.h"
#include "LogisimLibrary.h"
#include "LogisimComponentList.h"


class CLogisimComponent;
class CLogisimFileReader
{
protected:
	CChars					mszFileName;
	CChars					mszDirectory;
	CXMLFile				mcFile;

	CLogisimCircuitList		mlCircuits;
	CLogisimLibraryList		mlLibraries;
	CChars					mszMainCircuitName;
	CLogisimComponentList	mcComponents;

public:
	void			Init(char* szDirectory, char* szFileName);
	void			Kill(void);

	CMarkupTag*		Read(void);
	BOOL			Convert(CMarkupTag* pcContainer);

protected:
	BOOL			ConvertCircuit(CMarkupTag* pcCircuitTag);
	BOOL			ConvertMain(CMarkupTag* pcMainTag);
	BOOL			ConvertWire(CMarkupTag* pcWireTag, CLogisimCircuit* pcCircuit);
	BOOL			ConvertComponent(CMarkupTag* pcCompTag, CLogisimCircuit* pcCircuit);
	BOOL			ConvertLibrary(CMarkupTag* pcMainTag);

	BOOL			ConvertATagsToMap(CMapStringString* pcDest, CMarkupTag* pcCompTag);
	BOOL			GetMapValueAsInt(CMapStringString* pcMap, char* szKey, int* piValue, char* szDefault = NULL);
	BOOL			GetMapValue(CMapStringString* pcMap, char* szKey, char** pszValue, char* szDefault = NULL);
	BOOL			GetMapValueAsFacing(CMapStringString* pcMap, char* szKey, ELogisimFacing* peFacing, char* szDefault = NULL);

	BOOL			CreateTunnel(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreatePullResistor(CMarkupTag* pcCompTag, SInt2 sLoc);

	BOOL			ParseInt2(SInt2* ps, char* sz);
};


#endif // !__LOGISIM_FILE_READER_H__


