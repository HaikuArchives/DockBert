#ifndef INNER_PANEL_ICONS_H
#define INNER_PANEL_ICONS_H

#include <stdio.h>

#include <Entry.h>
#include <Path.h>
#include <Handler.h>
#include <ClassInfo.h>
#include <Bitmap.h>
#include <Looper.h>
#include <List.h>

#include "NMenu.h"

#include "DockArchivableUtils.h"

#include "Bubble.h"

class BMessage;
class BList;
class BMessageRunner;

class TRaisingIconPanel;

const uint32 kTrackerEmptyTrash = 0x1000;
const uint32 kDeskbarPreferences = 0x1001;
const uint32 kLaunchTimePrefs = 0x1002;
const uint32 kClockToggleClockDate = 0x1003;
const uint32 kClockToggle24hDisplay = 0x1004;
const uint32 kClockToggleShowSeconds = 0x1005;

const uint32 CMD_SHUTDOWN_SYSTEM = 301;
const uint32 CMD_REBOOT_SYSTEM = 302;
const uint32 CMD_SUSPEND_SYSTEM = 304;

#define ROSTER_SIG "application/x-vnd.Be-ROST"

const uint32 kDefaultBigIconSize = 48;
const uint32 kDefaultSmallIconSize = 16;

// TPanelIcon:
//   Base class for all TRaisingIconPanel items

// fZoomStep goes from 0 [normal] to 16 [zoomed]
class TPanelIcon : public BHandler, public TBubbleTarget
{
public:
	TPanelIcon()
		: fParent(0), fZoomStep(0) {}
	// we dont want the BHandler to be reconstructed
	TPanelIcon( BMessage * /* archive */ )
		: BHandler(), fParent(0), fZoomStep(0) {}

	virtual ~TPanelIcon();

	virtual status_t InitCheck() { return B_OK; }

	// debug functions
	// DoDebugDump should dump this item's info using fParent->Parent()->DebugCall
	// DoCheckup should check if this item is OK, if not, do some dumping
	virtual void DoDebugDump();
	virtual void DoCheckup() {}

	// called when the item is added to the panel
	virtual void AttachedToPanel() {}

	// must return the item's size
	virtual void GetPreferredSize( float *width, float *height )
	{
		*width = 0;
		*height = 0;
	}

	// some item specific functions, zoomable icons' fZoomStep is modified
	virtual bool IsZoomable() const { return true; }
	// if Removable() returns false, the item wont be removed
	virtual bool Removable() const { return true; }

	// should return true if this items accepts drops
	virtual bool AcceptsDrop( bool /*force*/ = false ) const { return false; }
	// called when something is dropped in this item
	virtual status_t DroppedSomething( const BMessage * ) { return B_ERROR; }

	// used by DoMenu, should return a valid menu, or 0
	virtual TAwarePopupMenu *Menu() { return 0; }

	// default items behaviour is to show the menu when the right mouse button is pressed
	virtual void MouseDown( BPoint /* point */, uint32 /* modifiers */, uint32 buttons )
	{
		if ( buttons & B_SECONDARY_MOUSE_BUTTON )
		{
			DoMenu();
		}
	}

	virtual void MouseUp( BPoint /* point */, uint32 /* modifiers*/, uint32 /* buttons */ ) {}

	// this is called when drawing is needed
	virtual void Draw() {}

	// ContentLocation() and Frame() work like the BMenuItems ones
	BPoint ContentLocation()
	{
		return fFrame.LeftTop();
	}

	BRect Frame()
	{
		return BRect( 0, 0, fFrame.Width(), fFrame.Height() );
	}

	// bubble help specific stuff
	// BubbleText() should return the bubble text
	virtual const char *BubbleText() const { return 0; }
	// our own Touches implementation should do
	virtual bool Touches( BPoint p ) const;
	virtual BRect BubbleTargetRect() const { return fFrame; }
	// should this BubbleTarget be deleted by BubbleHelp? NO!
	virtual bool DeleteMe() const { return false; }

private:
	friend class TRaisingIconPanel;
	friend class TPanelWindowView;
	friend class TIconBubbleHelpTarget;

	BRect fFrame;

protected:
	void DoMenu();

