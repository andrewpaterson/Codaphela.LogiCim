#ifndef __LOGISIM_FILE_READER_H__
#define __LOGISIM_FILE_READER_H__
#include "BaseLib/MapStringString.h"
#include "BaseLib/XMLFile.h"
#include "BaseLib/Files.h"
#include "BaseLib/Int32Vec2.h"
#include "LogisimFacing.h"
#include "LogisimTrigger.h"
#include "LogisimCircuit.h"
#include "LogisimLibrary.h"
#include "LogisimComponentList.h"
#include "LogisimCustomComponent.h"
#include "LogisimCircuitComponent.h"


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
	CLogisimCircuitComponentList	mcCircuitComponentList;

public:
	bool								Init(char* szDirectory, char* szFileName);
	void								Kill(void);

	CMarkupTag*							Read(void);
	bool								Convert(CMarkupTag* pcContainer);
	char*								GetFileName(void);

	CListTemplate<CLogisimCircuit>*		GetCircuits(void);

protected:
	bool			ConvertCircuit(CMarkupTag* pcCircuitTag);
	bool			ConvertMain(CMarkupTag* pcMainTag);
	bool			ConvertWire(CMarkupTag* pcWireTag, CLogisimCircuit* pcCircuit);
	bool			ConvertComponent(CMarkupTag* pcCompTag, CLogisimCircuit* pcCircuit);
	bool			ConvertLibrary(CMarkupTag* pcMainTag);

	bool			ConvertATagsToMap(CMapStringString* pcDest, CMarkupTag* pcCompTag);
	bool			GetMapValueAsInt(CMarkupTag* pcTag, CMapStringString* pcMap, char* szKey, int32* piValue, char* szDefault = NULL);
	bool			GetMapValue(CMapStringString* pcMap, char* szKey, char** pszValue, char* szDefault = NULL);
	bool			GetMapValueAsFacing(CMapStringString* pcMap, char* szKey, ELogisimFacing* peFacing, char* szDefault = NULL);
	bool			GetMapValueAsTrigger(CMapStringString* pcMap, char* szKey, ELogisimTrigger* peTrigger, char* szDefault = NULL);
	bool			GetMapValueAsNumericType(CMapStringString* pcMap, char* szKey, ELogisimNumericType* peTrigger, char* szDefault = NULL);
	bool			GetMapValueAsSplitterAppear(CMapStringString* pcMap, char* szKey, ELogisimSplitterAppearance* peAppear, char* szDefault = NULL);
	bool			GetMapValueAsControlledBufferControl(CMapStringString* pcMap, char* szKey, ELogisimControlledBufferControl* peValue, char* szDefault = NULL);
	bool			GetMapValueAsPullResistorPull(CMapStringString* pcMap, char* szKey, ELogisimPullResistorPull* pePull, char* szDefault = NULL);
	bool			GetMapValueAsSelectLocation(CMapStringString* pcMap, char* szKey, ELogisimSelectLocation* peSelectLocation, char* szDefault = NULL);
	bool			GetMapValueAsGateOut(CMapStringString* pcMap, char* szKey, ELogisimGateOut* peOut, char* szDefault = NULL);
	bool			GetMapValueAsAlignment(CMapStringString* pcMap, char* szKey, ELogisimAlignment* peValue, char* szDefault = NULL);
	bool			GetMapValueAsDataBus(CMapStringString* pcMap, char* szKey, ELogisimRAMDataBus* peValue, char* szDefault = NULL);
	bool			GetMapValueAsHexLong(CMapStringString* pcMap, char* szKey, uint64* pulliValue, bool bInclude0x, char* szDefault = NULL);
	bool			GetMapValueAsAppearance(CMarkupTag* pcTag, CMapStringString* pcMap, char* szKey);
	bool			GetMapValueAsRGB(CMapStringString* pcMap, char* szKey, uint32* puiRGB, char* szDefault = NULL);
	bool			CheckMap(CMarkupTag* pcTag, CMapStringString* pcMap, const char* szFirst, ...);
	bool			CheckMap(CMarkupTag* pcTag, CMapStringString* pcMap, CArrayChars* paszKeys);
	bool			UnknownTagError(CMarkupTag* pcTag, CMapStringString* pcMap);
	bool			IsString(char* szLeft, char* szRight);

protected:
	CLogisimTunnel*					CreateTunnel(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimPullResistor*			CreatePullResistor(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimConstant*				CreateConstant(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimANDGate*				CreateANDGate(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimNANDGate*				CreateNANDGate(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimNORGate*				CreateNORGate(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimORGate*					CreateORGate(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimXORGate*				CreateXORGate(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimNOTGate*				CreateNOTGate(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimClock*					CreateClock(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimControlledBuffer*		CreateControlledBuffer(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimCounter*				CreateCounter(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimDecoder*				CreateDecoder(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimDigitalOscilloscope*	CreateDigitalOscilloscope(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimLED*					CreateLED(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimPin*					CreatePin(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimProbe*					CreateProbe(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimRAM*					CreateRAM(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimROM*					CreateROM(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimSplitter*				CreateSplitter(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimText*					CreateText(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimDTypeFlipFlop*			CreateDFlipFlop(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimRandom*					CreateRandom(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimComparator*				CreateComparator(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimShiftRegsiter*			CreateShiftRegister(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimBuffer*					CreateBuffer(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimBitAdder*				CreateBitAdder(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimRegister*				CreateRegister(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);
	CLogisimMultiplexer*			CreateMultiplexer(CMarkupTag* pcCompTag, SInt32Vec2 sLoc);

	CLogisimCustomComponent*		CreateCustomComponent(CMarkupTag* pcCompTag, SInt32Vec2 sLoc, char* szName);
	CLogisimCircuitComponent*		CreateCircuitComponent(CMarkupTag* pcCompTag, SInt32Vec2 sLoc, char* szName);

protected:
	bool					PopulateGate(CMarkupTag* pcCompTag, CLogisimGate* pcComp, CMapStringString* pcMap);
	bool					ParseInt2(SInt32Vec2* ps, char* sz);
	CLogisimComponent*		KillComponent(CLogisimComponent* pcComp);
};


#endif // __LOGISIM_FILE_READER_H__

