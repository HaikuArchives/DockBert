#include "InnerPanel.h"

#include <Shape.h>
#include <Point.h>
#include <Bitmap.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <MessageRunner.h>
#include <Font.h>
#include <PropertyInfo.h>
#include <View.h>
#include <AppFileInfo.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Volume.h>
#include <ClassInfo.h>
#include <Alert.h>
#include <NodeMonitor.h>
#include <Polygon.h>

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "PanelWindow.h"
#include "PanelWindowView.h"

#include "DockArchivableUtils.h"

#include "tracker_private.h"

#include "ResourceSet.h"
#include "othericons.h"

#include "FSUtils.h"

// TReplicantShelf
#include "StatusViewShelf.h"

using namespace BPrivate;

const property_info _inner_panel_property_list[] = {
	{ "FrameColor",	{ B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_RGB_COLOR_TYPE }, {}, {} },
	{ "Name",		{ B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_STRING_TYPE }, {}, {} },
//	{ "FrameHeight",{ B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_INT32_TYPE }, {}, {}  },

	{ "Index",{ B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_INT32_TYPE }, {}, {}  },

	{ "AboutInfo",	{ B_GET_PROPERTY }, { B_DIRECT_SPECIFIER, 0 }, 0, 0, { B_STRING_TYPE }, {}, {} },
	{ "VersionInfo",{ B_GET_PROPERTY }, { B_DIRECT_SPECIFIER, 0 }, 0, 0, { B_STRING_TYPE }, {}, {} },
	{NULL, {}, {}, NULL, 0, {}, {}, {} }
};

const property_info _raising_icon_panel_property_list[] = {
	{ "AutoHide", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_BOOL_TYPE }, {}, {} },
	{ "DrawSignWhenNotRunning", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_BOOL_TYPE }, {}, {} },
	{ "DrawSmallSign", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_BOOL_TYPE }, {}, {} },
	{NULL, {}, {}, NULL, 0, {}, {}, {} }
};

const property_info _application_panel_property_list[] = {
	{ "DrawCpuUsageBars", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_BOOL_TYPE }, {}, {} },
	{ "DoAlphaBlendedActiveHandles", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_BOOL_TYPE }, {}, {} },

	{NULL, {}, {}, NULL, 0, {}, {}, {} }
};

TInnerPanel::TInnerPanel(TPanelWindowView *parent)
	: fParent(0), fPanelTimer(0)
{
	fBackFrameColor = (rgb_color){218,218,205,255};
	fTabName = "unnamed";
	fFrameHeight = 11 + kDefaultBigIconSize;

	AttachedToDock( parent );
}

TInnerPanel::TInnerPanel( BMessage *msg )
	: BHandler(msg), fParent(0), fPanelTimer(0)
{
	int32 settingsversion;

	if ( msg->FindInt32( "SettingVersion", &settingsversion ) != B_OK )
		settingsversion = 0;

	rgb_color *temp_c;
	ssize_t _size;
	if ( msg->FindData( "FrameColor", B_RGB_COLOR_TYPE, (const void**)&temp_c, &_size ) == B_OK)
		fBackFrameColor = *temp_c;
	else
		fBackFrameColor = (rgb_color){218,218,205,255};
	
	if ( msg->FindString( "Name", &fTabName ) != B_OK )
		fTabName = "unnamed";

//	fFrameHeight = 43;
	fFrameHeight = 11 + kDefaultBigIconSize;

	TPanelWindowView *parent;
	msg->FindPointer( "parent", (void**)&parent );

	AttachedToDock( parent );
}

TInnerPanel::~TInnerPanel()
{
	StopTimer();
}

status_t TInnerPanel::Archive( BMessage *into, bool deep ) const
{
	into->AddData( "FrameColor", B_RGB_COLOR_TYPE, &fBackFrameColor, sizeof(fBackFrameColor) );

	into->AddString( "Name", fTabName );
	into->AddInt32( "FrameHeight", fFrameHeight );

	return BHandler::Archive( into, deep );
}

BHandler *TInnerPanel::ResolveSpecifier( BMessage *message, int32 index, BMessage *specifier, int32 what, const char *property )
{
	BPropertyInfo prop_info( const_cast<property_info *>(_inner_panel_property_list) );
	if ( prop_info.FindMatch( message, index, specifier, what, property ) >= 0 )
		return this;
	return BHandler::ResolveSpecifier( message, index, specifier, what, property );
}

void TInnerPanel::MessageReceived( BMessage *message )
{
	if ( message->what == B_GET_PROPERTY || message->what == B_SET_PROPERTY )
	{
		int32 index, what;
		BMessage specifier;
		const char *property;
		if ( message->GetCurrentSpecifier( &index, &specifier, &what, &property ) == B_OK )
		{
			BMessage reply( B_REPLY );
			if ( message->what == B_GET_PROPERTY )
			{
				if ( GetOptions( property, &reply ) )
					reply.AddInt32( "error", 0 );
				else
					reply.AddInt32( "error", -1 );
			}
			else
			{
				if ( SetOptions( property, message ) )
					reply.AddInt32( "error", 0 );
				else
					reply.AddInt32( "error", -1 );
			}
			message->SendReply( &reply );
		}
	}
	else
		BHandler::MessageReceived(message);
}

status_t TInnerPanel::GetSupportedSuites( BMessage *message )
{
	message->AddString( "suites", "suite/vnd.DM-innerpanel" );
	BPropertyInfo prop_info( const_cast<property_info *>(_inner_panel_property_list) );
	message->AddFlat( "messages", &prop_info );
	return BHandler::GetSupportedSuites(message);
}

void TInnerPanel::AttachedToDock( TPanelWindowView *view )
{
	fParent = view;

	fParent->Window()->AddHandler( this );

	SetFrame( BRect( 0, 0, 0, fFrameHeight ) );
}