	TRaisingIconPanel *fParent;
	int fZoomStep;
};

// this class handles the job of the zooming effect and drawing for
// general system icons
class _EXPORT TZoomableIcon : public TPanelIcon
{
public:
	TZoomableIcon();
	TZoomableIcon( BBitmap *, BBitmap * );
	TZoomableIcon( BMessage * );
	virtual ~TZoomableIcon();

	// should our bitmaps be freed by ~TZoomableIcon ?
	void SetDestroyBitmaps( bool );

	// returns 32x32
	virtual void GetPreferredSize( float *, float * );
	virtual void Draw();

protected:
	// the default implementation prepares drawing using B_OP_COPY
	virtual void PrepareDrawing();

	BBitmap *fSmallIcon, *fBigIcon;
	BBitmap *fDimmedSmallIcon, *fDimmedBigIcon;
	bool fDeleteBitmaps;
	bool fDisabled;

private:
	BBitmap *Bitmap();
	static BBitmap *DimmBitmap(BBitmap *);
};

class _EXPORT TTrackerIcon : public TZoomableIcon
{
public:
	TTrackerIcon( entry_ref );
	TTrackerIcon( BMessage * );
	virtual ~TTrackerIcon();

	virtual status_t InitCheck();
	virtual void AttachedToPanel();
	virtual void DoDebugDump();
	virtual void DoCheckup();

	INSTANTIATE_OBJECT( TTrackerIcon )

	virtual status_t Archive( BMessage *into, bool deep = true ) const
	{
		BEntry entry( &fRef );
		into->AddRef( "ref", &fRef );
		into->AddString( "path", fPath.Path() );
		return TPanelIcon::Archive( into, deep );
	}

	virtual void MessageReceived( BMessage * );
	virtual TAwarePopupMenu *Menu();
	virtual bool AcceptsDrop( bool force = false ) const;
	virtual status_t DroppedSomething( const BMessage * );
	virtual void MouseDown( BPoint point, uint32 modifiers, uint32 buttons );

	virtual void ReloadIcons();
	void SetRefTo( entry_ref );

	virtual void Launch();
	virtual void Quit();

	entry_ref Ref() const { return fRef; }

	virtual void EntryRemoved();

	virtual const char *BubbleText() const;

protected:
	entry_ref fRef;
	BPath fPath;
private:
	node_ref fNode;
};

const uint32 kActiveBitmapBlendingAnimation = 'aBBa';
const int32 kActiveBitmapBlendingAnimationStepCount	= 10;
const int32 kActiveBitmapBlendingAnimationLength	= 500000;

const uint32 kFlashAnimation = 'affa';

class _EXPORT TAppPanelIcon : public TTrackerIcon
{
public:
	TAppPanelIcon( entry_ref &ref, BList *team, const char *sig );
	TAppPanelIcon( BMessage * );
	virtual ~TAppPanelIcon();

	virtual void DoDebugDump();
	virtual void DoCheckup();

	INSTANTIATE_OBJECT( TAppPanelIcon )

	virtual status_t Archive( BMessage *into, bool deep = true ) const
	{
		into->AddString( "signature", BString(fSig) );
		return TTrackerIcon::Archive( into, deep );
	}

	virtual TAwarePopupMenu *Menu();
	virtual void MessageReceived( BMessage * );
	virtual bool AcceptsDrop( bool /*force*/ = false ) const { return true; }
	virtual status_t DroppedSomething( const BMessage * );
	virtual void Draw();

	virtual void Launch();
	virtual void Quit();

	virtual void EntryRemoved();

	virtual void SetActive( bool );

	float CpuUsage();

	const char *Signature() const { return fSig; }
	BList *Teams() { return fTeam; }

	virtual void Flash();

//	virtual status_t AddContextItem( BBitmap *bitmap, const char *label, BMessage *message );
//	virtual status_t RemoveContextItem( int index );
//	virtual void ClearContextItems();

protected:
	virtual void PrepareDrawing();

private:
	BList *fTeam;
	const char *fSig;

	bigtime_t fPrevKernTime, fPrevUserTime;
	bigtime_t fPreviousTiming;

	int fWindowCyclerIndex;
	int32 fPreviousActivatedWindow;

