#ifndef PANEL_WINDOW_VIEW_H
#define PANEL_WINDOW_VIEW_H

#include <View.h>
#include <Window.h>
#include <List.h>
#include <Locker.h>

#include "Bubble.h"

#include "LockingList2.h"

class BMessage;
class BBitmap;
class BMessageRunner;

class TInnerPanel;
class TRaisingIconPanel;
class TPanelIcon;
class TApplicationPanel;
class TAppPanelIcon;
class TShortcutPanel;
class TPossibleApplicationIcon;

class TAwarePopupMenu;

const int kLocationTop = 'ltop';
const int kLocationBottom = 'lbot';
const int kLocationLeft = 'llft';
const int kLocationRight = 'lrht';

const uint32 kDoBubbleHelp = 'bhlp';

class BPicture;

class TPanelWindowView : public BView {
public:
	TPanelWindowView();
	TPanelWindowView( BMessage * );
	~TPanelWindowView();

	void AttachedToWindow();
	void MessageReceived( BMessage * );
	void MouseDown( BPoint );
	void MouseUp( BPoint );
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	void Draw( BRect );
	virtual BHandler *ResolveSpecifier( BMessage *, int32, BMessage *, int32, const char * );
	virtual status_t GetSupportedSuites( BMessage * );

	void GetDockbertSuites( BMessage * );

	virtual status_t Archive( BMessage *into, bool deep = true ) const;
	static BArchivable *Instantiate(BMessage *from);

	void ChangedSize( TInnerPanel * );
	void SetHighlightedIcon( TRaisingIconPanel *parent, TPanelIcon * );
	void AddToIconAnimationList( TPanelIcon * );
	void RemoveIconFromAnimationList( TPanelIcon * );
	void AddPanel( TInnerPanel *, TInnerPanel * = 0 );
	void RemovePanel( TInnerPanel * );
	int32 IndexOf( TInnerPanel * );
	bool MoveTabTo( TInnerPanel *, int32 );
	int32 CountPanels();

	void AddRunning( TApplicationPanel *, TAppPanelIcon * );
	void AddShortcut( TShortcutPanel *, entry_ref, int32 index = -1 );
	void AddShortcut( TShortcutPanel *, const char * );
	void RemoveShortcut( TPanelIcon * );

	void AddTeam( BList *team, BBitmap *icon, char *name, char *sig );
	void AddTeam( team_id, const char *sig, entry_ref );
	void RemoveTeam( team_id );

	int WhereAmI() const { return fLocation; }

	bool SetOptions( const char *, const BMessage * );
	bool GetOptions( const char *, BMessage * );

	void HandleDroppedMessage( BMessage *, BPoint );

	inline rgb_color BackColor() const { return fColor2; }
	bool UseTransparentMenus() const { return fUseTransparentMenus; }
	bool DrawOuterFrame() const { return fDrawOuterFrame; }
	bool UseWindowShapping() const { return fUseWindowShapping; }
	rgb_color OuterFrameColor() const { return fOuterFrameColor; }

	void RegisterWorkspaceChange( BMessenger * );

	void WorkspaceChanged();

	void GoMenu( TAwarePopupMenu *, BPoint, BRect * );

	void DraggingItem( TPanelIcon *item );

	TAppPanelIcon *ItemWith( team_id );
	TAppPanelIcon *ItemWith( const char *sig );

	void DebugCall(int32, const char *, ...);

	TBubbleHelp *BubbleHelp() const { return fBubbleHelp; }
	bool IsAMenuOpen() const { return fOpenMenu!=0; }
	const char *LastActivatedAppSig() const { return fLastActivatedAppSig.String(); }

	void DoDebugDump();
	void DoCheckup();

private:
	int DoIconSmallerWithTime();
	TInnerPanel *PanelAt( BPoint );
	void BuildViewsPicture(bool);

	void WindowResizeBy( float i )
	{
		if ( fLocation == kLocationTop || fLocation == kLocationBottom )
			Window()->ResizeBy( i, 0 );
		else
			Window()->ResizeBy( 0, i );
	}

	static int32 _rearrange_tabs_thread_entry( void * );
	void ReArrangeTabsThread();

	LockingList2<TInnerPanel> fPanels;
	TPanelIcon *fHighlightedIcon;
	BMessageRunner *fTimer;
	bool fIsDraggingFromOutside;
	TApplicationPanel *fRunningAppPanel;
	TInnerPanel *fPreviousMouseMovedPanel;
	LockingList2<TPanelIcon> fHighlightedIconList;
	BObjectList<TShortcutPanel> fShortcutPanelList;
	BMessage *fThisArchive;
	BList fWorkspaceChangeNotifyList;

	BPicture *fMyPicture;

//	BLocker fHighlightIconsLock;
//	BLocker fPanelListLock;

	TAwarePopupMenu *fOpenMenu;

	TAppPanelIcon *fLastActiveAppIcon;
	BString fLastActivatedAppSig;

	TBubbleHelp *fBubbleHelp;

	BPoint fPreviousMousePosition;
	int fBubbleCounter;

	// options
	bool fUseTransparentMenus;
	int32 fLocation;
	rgb_color fColor1, fColor2, fColor3;
	bool fDrawOuterFrame;
	bool fAlwaysOnTop;
	bool fHideStandardDeskbar;
	bool fUseWindowShapping;
	rgb_color fOuterFrameColor;
	int32 fDebugLevel;
};

#endif