void TInnerPanel::HandleDroppedMessage( BMessage *message, BPoint point )
{
	if ( message->what == B_PASTE )
	{
		rgb_color *r;
		ssize_t s;

		if ( message->FindData( "RGBColor", B_RGB_COLOR_TYPE, (const void**)&r, &s ) == B_OK )
		{
			BMessage msg;
			msg.AddData( "data", B_RGB_COLOR_TYPE, r, sizeof(rgb_color) );
			SetOptions( "FrameColor", &msg );
		}
	}
	else
		fParent->HandleDroppedMessage( message, point );
}

bool TInnerPanel::HitsFrame( BPoint point )
{
	BRect rect = fFrameFrame;
	rect.left += 5;
	rect.right -= 5;
	return rect.Contains(point);
}

void TInnerPanel::IncreaseFrame( float amount, bool update )
{
	fFrame.right += amount;
	fFrameFrame.right += amount;

	if ( update && fParent)
		fParent->ChangedSize( this );
}

void TInnerPanel::SetFrame( BRect rect, bool update )
{
	fFrame = rect;
	fFrameFrame = BRect( rect.left + 5, rect.top + 6, rect.right - 5, rect.bottom-3 );

	if ( update && fParent)
		fParent->ChangedSize( this );
}

void TInnerPanel::SetFramesFrame( BRect rect )
{
	fFrameFrame = rect;
}

void TInnerPanel::MoveFrameTo( BPoint point )
{
	float w = fFrame.Width();
	float h = fFrame.Height();

	BRect r;
	r.left = point.x;
	r.top = point.y;
	r.right = point.x + w;
	r.bottom = point.y + h;

	SetFrame( r );
}

void TInnerPanel::Invalidate()
{
	fParent->Invalidate( fFrame );
}

bool TInnerPanel::SetOptions( const char *what, const BMessage *msg )
{
	if ( !strcasecmp( what, "FrameColor" ) )
	{
		rgb_color *temp_c;
		ssize_t _size;
		if ( msg->FindData( "data", B_RGB_COLOR_TYPE, (const void**)&temp_c, &_size ) == B_OK)
		{
			fBackFrameColor = *temp_c;
			Invalidate();
		}
		else
			return false;
	}
	else if ( !strcasecmp( what, "Name" ) )
	{
		BString n;
		if ( msg->FindString( "data", &n ) == B_OK )
			fTabName = n;
		else
			return false;
	}
/*	else if ( !strcasecmp( what, "FrameHeight" ) )
	{
		if ( msg->FindInt32( "data", &fFrameHeight ) == B_OK )
		{
			BRect rect = Frame();
			rect.bottom = fFrameHeight;
			SetFrame( rect );
		}
	}*/
	else if ( !strcasecmp( what, "Index" ) )
	{
		int32 index;
		if ( msg->FindInt32( "data", &index ) != B_OK || !fParent->MoveTabTo( this, index ) )
			return false;
	}
	else
		return false;

	return true;
}

bool TInnerPanel::GetOptions( const char *what, BMessage *msg )
{
	if ( !strcasecmp( what, "FrameColor" ) )
		msg->AddData( "response", B_RGB_COLOR_TYPE, &fBackFrameColor, sizeof(fBackFrameColor) );
	else if ( !strcasecmp( what, "Name" ) )
		msg->AddString( "response", fTabName );
//	else if ( !strcasecmp( what, "FrameHeight" ) )
//		msg->AddInt32( "response", fFrameHeight );
	else if ( !strcasecmp( what, "Index" ) )
		msg->AddInt32( "response", fParent->IndexOf(this) );
	else if ( !strcasecmp( what, "AboutInfo" ) )
		msg->AddString( "about", BString("standard set of tabs - by Hugo Santos (linn) and Daniel T. Bender (HarvestMoon)") );
	else if ( !strcasecmp( what, "VersionInfo" ) )
	{
		msg->AddString( "status", BString( BUILDSTATUS ) );
		msg->AddString( "build_nr", BString( BUILDNR ) );
		msg->AddString( "version", BString( BUILDVERSION ) );
		msg->AddString( "build_date", BString( BUILDDATE ) );
	}
	else
		return false;
	return true;
}

void TInnerPanel::Draw(BRect rect)
{
	if ( fFrame.Width() >= 30 ) // todo: review this 30
		DrawBackFrame();
	DrawContent(rect);
}

rgb_color TInnerPanel::BackColor() const
{
	return fParent->BackColor();
}

rgb_color TInnerPanel::FrameColor() const
{
	return fBackFrameColor;
}

