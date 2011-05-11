#ifndef TRACKER_MENUS_H
#define TRACKER_MENUS_H

#include "NMenu.h"

#include <Entry.h>
#include <Messenger.h>
#include <String.h>

#include "NavMenu.h"

const uint32 T_BRING_HERE = 'tbah';

class TDockWindowMenuItem : public TBitmapMenuItem
{
public:
	TDockWindowMenuItem( TMenu *menu, int32 window_id, bool mini, bool currentWs, uint32 workspace );

	virtual void GetContentSize( float *width, float *height );
	virtual void DrawContent();

protected:
	virtual status_t Invoke( BMessage *message = NULL );

private:
	int32 fID;
	bool fMini;
	bool fCurrentWS;
	char fWorkspaceString[8];
	const BBitmap *fWorkspaceBitmap;
};

class TDockInnerWindowMenuItem : public TBitmapMenuItem
{
public:
	TDockInnerWindowMenuItem( const char *label, BBitmap *bitmap, int32 action, int32 window_id, team_id teamid, const char *name );

protected:
	virtual status_t Invoke( BMessage *message = NULL );

private:
	int32 fAction;
	int32 fID;
	team_id fParentTeamID;
	BString fWindowName;
};

class TDockShowHideMenuItem : public TBitmapMenuItem
{
public:
	TDockShowHideMenuItem( const char *title, BBitmap *bitmap, const BList *teams, uint32 action );

protected:
	virtual status_t Invoke( BMessage *message = NULL );

private:
	const BList*		fTeams;
	uint32				fAction;
};

class TTrackerMenuItem : public TBitmapMenuItem
{
public:
	TTrackerMenuItem( const char *title, BBitmap *bitmap, entry_ref ref );
	TTrackerMenuItem( TMenu *submenu, BBitmap *bitmap, entry_ref ref );

	entry_ref fRef;

protected:
	virtual status_t Invoke( BMessage *message = NULL );
};

class TTrackerMenu : public TAwarePopupMenu
{
public:
	TTrackerMenu( const char *label, entry_ref ref );

	virtual void Show();
	void Build();

private:
	entry_ref fBaseRef;
	bool fCached;
};

class TDockMenus
{
public:
	static TAwarePopupMenu *WindowListMenu( const BList *teams, const char *signature, bool add_workspace_functions = true );
	static TAwarePopupMenu *TrackerMenu( entry_ref &ref );
	static int BuildTrackerMenu( TMenu *menu, entry_ref &ref );
};

#endif
