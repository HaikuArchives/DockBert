#ifndef INNER_PANEL_H
#define INNER_PANEL_H

#include <OS.h>
#include <String.h>
#include <List.h>
#include <Rect.h>
#include <GraphicsDefs.h>
#include <time.h> // time_t
#include <Entry.h> // entry_ref
#include <Archivable.h>
#include <Handler.h>
#include <PropertyInfo.h>

#ifdef DEBUG
#include <stdio.h>
#include <ClassInfo.h>
#include <Path.h>
#endif

#include <ctype.h>

#include "DockArchivableUtils.h"
#include "InnerPanelIcons.h"
#include "NMenu.h"


class BBitmap;
class BMessage;
class BMessageRunner;
class BFont;

class TPanelWindowView;

const int kInnelPanelTimerMessage = 'iptm';

const uint32 B_STARTED_DRAGGING	= 'bsdg';
const uint32 B_IS_DRAGGING		= 'bidg';
const uint32 B_ENDED_DRAGGING	= 'bedg';

class _EXPORT TInnerPanel : public BHandler
{
public:
	TInnerPanel( TPanelWindowView *);
	TInnerPanel( BMessage * );
	virtual ~TInnerPanel();

	virtual status_t Archive( BMessage *into, bool deep = true ) const;
	INSTANTIATE_OBJECT( TInnerPanel )

	virtual BHandler *ResolveSpecifier( BMessage *, int32, BMessage *, int32, const char * );
	virtual void MessageReceived( BMessage * );
	virtual status_t GetSupportedSuites( BMessage * );

	virtual bool WorthArchiving() { return true; }

	virtual void AttachedToDock( TPanelWindowView * );

	virtual void MouseDown( BPoint, uint32 ) {}
	virtual void MouseUp( BPoint, uint32 ) {}
	virtual void MouseMoved( BPoint, uint32 ) {}

	virtual void HandleDroppedMessage( BMessage *, BPoint );

	virtual bool HitsFrame(BPoint);

	BRect Frame() { return fFrame; }

	virtual void IncreaseFrame( float, bool = true );
	virtual void SetFrame( BRect, bool = true );
	void SetFramesFrame( BRect );
	void MoveFrameTo( BPoint );

	void Invalidate();

	virtual bool SetOptions( const char *, const BMessage * );
	virtual bool GetOptions( const char *, BMessage * );

	virtual void Draw(BRect);

	virtual void TimerFrame() {}

	virtual rgb_color BackColor() const;
	rgb_color FrameColor() const;

	virtual void IsDragging( BPoint, uint32 ) {}

	TPanelWindowView *Parent() const { return fParent; }

	virtual void DoDebugDump() {}
	virtual void DoCheckup() {}

protected:
friend class TPanelWindowView;
	virtual void DrawContent(BRect) {}
	virtual void DrawBackFrame();

	bool InitTimer();
	bool StopTimer();
	void SetTimerInterval( bigtime_t );
	
	TPanelWindowView *fParent;
	BRect fFrame;
	BRect fFrameFrame;

	BMessageRunner *fPanelTimer;

	// options
	rgb_color fBackFrameColor;
	BString fTabName;
	int32 fFrameHeight;
};

class _EXPORT TRaisingIconPanel : public TInnerPanel, public BList
{
public:
	TRaisingIconPanel( TPanelWindowView *);
	TRaisingIconPanel( BMessage * );
	virtual ~TRaisingIconPanel();

	virtual status_t Archive( BMessage *into, bool deep = true ) const;
	INSTANTIATE_OBJECT( TRaisingIconPanel )

	virtual BHandler *ResolveSpecifier( BMessage *, int32, BMessage *, int32, const char * );
	virtual status_t GetSupportedSuites( BMessage * );

	virtual void AttachedToDock( TPanelWindowView * );

	virtual void MouseDown( BPoint, uint32 );
	virtual void MouseUp( BPoint, uint32 );
	virtual void MouseMoved( BPoint, uint32 );

	virtual void IncreaseFrame( float, bool = true );
	virtual void SetFrame( BRect, bool = true );

	virtual bool AddItem( TPanelIcon *, int32 index = -1 );
	virtual bool RemoveItem( TPanelIcon *, bool = true );

	virtual void Invalidate( TPanelIcon *, bool = false );
	virtual void Invalidate( TPanelIcon *, BRect );

	virtual bool SetOptions( const char *, const BMessage * );
	virtual bool GetOptions( const char *, BMessage * );

	bool DrawSmallSign() const { return fDrawSmallSign; }
	bool DrawSignWhenNotRunning() const { return fDrawSignWhenNotRunning; }
	BBitmap *SmallSignHandleBitmap() const { return fSmallSignBitmap; }
	BBitmap *FlashedAppHandleBitmap() const { return fFlashedAppHandleBitmap; }
	bool DoAlphaBlendedActiveHandles() const { return fDoAlphaBlendedActiveHandles; }
	bool DrawCpuUsageBars() const { return fDrawCpuUsageBars; }

	inline void IconRect( int index, BRect &rect, bool frame = false )
	{
		rect = static_cast<TPanelIcon*>(ItemAt(index))->fFrame;
		if ( frame )
		{
			rect.left --;
			rect.right ++;
			rect.top --;
			rect.bottom ++;
		}
	}