void TInnerPanel::DrawBackFrame()
{
	BRect rect = fFrameFrame;

	fParent->SetHighColor( fBackFrameColor );

	fParent->FillRect( BRect( rect.left + 5, rect.top + 8, rect.right - 5, rect.bottom ) );

	fParent->FillRect( BRect( rect.left + 10, rect.top, rect.right - 10, rect.top + 8 ) );

	BRect left_top_arc = BRect( rect.left + 5, rect.top, rect.left + 20, rect.top + 15 );
	fParent->FillArc( left_top_arc, 90, 90 );
	BRect right_top_arc = BRect( rect.right - 20, rect.top, rect.right - 5, rect.top + 15 );
	fParent->FillArc( right_top_arc, 0, 90 );

	BPoint p[3];
	p[0].x = rect.left+2; p[0].y = rect.bottom;
	p[1].x = rect.left+5; p[1].y = rect.bottom-3;
	p[2].x = rect.left+5; p[2].y = rect.bottom;

	BPolygon pol( p, 3 );

	fParent->FillPolygon( &pol );

	p[0].x = rect.right-2; p[0].y = rect.bottom;
	p[1].x = rect.right-5; p[1].y = rect.bottom-3;
	p[2].x = rect.right-5; p[2].y = rect.bottom;

	pol = BPolygon( p, 3 );

	fParent->FillPolygon( &pol );

	if ( fParent->UseWindowShapping() && fParent->DrawOuterFrame() )
	{
//	dark frame
		fParent->SetHighColor( fParent->OuterFrameColor() );

		fParent->StrokeArc( left_top_arc, 90, 90 );
		fParent->StrokeArc( right_top_arc, 0, 90 );

		fParent->StrokeLine( BPoint( rect.left + 10, rect.top ), BPoint( rect.right - 10, rect.top) );
		fParent->StrokeLine( BPoint( rect.left + 5, rect.top + 5 ), BPoint( rect.left +5, rect.bottom - 5 +1) );
		fParent->StrokeLine( BPoint( rect.right -5, rect.top + 5 ), BPoint( rect.right -5, rect.bottom - 5 +1) );

		fParent->StrokeLine( BPoint( rect.left + 2, rect.bottom), BPoint( rect.left + 5, rect.bottom - 3 ) );
		fParent->StrokeLine( BPoint( rect.right - 2, rect.bottom), BPoint( rect.right - 5, rect.bottom - 3 ) );

	// hm-ex
		fParent->StrokeLine( BPoint( fFrame.left, fFrame.bottom - 3), BPoint( fFrame.left + 6, fFrame.bottom - 3 ) );
		fParent->StrokeLine( BPoint( fFrame.right - 6, fFrame.bottom - 3), BPoint( fFrame.right, fFrame.bottom - 3 ) );
	}
}

bool TInnerPanel::InitTimer()
{
	if ( !fPanelTimer )
	{
		BMessage *_message = new BMessage( kInnelPanelTimerMessage );
		_message->AddPointer( "panel_target", this );
		fPanelTimer = new BMessageRunner( BMessenger(fParent->Window()), _message, 99999999 );
		if ( fPanelTimer->InitCheck() != B_OK )
		{
			delete fPanelTimer;
			fPanelTimer = 0;
			return false;
		}
		return true;
	}
	else
		return false;
}

bool TInnerPanel::StopTimer()
{
	if ( fPanelTimer )
	{
		delete fPanelTimer;
		fPanelTimer = 0;
		return true;
	}
	else
		return false;
}

void TInnerPanel::SetTimerInterval( bigtime_t interval )
{
	if ( fPanelTimer )
		fPanelTimer->SetInterval( interval );
}

TRaisingIconPanel::TRaisingIconPanel(TPanelWindowView *parent)
	: TInnerPanel( parent )
{
	fAutoHide = false;

	fIsMouseOver = false;
	fRealFrameSize = 0;
	fIsCompletelyShown = true;

	fDrawSignWhenNotRunning = true;
	fDrawSmallSign = true;

	fDrawCpuUsageBars = false;
	fDoAlphaBlendedActiveHandles = false;

	fSmallSignBitmap = const_cast<BBitmap*>( AppResSet()->FindBitmap( 'BBMP', R_SmallSign ) );
	fFlashedAppHandleBitmap = const_cast<BBitmap*>( AppResSet()->FindBitmap( 'BBMP', R_FlashSign ) );
}

TRaisingIconPanel::TRaisingIconPanel( BMessage *msg )
	: TInnerPanel( msg )
{
	fIsMouseOver = false;
	fRealFrameSize = 0;
	fIsCompletelyShown = true;

	if ( msg->FindBool( "AutoHide", &fAutoHide ) != B_OK )
		fAutoHide = false;
	if ( msg->FindBool( "DrawSignWhenNotRunning", &fDrawSignWhenNotRunning ) != B_OK )
		fDrawSignWhenNotRunning = true;
	if ( msg->FindBool( "DrawSmallSign", &fDrawSmallSign ) != B_OK )
		fDrawSmallSign = true;
	if ( msg->FindBool( "DrawCpuUsageBars", &fDrawCpuUsageBars ) != B_OK )
		fDrawCpuUsageBars = false;
	if ( msg->FindBool( "DoAlphaBlendedActiveHandles", &fDoAlphaBlendedActiveHandles ) != B_OK )
		fDoAlphaBlendedActiveHandles = false;

	fSmallSignBitmap = const_cast<BBitmap*>( AppResSet()->FindBitmap( 'BBMP', R_SmallSign ) );
	fFlashedAppHandleBitmap = const_cast<BBitmap*>( AppResSet()->FindBitmap( 'BBMP', R_FlashSign ) );
}

TRaisingIconPanel::~TRaisingIconPanel()
{
	int32 count = CountItems();
	for ( int i=0; i<count; i++ )
	{
		TPanelIcon *item = static_cast<TPanelIcon*>(ItemAt(i));
		delete item;
	}
}

status_t TRaisingIconPanel::Archive( BMessage *into, bool deep ) const
{
	into->AddBool( "AutoHide", fAutoHide );
	into->AddBool( "DrawSignWhenNotRunning", fDrawSignWhenNotRunning );
	into->AddBool( "DrawSmallSign", fDrawSmallSign );

	return TInnerPanel::Archive( into, deep );
}

BHandler *TRaisingIconPanel::ResolveSpecifier( BMessage *message, int32 index, BMessage *specifier, int32 what, const char *property )
{
	BPropertyInfo prop_info( const_cast<property_info *>(_raising_icon_panel_property_list) );
	if ( prop_info.FindMatch( message, index, specifier, what, property ) >= 0 )
		return this;
	return TInnerPanel::ResolveSpecifier( message, index, specifier, what, property );
}

status_t TRaisingIconPanel::GetSupportedSuites( BMessage *message )
{
	message->AddString( "suites", "suite/vnd.DM-raisingiconpanel" );
	BPropertyInfo prop_info( const_cast<property_info *>(_raising_icon_panel_property_list) );
	message->AddFlat( "messages", &prop_info );
	return TInnerPanel::GetSupportedSuites(message);
}

