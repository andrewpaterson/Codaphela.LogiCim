#ifndef __LOGISIM_FILE_READER_H__
#define __LOGISIM_FILE_READER_H__
#include "BaseLib/MapStringString.h"
#include "CoreLib/XMLFile.h"
#include "CoreLib/Files.h"
#include "LogisimFacing.h"
#include "LogisimTrigger.h"
#include "LogisimCircuit.h"
#include "LogisimLibrary.h"
#include "LogisimComponentList.h"
#include "LogisimCustomComponent.h"


class CLogisimComponent;
class CLogisimFileReader
{
protected:
	CChars							mszFileName;
	CChars							mszDirectory;
	CXMLFile						mcFile;
	CChars							mszFullPathName;

	CLogisimCircuitList				mlCircuits;
	CLogisimLibraryList				mlLibraries;
	CChars							mszMainCircuitName;
	CLogisimComponentList			mcComponents;
	CLogisimCustomComponentList		mcCustomComponentList;

public:
	BOOL			Init(char* szDirectory, char* szFileName);
	void			Kill(void);

	CMarkupTag*		Read(void);
	BOOL			Convert(CMarkupTag* pcContainer);
	char*			GetFileName(void);

protected:
	BOOL			ConvertCircuit(CMarkupTag* pcCircuitTag);
	BOOL			ConvertMain(CMarkupTag* pcMainTag);
	BOOL			ConvertWire(CMarkupTag* pcWireTag, CLogisimCircuit* pcCircuit);
	BOOL			ConvertComponent(CMarkupTag* pcCompTag, CLogisimCircuit* pcCircuit);
	BOOL			ConvertLibrary(CMarkupTag* pcMainTag);

	BOOL			ConvertATagsToMap(CMapStringString* pcDest, CMarkupTag* pcCompTag);
	BOOL			GetMapValueAsInt(CMarkupTag* pcTag, CMapStringString* pcMap, char* szKey, int* piValue, char* szDefault = NULL);
	BOOL			GetMapValue(CMapStringString* pcMap, char* szKey, char** pszValue, char* szDefault = NULL);
	BOOL			GetMapValueAsFacing(CMapStringString* pcMap, char* szKey, ELogisimFacing* peFacing, char* szDefault = NULL);
	BOOL			GetMapValueAsTrigger(CMapStringString* pcMap, char* szKey, ELogisimTrigger* peTrigger, char* szDefault = NULL);
	BOOL			GetMapValueAsNumericType(CMapStringString* pcMap, char* szKey, ELogisimNumericType* peTrigger, char* szDefault = NULL);
	BOOL			GetMapValueAsSplitterAppear(CMapStringString* pcMap, char* szKey, ELogisimSplitterAppearance* peAppear, char* szDefault = NULL);
	BOOL			GetMapValueAsControlledBufferControl(CMapStringString* pcMap, char* szKey, ELogisimControlledBufferControl* peValue, char* szDefault = NULL);
	BOOL			GetMapValueAsPullResistorPull(CMapStringString* pcMap, char* szKey, ELogisimPullResistorPull* pePull, char* szDefault = NULL);
	BOOL			GetMapValueAsSelectLocation(CMapStringString* pcMap, char* szKey, ELogisimSelectLocation* peSelectLocation, char* szDefault = NULL);
	BOOL			GetMapValueAsGateOut(CMapStringString* pcMap, char* szKey, ELogisimGateOut* peOut, char* szDefault = NULL);
	BOOL			GetMapValueAsAlignment(CMapStringString* pcMap, char* szKey, ELogisimAlignment* peValue, char* szDefault = NULL);
	BOOL			GetMapValueAsDataBus(CMapStringString* pcMap, char* szKey, ELogisimRAMDataBus* peValue, char* szDefault = NULL);
	BOOL			GetMapValueAsHexLong(CMapStringString* pcMap, char* szKey, unsigned long long int* pulliValue, BOOL bInclude0x, char* szDefault = NULL);
	BOOL			GetMapValueAsAppearance(CMarkupTag* pcTag, CMapStringString* pcMap, char* szKey);
	BOOL			GetMapValueAsRGB(CMapStringString* pcMap, char* szKey, uint32* puiRGB, char* szDefault = NULL);
	BOOL			CheckMap(CMarkupTag* pcTag, CMapStringString* pcMap, const char* szFirst, ...);
	BOOL			CheckMap(CMarkupTag* pcTag, CMapStringString* pcMap, CArrayChars* paszKeys);
	BOOL			UnknownTagError(CMarkupTag* pcTag, CMapStringString* pcMap);
	BOOL			IsString(char* szLeft, char* szRight);

	BOOL			CreateTunnel(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreatePullResistor(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateConstant(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateANDGate(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateNANDGate(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateNORGate(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateORGate(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateNOTGate(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateXORGate(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateClock(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateControlledBuffer(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateCounter(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateDecoder(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateDigitalOscilloscope(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateLED(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreatePin(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateProbe(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateRAM(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateROM(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateSplitter(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateText(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateDFlipFlop(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateRandom(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateComparator(CMarkupTag* pcCompTag, SInt2 sLoc);
	BOOL			CreateShiftRegister(CMarkupTag* pcCompTag, SInt2 sLoc);

	BOOL			PopulateGate(CMarkupTag* pcCompTag, CLogisimGate* pcComp, CMapStringString* pcMap);

	BOOL			CreateCustomComponent(CMarkupTag* pcCompTag, SInt2 sLoc, char* szName);

	BOOL			ParseInt2(SInt2* ps, char* sz);
};


#endif // !__LOGISIM_FILE_READER_H__