	inline TPanelIcon *IconAt( BPoint point, bool frame = false )
	{
		int index, count = CountItems();
		for ( index=0; index<count; index ++ )
		{
			BRect rect;
			IconRect( index, rect, frame );
			if ( rect.Contains( point ) )
				return static_cast<TPanelIcon*>(ItemAt(index));
		}

		return 0;
	}

	virtual void DoDebugDump();
	virtual void DoCheckup();

protected:
	virtual void DrawContent( BRect );
	virtual void MouseDownIcon( TPanelIcon *, BPoint, uint32 );
	virtual void MouseUpIcon( TPanelIcon *, BPoint, uint32 );

	virtual void TimerFrame();

	void RebuildIconFrames(int32 index);

	BRect fIconFrame;
	bool fIsMouseOver;
	float fRealFrameSize;
	bool fIsCompletelyShown;
	BBitmap *fSmallSignBitmap;
	BBitmap *fFlashedAppHandleBitmap;

// <options>
	bool fAutoHide;
	bool fDrawSignWhenNotRunning;
	bool fDrawSmallSign;
	bool fDrawCpuUsageBars;
	bool fDoAlphaBlendedActiveHandles;
// </options>
};

const uint32 kApplicationPanelTimer = 'aptm';

class _EXPORT TApplicationPanel : public TRaisingIconPanel
{
public:
	TApplicationPanel(TPanelWindowView *parent);
	TApplicationPanel( BMessage *msg );
	virtual ~TApplicationPanel();

	virtual status_t Archive( BMessage *into, bool deep = true ) const;
	INSTANTIATE_OBJECT( TApplicationPanel )

	virtual BHandler *ResolveSpecifier( BMessage *, int32, BMessage *, int32, const char * );
	virtual status_t GetSupportedSuites( BMessage * );

	virtual bool SetOptions( const char *, const BMessage * );
	virtual bool GetOptions( const char *, BMessage * );

	virtual void AttachedToDock( TPanelWindowView * );

	virtual void MessageReceived( BMessage * );

	virtual bool AddItem( TPanelIcon *icon, int32 index = -1 )
	{ return TRaisingIconPanel::AddItem(icon, index); }

	virtual bool AddItem( const char *, int32 index = -1 );
	virtual bool AddItem( entry_ref&, int32 index = -1 );
	virtual bool RemoveItem( team_id );
	virtual bool RemoveItem( TPanelIcon *icon, bool deleteit= true )
		{ return TRaisingIconPanel::RemoveItem( icon, deleteit ); }

	TAppPanelIcon *ItemWith( const char * );
	TTrackerIcon *ItemWith( entry_ref& );
	TAppPanelIcon *ItemWith( team_id );

	virtual void IsDragging( BPoint, uint32 );
	virtual void HandleDroppedMessage( BMessage *, BPoint );

protected:
	BMessageRunner *fTimer;
};

class _EXPORT TShortcutPanel : public TApplicationPanel
{
public:
	TShortcutPanel(TPanelWindowView *parent);
	TShortcutPanel( BMessage *msg );

	INSTANTIATE_OBJECT( TShortcutPanel )
	virtual status_t Archive( BMessage *into, bool deep = true ) const;

	virtual BHandler *ResolveSpecifier( BMessage *, int32, BMessage *, int32, const char * );
	virtual void MessageReceived( BMessage * );
	virtual status_t GetSupportedSuites( BMessage * );

	virtual void AttachedToDock( TPanelWindowView * );

	virtual bool WorthArchiving() { return CountItems() > 0; }

	virtual bool AddItem( TPanelIcon *icon, int32 index = -1 );
	virtual bool AddItem( const char *mime, int32 index = -1 ) { return TApplicationPanel::AddItem(mime, index); }
	virtual bool AddItem( entry_ref&, int32 index = -1 );

	virtual bool RemoveItem( team_id id );

	virtual bool RemoveItem( TPanelIcon *, bool = true );

	virtual bool SetOptions( const char *, const BMessage * );
	virtual bool GetOptions( const char *, BMessage * );

	virtual void IsDragging( BPoint, uint32 );
	virtual void HandleDroppedMessage( BMessage *, BPoint );
protected:
	virtual void DrawContent( BRect );
	virtual void MouseDownIcon( TPanelIcon *, BPoint, uint32 );
};

class TReplicantShelfPanel;
#if 0

class TReplicantTrayPanel : public TReplicantTrayInternal
{
public:
	TReplicantTrayPanel( TReplicantShelfPanel *, bool );

	virtual void GetPreferredSize( float *, float * );
	virtual BPoint LocForReplicant(int32 replicantCount, int32 index, float width);

	virtual void AdjustPlacement();

	virtual void Draw(BRect);
private:
	TReplicantShelfPanel *fShelfPanel;
};

class TReplicantShelfPanel : public TInnerPanel
{
public:
	TReplicantShelfPanel( TPanelWindowView * );
	virtual ~TReplicantShelfPanel();

	virtual void SetFrame( BRect, bool = true );

	void AdjustPlacement();

private:
	TReplicantTrayPanel *fTray;
};

#endif // if 0

#endif