void TRaisingIconPanel::AttachedToDock( TPanelWindowView *view )
{
	TInnerPanel::AttachedToDock(view);
}

void TRaisingIconPanel::MouseDown( BPoint point, uint32 buttons )
{
	bool do_mouse_up = false;
	BPoint pw;
	if ( buttons & B_PRIMARY_MOUSE_BUTTON )
	{
		int count = 10;
		uint32 mods;
		bool did_move = false;
		while ( count )
		{
			count --;
			fParent->GetMouse( &pw, &mods );
			if ( !mods )
			{
				do_mouse_up = true;
				break;
			}
			if ( pw != point )
			{
				if ( did_move )
				{
					TPanelIcon *hicon = IconAt( point );
					fParent->DraggingItem( hicon );
					return;
				}
				else
					did_move = true;
			}
			snooze( 500000 / 10 );
		}
	}

	if ( fIconFrame.Contains( point ) )
	{
		TPanelIcon *hicon = IconAt( point );

		if ( hicon )
			MouseDownIcon( hicon, point, buttons );
	}

	if ( do_mouse_up )
	{
		MouseUp( pw, 0 );
	}
}

void TRaisingIconPanel::MouseUp( BPoint point, uint32 buttons )
{
	if ( fIconFrame.Contains( point ) )
	{
		TPanelIcon *hicon = IconAt(point);
		if ( hicon )
			MouseUpIcon( hicon, point, buttons );
	}
}

void TRaisingIconPanel::MouseMoved( BPoint point, uint32 transit)
{
	TPanelIcon *hicon = 0;

	if ( transit != B_EXITED_VIEW )
	{
		fIsMouseOver = true;
		if ( fIconFrame.Contains( point ) )
		{
			hicon = IconAt( point, true );
		}
	}
	else
		fIsMouseOver = false;

	if ( hicon && !hicon->IsZoomable() )
		hicon = 0;

	fParent->SetHighlightedIcon( this, hicon );
}

void TRaisingIconPanel::IncreaseFrame( float amount, bool update )
{
	fRealFrameSize += amount;
	fIconFrame.right += amount;
	TInnerPanel::IncreaseFrame( amount, update );
}

void TRaisingIconPanel::SetFrame( BRect rect, bool update )
{
	fIconFrame = rect;
	fIconFrame.left += 15;
	fIconFrame.right -= 15;
	fIconFrame.top = (rect.Height() - kDefaultBigIconSize) / 2 +1; // +1 +1;
	fIconFrame.bottom = fIconFrame.top + kDefaultBigIconSize;
	TInnerPanel::SetFrame( rect, update );
	fRealFrameSize = fFrame.Width();

	// todo: optimize this
	RebuildIconFrames(-1);
}

bool TRaisingIconPanel::AddItem( TPanelIcon *icon, int32 index )
{
	if ( !icon )
		return false;

	bool b;

	if ( index < 0 )
		b = BList::AddItem(icon);
	else
		b = BList::AddItem(icon, index);

	if ( !b )
		return false;

	if ( !icon->Looper() )
		fParent->Window()->AddHandler( icon );

	icon->fParent = this;
	icon->AttachedToPanel();

	Parent()->BubbleHelp()->AddBubble( icon );

	if ( CountItems() == 1 )
	{
		IncreaseFrame( 30 );
	}

	float iw, ih;
	icon->GetPreferredSize( &iw, &ih );

	IncreaseFrame( iw, fIsCompletelyShown );

	RebuildIconFrames(index<0?CountItems()-1:index);

	return true;
}

bool TRaisingIconPanel::RemoveItem( TPanelIcon *icon, bool deleteit )
{
	if ( !BList::RemoveItem(icon) )
		return false;

	Parent()->BubbleHelp()->DeleteBubble( icon );

	fParent->RemoveIconFromAnimationList( icon );

	int32 index = IndexOf(icon);

//	IncreaseFrame( - (icon->fIconSize.x > 16 ? 16 : icon->fIconSize.x) *2, fIsCompletelyShown );
	IncreaseFrame( -icon->fFrame.Width(), fIsCompletelyShown );

	if ( CountItems() == 0 )
	{
		IncreaseFrame( -30 );
	}

	if ( deleteit )
		delete icon;
	else
		icon->fParent = 0;

	RebuildIconFrames( index );

	return true;
}

void TRaisingIconPanel::Invalidate( TPanelIcon *icon, bool full )
{
	int icon_index = IndexOf( icon );

	if ( icon_index >= 0 )
	{
		BRect r;
		IconRect( icon_index, r, full );
		r.left --;
		r.top --;
		r.right ++;
		r.bottom ++;
		fParent->Window()->Lock();
		fParent->Invalidate(r);
		fParent->Window()->Unlock();
	}
}

void TRaisingIconPanel::Invalidate( TPanelIcon *icon, BRect r  )
{
	int icon_index = IndexOf( icon );

	if ( icon_index >= 0 )
	{
		BRect frame;
		IconRect( icon_index, frame );
		r.left += frame.left;
		r.top += frame.top;
		r.right += frame.left;
		r.bottom += frame.top;
		fParent->Window()->Lock();
		fParent->Invalidate(r);
		fParent->Window()->Unlock();
	}
}

