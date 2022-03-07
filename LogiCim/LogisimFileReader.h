#ifndef __LOGISIM_FILE_READER_H__
#define __LOGISIM_FILE_READER_H__
#include "CoreLib/XMLFile.h"
#include "CoreLib/Files.h"
#include "LogisimCircuit.h"
#include "LogisimLibrary.h"


class CLogisimFileReader
{
protected:
	CChars					mszFileName;
	CChars					mszDirectory;
	CXMLFile				mcFile;

	CLogisimCircuitList		mlCircuits;
	CLogisimLibraryList		mlLibraries;
	CChars					mszMainCircuitName;

public:
	void			Init(char* szDirectory, char* szFileName);
	void			Kill(void);

	CMarkupTag*		Read(void);
	BOOL			Convert(CMarkupTag* pcContainer);

protected:
	BOOL			ConvertCircuit(CMarkupTag* pcCircuitTag);
	BOOL			ConvertMain(CMarkupTag* pcMainTag);
	BOOL			ConvertWire(CMarkupTag* pcWireTag);
	BOOL			ConvertComponent(CMarkupTag* pcCompTag);
	BOOL			ConvertLibrary(CMarkupTag* pcMainTag);
};


#endif // !__LOGISIM_FILE_READER_H__