	bool fActive;
	float fActiveBitmapAlpha;
	int fAlphaAnimationFloatingMessages;

	bool fFlashed;
};

const uint32 kClockTimerMessage = 'ctmg';

class TClockIcon : public TPanelIcon
{
public:
	TClockIcon();
	TClockIcon( BMessage * );
	virtual ~TClockIcon();

	INSTANTIATE_OBJECT( TClockIcon )

	virtual status_t Archive( BMessage *into, bool deep = true ) const
	{
		into->AddBool( "ShowDate", fShowDate );
		into->AddBool( "Show24h", fShow24h );
		into->AddBool( "ShowSeconds", fShowSeconds );
		return TPanelIcon::Archive( into, deep );
	}

	virtual void AttachedToPanel();
	virtual void GetPreferredSize( float *, float * );

	virtual bool IsZoomable() const { return false; }
	virtual TAwarePopupMenu *Menu();
	virtual void MouseDown( BPoint point, uint32 modifiers, uint32 buttons );
	virtual void Draw();
	virtual void MessageReceived( BMessage * );

	virtual const char *BubbleText() const;
protected:
	void DrawTime();

	BFont fFont;
	BMessageRunner *fTimer;

	bool fShowDate;
	bool fShow24h;
	bool fShowSeconds;

	float fIWidth, fIHeight;
};

class TTrashIcon : public TTrackerIcon
{
public:
	TTrashIcon();
	TTrashIcon( BMessage * );
	virtual ~TTrashIcon();

	INSTANTIATE_OBJECT( TTrashIcon )

	virtual bool Removable() const;

	virtual TAwarePopupMenu *Menu();
	virtual status_t DroppedSomething( const BMessage * );
	virtual void MessageReceived( BMessage * );

	virtual const char *BubbleText() const { return "Trash"; }
};

class TWorkspacesMenuItem : public TMenuItem
{
public:
	TWorkspacesMenuItem( const char *label, int i )
		: TMenuItem( label, 0 ), fWorkspace(i)
	{
	}

	virtual status_t Invoke( BMessage *message )
	{
		activate_workspace(fWorkspace);
		return TMenuItem::Invoke( message );
	}

protected:
	int fWorkspace;
};

class TWorkspacesIcon : public TPanelIcon
{
public:
	TWorkspacesIcon();
	TWorkspacesIcon( BMessage * );
	virtual ~TWorkspacesIcon();

	INSTANTIATE_OBJECT( TWorkspacesIcon )

	virtual void GetPreferredSize( float *, float * );
	virtual void AttachedToPanel();
	virtual bool IsZoomable() const { return false; }
	virtual TAwarePopupMenu *Menu();
	virtual void MouseDown( BPoint point, uint32 modifiers, uint32 buttons );
	virtual void Draw();
	virtual void MessageReceived( BMessage * );

	virtual const char *BubbleText() const;

protected:
	virtual void DrawIcon();

	BFont fFont;
	BBitmap *fWorkspaceImage;
};

class TDockbertIcon : public TTrackerIcon
{
public:
	TDockbertIcon( entry_ref &ref );
	TDockbertIcon( BMessage * );

	INSTANTIATE_OBJECT( TDockbertIcon )

	virtual bool Removable() const;

	virtual TAwarePopupMenu *Menu();
	virtual void MessageReceived( BMessage * );
	virtual void MouseDown( BPoint point, uint32 modifiers, uint32 buttons );

	virtual const char *BubbleText() const { return "Be Menu"; }
};

class TSeparatorIcon : public TPanelIcon
{
public:
	TSeparatorIcon();
	TSeparatorIcon( BMessage * );

	INSTANTIATE_OBJECT( TSeparatorIcon )

	virtual bool IsZoomable() const { return false; }
	virtual void GetPreferredSize( float *, float * );
	virtual void Draw();
private:
	BBitmap *fBitmap;
};

class TShowDesktopIcon : public TZoomableIcon
{
public:
	TShowDesktopIcon();
	TShowDesktopIcon( BMessage * );

	INSTANTIATE_OBJECT( TShowDesktopIcon )

	virtual void MouseDown( BPoint point, uint32 modifiers, uint32 buttons );

	const char *BubbleText() const;

private:
	BList fWindowList;
};

#endif