bool TRaisingIconPanel::SetOptions( const char *what, const BMessage *msg )
{
	if ( !strcasecmp( what, "AutoHide" ) )
	{
		bool val;
		if ( msg->FindBool( "data", &val ) == B_OK )
		{
			fAutoHide = val;
			if ( fAutoHide )
			{
				InitTimer();
				SetTimerInterval( 1000 );
			}
			else
			{
				StopTimer();
				fIsCompletelyShown = true;
				IncreaseFrame( fRealFrameSize - fFrame.Width() );
			}
		}
		else
			return false;
	}
	else if ( !strcasecmp( what, "DrawSignWhenNotRunning" ) )
	{
		if ( msg->FindBool( "data", &fDrawSignWhenNotRunning ) != B_OK )
			return false;
		TInnerPanel::Invalidate();
	}
	else if ( !strcasecmp( what, "DrawSmallSign" ) )
	{
		if ( msg->FindBool( "data", &fDrawSmallSign ) != B_OK )
			return false;
		TInnerPanel::Invalidate();
	}
	else
		return TInnerPanel::SetOptions( what, msg );

	return true;
}

bool TRaisingIconPanel::GetOptions( const char *what, BMessage *msg )
{
	if ( !strcasecmp( what, "AutoHide" ) )
		msg->AddBool( "data", fAutoHide );
	else if ( !strcasecmp( what, "DrawSignWhenNotRunning" ) )
		msg->AddBool( "data", fDrawSignWhenNotRunning );
	else if ( !strcasecmp( what, "DrawSmallSign" ) )
		msg->AddBool( "data", fDrawSmallSign );
	else
		return TInnerPanel::GetOptions( what, msg );
	return true;
}

void TRaisingIconPanel::DoDebugDump()
{
	int count = CountItems();

	fParent->DebugCall( 1, "\tI have %i items and am a %s", count, class_name(this) );

	TPanelIcon *icon;

	for ( int i=0; i<count; i++ )
	{
		icon = static_cast<TPanelIcon*>(ItemAt(i));
		icon->DoDebugDump();
	}
}

void TRaisingIconPanel::DoCheckup()
{
	int count = CountItems();

	TPanelIcon *icon;

	for ( int i=0; i<count; i++ )
	{
		fParent->DebugCall( 1, "\tdoing checkup for icon %i", i+1 );
		icon = static_cast<TPanelIcon*>(ItemAt(i));
		icon->DoCheckup();
	}
}

void TRaisingIconPanel::DrawContent( BRect updateRect )
{
	int count = CountItems();

	TPanelIcon *icon;

	for ( int i=0; i<count; i++ )
	{
		icon = static_cast<TPanelIcon*>(ItemAt(i));

		if ( updateRect.Intersects( icon->fFrame ) )
			icon->Draw();
	}
}

void TRaisingIconPanel::MouseDownIcon( TPanelIcon *icon, BPoint where, uint32 buttons )
{
	icon->MouseDown( where, modifiers(), buttons );
}

void TRaisingIconPanel::MouseUpIcon( TPanelIcon *icon, BPoint where, uint32 buttons )
{
	icon->MouseUp( where, modifiers(), buttons );
}

void TRaisingIconPanel::TimerFrame()
{
// todo: this is completly broken.. fix
	if ( !fIsMouseOver )
	{
		fIsCompletelyShown = false;
		if ( fFrame.Width() > 60 )
		{
			IncreaseFrame( -1 );
			fRealFrameSize ++;
		}
	}
	else
	{
		if ( fFrame.Width() < fRealFrameSize )
		{
			IncreaseFrame( 1 );
			fRealFrameSize --;
		}
		else
			fIsCompletelyShown = true;
	}
}

void TRaisingIconPanel::RebuildIconFrames(int32 index)
{
	int32 count = CountItems();
	if ( index < 0 || index >= count )
		index = 0;
	float j = (index == 0) ? (fIconFrame.left) : (static_cast<TPanelIcon*>(ItemAt(index-1))->fFrame.right);

	for ( ; index<count; index++ )
	{
		TPanelIcon *icon = static_cast<TPanelIcon*>(ItemAt(index));
		float width, height;
		icon->GetPreferredSize(&width, &height);
		icon->fFrame.left = j;
		icon->fFrame.right = j+width;
		icon->fFrame.top = fIconFrame.top;
		icon->fFrame.bottom = fIconFrame.top+height;
		Invalidate(icon);
		j += width;
	}
}

TApplicationPanel::TApplicationPanel( TPanelWindowView *parent )
	: TRaisingIconPanel(parent), fTimer(0)
{
	if ( fDrawCpuUsageBars )
	{
		fTimer = new BMessageRunner( BMessenger(this), new BMessage( kApplicationPanelTimer ), 500000 );
		if ( fTimer->InitCheck() != B_OK )
			printf("failed to create timer..\n");
	}
}

TApplicationPanel::TApplicationPanel( BMessage *message )
	: TRaisingIconPanel(message), fTimer(0)
{
	if ( fDrawCpuUsageBars )
	{
		fTimer = new BMessageRunner( BMessenger(this), new BMessage( kApplicationPanelTimer ), 500000 );
		if ( fTimer->InitCheck() != B_OK )
			printf("failed to create timer..\n");
	}
}

TApplicationPanel::~TApplicationPanel()
{
	if ( fTimer )
		delete fTimer;
}

status_t TApplicationPanel::Archive( BMessage *into, bool deep ) const
{
	into->AddBool( "DrawCpuUsageBars", fDrawCpuUsageBars );
	into->AddBool( "DoAlphaBlendedActiveHandles", fDoAlphaBlendedActiveHandles );

	return TRaisingIconPanel::Archive( into, deep );
}

BHandler *TApplicationPanel::ResolveSpecifier( BMessage *message, int32 index, BMessage *specifier, int32 what, const char *property )
{
	BPropertyInfo prop_info( const_cast<property_info *>(_application_panel_property_list) );
	if ( prop_info.FindMatch( message, index, specifier, what, property ) >= 0 )
		return this;
	return TRaisingIconPanel::ResolveSpecifier( message, index, specifier, what, property );
}

