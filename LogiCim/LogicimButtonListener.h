#ifndef __LOGICIM_BUTTON_LISTENER_H__
#define __LOGICIM_BUTTON_LISTENER_H__
#include "WindowLib/ButtonListener.h"
#include "WindowLib/MenuListener.h"
#include "WindowLib/ListListener.h"


//The window content can't be every kind of listener itself, each is a CListener, so these pass what they hear on.
class CLogicimWindowContent;


class CLogicimButtonListener : public CButtonListener
{
CONSTRUCTABLE(CLogicimButtonListener);
public:
	CLogicimWindowContent*	mpcContent;

	void	Init(CLogicimWindowContent* pcContent);
	void	ButtonClicked(CUnknown* pcSource, void* pvContext) override;
};


class CLogicimMenuListener : public CMenuListener
{
CONSTRUCTABLE(CLogicimMenuListener);
public:
	CLogicimWindowContent*	mpcContent;

	void	Init(CLogicimWindowContent* pcContent);
	void	MenuItemClicked(CUnknown* pcSource, void* pvContext) override;
};


class CLogicimListListener : public CListListener
{
CONSTRUCTABLE(CLogicimListListener);
public:
	CLogicimWindowContent*	mpcContent;

	void	Init(CLogicimWindowContent* pcContent);
	void	SelectionChanged(CUnknown* pcSource, void* pvContext) override;
};


#endif // __LOGICIM_BUTTON_LISTENER_H__