status_t TApplicationPanel::GetSupportedSuites( BMessage *message )
{
	message->AddString( "suites", "suite/vnd.DM-applicationpanel" );
	BPropertyInfo prop_info( const_cast<property_info *>(_application_panel_property_list) );
	message->AddFlat( "messages", &prop_info );
	return TRaisingIconPanel::GetSupportedSuites(message);
}

bool TApplicationPanel::SetOptions( const char *what, const BMessage *msg )
{
	if ( !strcasecmp( what, "DrawCpuUsageBars" ) )
	{
		if ( msg->FindBool( "data", &fDrawCpuUsageBars ) != B_OK )
			return false;
		if ( !fTimer && fDrawCpuUsageBars )
		{
			fTimer = new BMessageRunner( BMessenger(this), new BMessage( kApplicationPanelTimer ), 500000 );
			if ( fTimer->InitCheck() != B_OK )
				printf("failed to create timer..\n");
		}
		else if ( fTimer && !fDrawCpuUsageBars )
		{
			delete fTimer;
			fTimer = 0;
			TInnerPanel::Invalidate();
		}

		if ( fDrawCpuUsageBars )
			(new BAlert("Warning", "Using this option with BeOS R5 might give some rendering artifacts..", "Bummer" ))->Go(0);
	}
	else if ( !strcasecmp( what, "DoAlphaBlendedActiveHandles" ) )
	{
		if ( msg->FindBool( "data", &fDoAlphaBlendedActiveHandles ) != B_OK )
			return false;
		if ( fDoAlphaBlendedActiveHandles )
			(new BAlert("Warning", "Using this option with BeOS R5 might give some rendering artifacts..", "Bummer" ))->Go(0);
	}
	else
		return TRaisingIconPanel::SetOptions( what, msg );

	return true;
}

bool TApplicationPanel::GetOptions( const char *what, BMessage *msg )
{
	if ( !strcasecmp( what, "DrawCpuUsageBars" ) )
		msg->AddBool( "response", fDrawCpuUsageBars );
	else if ( !strcasecmp( what, "DoAlphaBlendedActiveHandles" ) )
		msg->AddBool( "response", fDoAlphaBlendedActiveHandles );
	else
		return TRaisingIconPanel::GetOptions( what, msg );
	return true;
}

void TApplicationPanel::AttachedToDock( TPanelWindowView *parent )
{
	TRaisingIconPanel::AttachedToDock( parent );
}

void TApplicationPanel::MessageReceived( BMessage *message )
{
	if ( message->what == kApplicationPanelTimer )
	{
		for ( int i=0; i<CountItems(); i++ )
		{
			TPanelIcon *icon = static_cast<TPanelIcon*>(ItemAt(i));
			if ( dynamic_cast<TAppPanelIcon*>( icon ) )
			{
//				Invalidate(icon, true);
				BRect rect;
				IconRect( i, rect );
				rect.right = rect.left+3;
				rect.top += 6;
				rect.bottom = rect.top+26;
				Parent()->Invalidate(rect);
			}
		}
	}
	else
		TRaisingIconPanel::MessageReceived(message);
}

bool TApplicationPanel::AddItem( const char *mime, int32 index )
{
	entry_ref ref;

	if ( be_roster->FindApp( mime, &ref ) != B_OK )
		return false;

	AddItem( ref, index );

	return true;
}

static bool // this is declared static in libtracker.. cant use it :(
FSIsDirFlavor(const BEntry *entry, directory_which directoryType)
{
	StatStruct dir_stat;
	StatStruct entry_stat;
	BVolume volume;
	BPath path;

	if (entry->GetStat(&entry_stat) != B_OK)
		return false;

	if (volume.SetTo(entry_stat.st_dev) != B_OK)
		return false;

	if (find_directory(directoryType, &path, false, &volume) != B_OK)
		return false;

	stat(path.Path(), &dir_stat);

	return dir_stat.st_ino == entry_stat.st_ino
		&& dir_stat.st_dev == entry_stat.st_dev;
}

bool TApplicationPanel::AddItem( entry_ref &ref, int32 index )
{
	BFile fl( &ref, B_READ_ONLY );
	BAppFileInfo info( &fl );

	char mime[ B_MIME_TYPE_LENGTH ];

	TPanelIcon *icon;

	if ( (info.InitCheck() == B_OK) && 
		 ( info.GetSignature( mime ) == B_OK ) &&
		 !strncmp( mime, "application/", 12 )
	   )
	{
		entry_ref _ref;

		if ( be_roster->FindApp( mime, &_ref ) != B_OK )
			return false;

		icon = (TPanelIcon*) ( new TAppPanelIcon( _ref, new BList(), strdup(mime) ) );
	}
	else
	{
		BEntry entry( &ref, true );
		if ( FSIsTrashDir( &entry ) )
			icon = (TPanelIcon*) ( new TTrashIcon() );
		else if ( FSIsDirFlavor( &entry, B_USER_DESKBAR_DIRECTORY ) )
			icon = (TPanelIcon*) ( new TDockbertIcon( ref ) );
		else
			icon = (TPanelIcon*) ( new TTrackerIcon( ref ) );
	}

	if ( icon->InitCheck() != B_OK )
	{
		delete icon;
		return false;
	}
	else
		AddItem( icon, index );

	return true;
}

bool TApplicationPanel::RemoveItem( team_id team)
{
	TAppPanelIcon *icon = ItemWith(team);
	if ( !icon )
		return false;
	icon->Teams()->RemoveItem( (void*)team );
	if ( icon->Teams()->CountItems() == 0 )
		return RemoveItem( icon );
	return true;
}

TAppPanelIcon *TApplicationPanel::ItemWith( const char *sig )
{
	if ( !sig )
		return 0;

	int count = CountItems();
	for ( int i=0; i<count; i++ )
	{
		TPanelIcon *paicon = static_cast<TPanelIcon*>(ItemAt(i));
		TAppPanelIcon *icon = dynamic_cast<TAppPanelIcon*>(paicon);
		if ( icon )
		{
			if ( !strcmp( icon->Signature(), sig ) )
				return icon;
		}
	}

	return 0;
}

TTrackerIcon *TApplicationPanel::ItemWith( entry_ref &ref )
{
	int count = CountItems();
	for ( int i=0; i<count; i++ )
	{
		TPanelIcon *paicon = static_cast<TPanelIcon*>(ItemAt(i));
		TTrackerIcon *icon = dynamic_cast<TTrackerIcon*>(paicon);
		if ( icon )
		{
			if ( icon->Ref() == ref )
				return icon;
		}
	}

	return 0;
}

TAppPanelIcon* TApplicationPanel::ItemWith( team_id team )
{
	int count = CountItems();
	for ( int i=0; i<count; i++ )
	{
		TPanelIcon *paicon = static_cast<TPanelIcon*>(ItemAt(i));
		TAppPanelIcon *icon = dynamic_cast<TAppPanelIcon*>( paicon );
		if ( icon )
		{
			if ( icon->Teams() )
			{
				if ( icon->Teams()->HasItem( reinterpret_cast<void*>(team) ) )
				{
					return icon;
				}
			}
			else
			{
				printf( "something very wrong is going on in here.. blist missing\n");
//				icon->PrintToStream();
			}
		}
	}

	return 0;
}

void TApplicationPanel::IsDragging( BPoint where, uint32 /* transition */ )
{
	TPanelIcon *icon = dynamic_cast<TPanelIcon*>(IconAt( where ));
	if ( icon )
	{
		if ( !icon->AcceptsDrop() )
			icon = 0;
	}
	fParent->SetHighlightedIcon( this, icon );
}

void TApplicationPanel::HandleDroppedMessage( BMessage *message, BPoint point)
{
	TPanelIcon *icon = dynamic_cast<TPanelIcon*>( IconAt( point ) );
	if ( !icon || !icon->AcceptsDrop() || icon->DroppedSomething( message ) != B_OK )
		TInnerPanel::HandleDroppedMessage( message, point );
}

TShortcutPanel::TShortcutPanel( TPanelWindowView *parent )
	: TApplicationPanel(parent)
{
	SetFrame( BRect( 0, 0, 150, fFrameHeight ) );
}

TShortcutPanel::TShortcutPanel( BMessage *message )
	: TApplicationPanel( message )
{
	for ( int i=0; ; i++ )
	{
		BMessage iconarchv;
		if ( message->FindMessage( "tab:icon", i, &iconarchv ) != B_OK )
			break;
		BArchivable *archive = instantiate_dock_object( &iconarchv );
		if ( archive )
		{
			TPanelIcon *icon = dynamic_cast<TPanelIcon*>( archive );
			if ( icon )
			{
				if ( icon->InitCheck() != B_OK )
					delete icon;
				else
				{
					TTrackerIcon *trackericon = dynamic_cast<TTrackerIcon*>(icon);
					if ( trackericon )
						fParent->AddShortcut( this, trackericon->Ref() );
					else
						AddItem( icon, -1 );
				}
			}
		}
	}
}

void TShortcutPanel::AttachedToDock( TPanelWindowView *parent )
{
	TApplicationPanel::AttachedToDock( parent );
}

status_t TShortcutPanel::Archive( BMessage *into, bool deep) const
{
	int count = CountItems();
	for ( int i=0; i<count; i++ )
	{
		BMessage iconarchv;
		if ( static_cast<TPanelIcon*>(ItemAt(i))->Archive( &iconarchv ) == B_OK )
		{
			into->AddMessage( "tab:icon", &iconarchv );
		}
	}

	return TApplicationPanel::Archive( into, deep );
}

BHandler *TShortcutPanel::ResolveSpecifier( BMessage *message, int32 index, BMessage *specifier, int32 what, const char *property )
{
	return TApplicationPanel::ResolveSpecifier( message, index, specifier, what, property );
}

void TShortcutPanel::MessageReceived( BMessage *message )
{
	TApplicationPanel::MessageReceived(message);
}

status_t TShortcutPanel::GetSupportedSuites( BMessage *message )
{
	return TApplicationPanel::GetSupportedSuites(message);
}

bool TShortcutPanel::AddItem( TPanelIcon *icon, int32 index )
{
	if ( CountItems() == 0 && fFrame.Width() == 150.f )
		IncreaseFrame( -150 );
	return TApplicationPanel::AddItem(icon, index);
}
 
bool TShortcutPanel::AddItem( entry_ref &ref, int32 index )
{
	if ( !TApplicationPanel::AddItem( ref, index ) )
		return false;

	return true;
}

bool TShortcutPanel::RemoveItem( team_id id )
{
	TAppPanelIcon *icon = ItemWith(id);
	if ( !icon )
		return false;
	icon->Teams()->RemoveItem((void*)id);
	if ( icon->Teams()->CountItems() == 0 )
		Invalidate(icon, true);
	return true;
}

bool TShortcutPanel::RemoveItem( TPanelIcon *icon, bool deleteit )
{
	if ( deleteit == false )
	{
		bool b = TRaisingIconPanel::RemoveItem( icon, false );
		if ( b && CountItems() == 0 )
			IncreaseFrame( 150 );
		return b;
	}
	else
		Invalidate(icon, true);
	return true;
}

bool TShortcutPanel::SetOptions( const char *what, const BMessage *message )
{
	return TApplicationPanel::SetOptions( what, message );
}

bool TShortcutPanel::GetOptions( const char *what, BMessage *message )
{
	return TApplicationPanel::GetOptions( what, message );
}

void TShortcutPanel::IsDragging( BPoint where, uint32 transition )
{
	TApplicationPanel::IsDragging( where, transition );
}

void TShortcutPanel::HandleDroppedMessage( BMessage *message, BPoint point)
{
	if ( (modifiers() & B_SHIFT_KEY) || ( modifiers() & B_CONTROL_KEY ) )
	{
		fParent->HandleDroppedMessage( message, point );
	}
	else
	{
		TApplicationPanel::HandleDroppedMessage( message, point );
	}
}

void TShortcutPanel::DrawContent( BRect update )
{
	if ( CountItems() == 0 )
	{
		BFont font( be_plain_font );
		font.SetFace( B_ITALIC_FACE | B_BOLD_FACE );
		float width = font.StringWidth( "[ drop items here ]" );
		BFont parentfont;
		fParent->GetFont(&parentfont);
		fParent->SetFont(&font);
		fParent->SetDrawingMode( B_OP_ALPHA );
		fParent->SetBlendingMode( B_CONSTANT_ALPHA, B_ALPHA_OVERLAY );
		fParent->SetHighColor( 0, 0, 0, 100 );
		fParent->DrawString( "[ drop items here ]", BPoint( fFrame.left + ((fFrame.Width() - width) / 2), fIconFrame.top+(fIconFrame.Height()/2) ) );
		fParent->SetFont(&parentfont);
		fParent->SetDrawingMode( B_OP_COPY );
	}
	else
		TApplicationPanel::DrawContent( update );
}

void TShortcutPanel::MouseDownIcon( TPanelIcon *hicon, BPoint point, uint32 buttons)
{
	if ( (buttons == B_PRIMARY_MOUSE_BUTTON) && ( modifiers() & B_CONTROL_KEY ) )
	{
		if ( hicon->Removable() )
			fParent->RemoveShortcut( hicon );
	}
	else
	{
		TApplicationPanel::MouseDownIcon( hicon, point, buttons );
	}
}

#if 0
TReplicantTrayPanel::TReplicantTrayPanel( TReplicantShelfPanel *shelf, bool vertical )
	: TReplicantTrayInternal(vertical), fShelfPanel(shelf)
{
}

void TReplicantTrayPanel::GetPreferredSize( float *preferredWidth, float *preferredHeight )
{
	float width = 0, height = kMinimumTrayHeight;
	
	uint32 id;
	BView *view;
	fShelf->ReplicantAt(IconCount() - 1, &view, &id);
	if (view) {	
		width = view->Frame().right + 3;
	}

	*preferredWidth = width;
	*preferredHeight = height + 1;
}

BPoint TReplicantTrayPanel::LocForReplicant(int32 replicantCount, int32 index, float width)
{
	return TReplicantTrayInternal::LocForReplicant(replicantCount, index, width );
}

void TReplicantTrayPanel::AdjustPlacement()
{
	fShelfPanel->AdjustPlacement();
	TReplicantTrayInternal::AdjustPlacement();
}

void TReplicantTrayPanel::Draw(BRect)
{
	BRect frame = Bounds();

/*	SetHighColor(light);
	StrokeLine(frame.LeftBottom(), frame.RightBottom());
	StrokeLine(frame.RightBottom(), frame.RightTop());
	
	SetHighColor(vdark);
	StrokeLine(frame.RightTop(), frame.LeftTop());
	StrokeLine(frame.LeftTop(), frame.LeftBottom());*/

//	SetHighColor( 40, 40, 40 );
//	StrokeRect(Bounds());

	SetHighColor( 132, 132, 132 );
//	StrokeLine(frame.LeftBottom(), frame.RightBottom());
	StrokeRect(frame);

	BPoint lefttop = frame.LeftTop();
	lefttop.x ++; lefttop.y ++;
	BPoint leftbottom = frame.LeftBottom();
	leftbottom.x ++; leftbottom.y --;
	BPoint righttop = frame.RightTop();
	righttop.x --; righttop.y ++;
	BPoint rightbottom = frame.RightBottom();
	rightbottom.x --; rightbottom.y --;

	SetHighColor( 196, 196, 196 );
	StrokeLine( leftbottom, rightbottom );

	SetHighColor( 164, 164, 164 );
	leftbottom.y --;

	StrokeLine(lefttop, leftbottom);
	StrokeLine(lefttop, righttop);

	lefttop.x ++;
	leftbottom.x ++;
	StrokeLine(lefttop, leftbottom);
	lefttop.y ++;
	righttop.y ++;
	StrokeLine(lefttop, righttop);
}


TReplicantShelfPanel::TReplicantShelfPanel( TPanelWindowView *parent )
	: TInnerPanel(parent)
{
	fTray = new TReplicantTrayPanel( this, false );
	fTray->SetMultiRow(false);

	fParent->AddChild( fTray );

//	fBackFrameColor = fTray->ViewColor();
//	Invalidate();

//	fTray->SetViewColor( fBackFrameColor );
}

TReplicantShelfPanel::~TReplicantShelfPanel()
{
	fParent->RemoveChild( fTray );
}

void TReplicantShelfPanel::SetFrame( BRect r, bool b)
{
	TInnerPanel::SetFrame( r, b );

	BPoint p = fFrameFrame.LeftTop();

	p.x += ( fFrameFrame.Width() - fTray->Bounds().Width() ) / 2;
	p.y += ( fFrameFrame.Height() - fTray->Bounds().Height() ) / 2 +1;

	fTray->MoveTo( p );
}

void TReplicantShelfPanel::AdjustPlacement()
{
	float w, h;

	fTray->GetPreferredSize( &w, &h );

	SetFrame( BRect(0, 0, w+32, fFrameHeight ) );
//	SetFrame( BRect(0, 0, w+32, 35 ) );
}

#endif
