#include "PanelWindowView.h"

#include "PanelWindow.h"
#include "InnerPanel.h"

#include "tracker_private.h"

#include <MessageRunner.h>
#include <Rect.h>
#include <Region.h>
#include <Roster.h>
#include <PropertyInfo.h>
#include <Application.h>
#include <Picture.h>
#include <FindDirectory.h>
#include <Alert.h>

#include <strings.h>

#include <Autolock.h>

//#define USE_WINDOW_SHAPING /* BeOS Dano code*/

property_info _dock_property_list[] = {
	{ "TransparentMenus", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_BOOL_TYPE }, {}, {} },
	{ "DrawOuterFrame", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_BOOL_TYPE }, {}, {} },
	{ "DockLocation", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_STRING_TYPE }, {}, {} },
	{ "BackColor", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_RGB_COLOR_TYPE }, {}, {} },
	{ "OuterFrameColor", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_RGB_COLOR_TYPE }, {}, {} },
	{ "AutoHide", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_BOOL_TYPE}, {}, {} },
	{ "HideEffectDelay", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_FLOAT_TYPE }, {}, {} },
	{ "AlwaysOnTop", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_BOOL_TYPE }, {}, {} },
	{ "HideStandardDeskbar", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_BOOL_TYPE }, {}, {} },
	{ "UseWindowShapping", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_BOOL_TYPE }, {}, {} },
	{ "DebugLevel", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, { B_INT32_TYPE }, {}, {} },

	{ "DebugDump", { B_EXECUTE_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, {}, {}, {} },
	{ "DoCheckup", { B_EXECUTE_PROPERTY }, { B_DIRECT_SPECIFIER }, 0, 0, {}, {}, {} },

	{ "tabs", { B_COUNT_PROPERTIES }, { B_DIRECT_SPECIFIER }, 0, 0, { B_INT32_TYPE }, {}, {} },

	{ "AboutInfo",	{ B_GET_PROPERTY }, { B_DIRECT_SPECIFIER, 0 }, 0, 0, { B_STRING_TYPE }, {}, {} },
	{ "VersionInfo",{ B_GET_PROPERTY }, { B_DIRECT_SPECIFIER, 0 }, 0, 0, { B_STRING_TYPE }, {}, {} },

	{ 0, {}, {}, 0, 0, {}, {}, {} }
};

TPanelWindowView::TPanelWindowView()
	: BView( BRect( 0, 0, 24, 24 ), "PanelWindowView", B_FOLLOW_ALL, B_WILL_DRAW  ),
//	fHighlightIconsLock("HighlightIconsLock"), fPanelListLock("PanelsListLock"),
	fBubbleHelp(0)
{
	fHighlightedIcon = 0;
	fPreviousMouseMovedPanel = 0;
	fIsDraggingFromOutside = false;
	fThisArchive = 0;

	fRunningAppPanel = 0;

	fDebugLevel = 0;
	fOpenMenu = 0;

	SetViewColor( B_TRANSPARENT_COLOR );

	fMyPicture = new BPicture();
}

TPanelWindowView::TPanelWindowView( BMessage *msg )
	: BView( BRect( 0, 0, 24, 24 ), "PanelWindowView", B_FOLLOW_ALL, B_WILL_DRAW ),
//	fHighlightIconsLock("HighlightIconsLock"), fPanelListLock("PanelsListLock"),
	fBubbleHelp(0)
{
	fHighlightedIcon = 0;
	fPreviousMouseMovedPanel = 0;
	fIsDraggingFromOutside = false;

	fRunningAppPanel = 0;

	fThisArchive = new BMessage(*msg);

	fDebugLevel = 0;
	fOpenMenu = 0;

	SetViewColor( B_TRANSPARENT_COLOR );

	fMyPicture = new BPicture();
}

TPanelWindowView::~TPanelWindowView()
{
	DebugCall( 5, "TPanelWindowView::~TPanelWindowView()" );

	if ( fTimer )
		delete fTimer;

	if ( fBubbleHelp )
		delete fBubbleHelp;

	fPanels.Lock();
	int32 count = fPanels.CountItems();
	for ( int i=0; i<count; i++ )
	{
		TInnerPanel *item = fPanels.ItemAt( i );
		delete item;
	}
	fPanels.Unlock();

	if ( fThisArchive )
		delete fThisArchive;

	delete fMyPicture;
}

void TPanelWindowView::AttachedToWindow()
{

	fTimer = new BMessageRunner( BMessenger(Window()), new BMessage(kPanelWindowViewTimer), 999999 );

	new BMessageRunner( BMessenger(this), new BMessage(kDoBubbleHelp), 100000 );

	fLastActiveAppIcon = 0,

	be_roster->StartWatching( BMessenger(this), B_REQUEST_LAUNCHED|B_REQUEST_QUIT|B_REQUEST_ACTIVATED );

	fUseWindowShapping = false;
	fOuterFrameColor = (rgb_color){96,96,96,255};

	fBubbleHelp = new TBubbleHelp();

	if ( fThisArchive )
	{
		// todo
		fColor3 = (rgb_color){218,218,205,255};

		if ( fThisArchive->FindBool( "TransparentMenus", &fUseTransparentMenus ) != B_OK )
			fUseTransparentMenus = false;
		if ( fThisArchive->FindInt32( "Location", &fLocation ) != B_OK )
			fLocation = kLocationBottom;

		rgb_color *temp_c;
		ssize_t _size;
		if ( fThisArchive->FindData( "BackColor", B_RGB_COLOR_TYPE, (const void**)&temp_c, &_size ) != B_OK)
			fColor2 = (rgb_color){229,235,231,255};
		else
			fColor2 = *temp_c;

		if ( fThisArchive->FindData( "OuterFrameColor", B_RGB_COLOR_TYPE, (const void**)&temp_c, &_size ) != B_OK)
			fOuterFrameColor = (rgb_color){96,96,96,255};
		else
			fOuterFrameColor = *temp_c;

		if ( fThisArchive->FindBool( "DrawOuterFrame", &fDrawOuterFrame ) != B_OK )
			fDrawOuterFrame = true;

#ifdef USE_WINDOW_SHAPING
		if ( fThisArchive->FindBool( "UseWindowShapping", &fUseWindowShapping ) != B_OK )
			fUseWindowShapping = true;

		((TPanelWindow*)Window())->SetShowHideLimit( fUseWindowShapping ? 0.1f : 0.f );
#endif

		bool autohide;
		if ( fThisArchive->FindBool( "AutoHide", &autohide ) != B_OK )
			autohide = false;
		((TPanelWindow*)Window())->SetAutoHide( autohide );

		int32 delay;
		if ( fThisArchive->FindInt32( "HideEffectDelay", &delay ) != B_OK )
			delay = kWindowHidderAnimationStaleSpeed;
		((TPanelWindow*)Window())->SetHideEffectDelay( delay );

		if ( fThisArchive->FindBool( "AlwaysOnTop", &fAlwaysOnTop ) != B_OK )
			fAlwaysOnTop = true;

		if ( fThisArchive->FindBool( "HideStandardDeskbar", &fHideStandardDeskbar ) != B_OK )
			fHideStandardDeskbar = false;

		if ( fHideStandardDeskbar )
		{
			BMessage msg(B_SET_PROPERTY);
			msg.AddBool( "data", true );
			msg.AddSpecifier( "Hidden" );
			msg.AddSpecifier( "Window", "Deskbar" );
			be_app->PostMessage(&msg);
		}

		for ( int i=0; ; i++ )
		{
			BMessage inner;
			if ( fThisArchive->FindMessage( "tabs", i, &inner ) != B_OK )
				break;
			inner.AddPointer( "parent", this );
			BArchivable *archive = instantiate_dock_object( &inner );
			if ( archive )
			{
				TInnerPanel *panel = dynamic_cast<TInnerPanel*>(archive);
				if ( panel )
				{
					AddPanel( panel );
					bool _isr;
					if ( inner.FindBool( "IsRunningAppPanel", &_isr ) == B_OK && _isr )
						fRunningAppPanel = cast_as( panel, TApplicationPanel );
				}
			}
		}

		delete fThisArchive;
		fThisArchive = 0;
	}
	else
	{
		// <options>
		fUseTransparentMenus = false;
		fLocation = kLocationBottom;
		fColor2 = (rgb_color){229,235,231,255};
		fColor3 = (rgb_color){218,218,205,255};
		fDrawOuterFrame = true;
		fUseWindowShapping = false;
		fHideStandardDeskbar = false;
		fAlwaysOnTop = false;
		// </options>
	}

	if ( fAlwaysOnTop )
		Window()->SetFeel( B_FLOATING_ALL_WINDOW_FEEL );
	else
		Window()->SetFeel( B_NORMAL_WINDOW_FEEL );

	if ( fPanels.CountItems() == 0 )
	{
		TShortcutPanel *cpanel = new TShortcutPanel(this);
		AddPanel( cpanel );

		cpanel->AddItem( new TTrashIcon() );
		cpanel->AddItem( new TWorkspacesIcon() );
		cpanel->AddItem( new TClockIcon() );

		BEntry entry( "/boot/system/Tracker" );
		entry_ref ref;
		entry.GetRef( &ref );
		AddShortcut( cpanel, ref );

		BPath path;
		if ( find_directory( B_USER_CONFIG_DIRECTORY, &path ) == B_OK )
		{
			path.Append( "be" );
			BEntry entry2( path.Path() );
			entry2.GetRef( &ref );
			AddShortcut( cpanel, ref );
		}

		cpanel = new TShortcutPanel(this);
		AddPanel( cpanel );
		AddShortcut( cpanel, "application/x-vnd.Haiku-WebPositive" );
		AddShortcut( cpanel, "application/x-vnd.Sugoi-BeShare" );

		fRunningAppPanel = new TApplicationPanel(this);
		AddPanel( fRunningAppPanel );
	}

//	AddPanel( new TReplicantShelfPanel(this) );
}

// old interface
void TPanelWindowView::AddTeam( BList *team, BBitmap *icon, char *name, char *sig)
{
	entry_ref ref;

	DebugCall( 6, "TPanelWindowView::AddTeam( %p, %p, %s, %s )", team, icon, name, sig );

	if ( be_roster->FindApp(sig, &ref) == B_OK )
	{
		AddRunning( fRunningAppPanel, new TAppPanelIcon( ref, team, sig) );
	}

	if ( icon )
		delete icon;
	if ( name )
		delete name;
}

void TPanelWindowView::AddTeam( team_id id, const char *sig, entry_ref ref )
{
	DebugCall( 6, "TPanelWindowView::AddTeam( %i, %s, ref )", (int)id, sig );

	BList *list = new BList;
	list->AddItem( (void*) id );
	AddRunning( fRunningAppPanel, new TAppPanelIcon( ref, list, strdup(sig) ) );
}

void TPanelWindowView::RemoveTeam( team_id id )
{
	DebugCall( 6, "TPanelWindowView::RemoveTeam( %i )", (int)id );

	TAppPanelIcon *icon = ItemWith(id);
	if ( icon )
	{
		if ( fLastActiveAppIcon == icon )
		{
			fLastActiveAppIcon->SetActive(false);
			fLastActiveAppIcon = 0;
		}
		dynamic_cast<TApplicationPanel*>(icon->fParent)->RemoveItem(id);
	}
	else
		DebugCall( 8, "\tteam isnt in any of the icons in this dock" );
}

void TPanelWindowView::MessageReceived( BMessage *message )
{
	if ( fDebugLevel >= 20 )
		message->PrintToStream();

	if ( message->what == B_GET_PROPERTY
		 || message->what == B_SET_PROPERTY
		 || message->what == B_COUNT_PROPERTIES
		 || message->what == B_CREATE_PROPERTY
		 || message->what == B_DELETE_PROPERTY
		 || message->what == B_EXECUTE_PROPERTY )
	{
		int32 index, what;
		BMessage specifier;
		const char *property;
		if ( message->GetCurrentSpecifier( &index, &specifier, &what, &property ) == B_OK )
		{
			BMessage reply( B_REPLY );
			if ( message->what == B_GET_PROPERTY
				 || message->what == B_COUNT_PROPERTIES
				 || message->what == B_EXECUTE_PROPERTY )
			{
				if ( GetOptions( property, &reply ) )
					reply.AddInt32( "error", 0 );
				else
					reply.AddInt32( "error", -1 );
			}
			else if ( message->what == B_SET_PROPERTY )
			{
				if ( SetOptions( property, message ) )
					reply.AddInt32( "error", 0 );
				else
					reply.AddInt32( "error", -1 );
			}
			else if ( message->what == B_CREATE_PROPERTY )
			{
				if ( !strcasecmp( property, "tab" ) )
				{
					int32 index;
					if ( message->FindInt32( "index", &index ) != B_OK )
						index = -1;
					TShortcutPanel *panel = new TShortcutPanel( this );

					fPanels.Lock();
					if ( index >= 0 )
					{
						TInnerPanel *rightpanel = fPanels.ItemAt(index);
						AddPanel( panel, rightpanel );
					}
					else
					{
						AddPanel( panel );
					}
					fPanels.Unlock();

					ChangedSize(0);

					reply.AddInt32( "error", 0 );
				}
				else
					reply.AddInt32( "error", -1 );
			}
			else if ( message->what == B_DELETE_PROPERTY )
			{
				int32 index;
				if ( specifier.FindInt32( "index", &index ) != B_OK )
					reply.AddInt32( "error", -1 );
				else
				{
					fPanels.Lock();
					TInnerPanel *panel = fPanels.ItemAt(index);
					if ( !panel )
						reply.AddInt32( "error", -1 );
					else
					{
						if ( panel != fRunningAppPanel )
						{
							RemovePanel( panel );
							reply.AddInt32( "error", 0 );
						}
						else
							reply.AddInt32( "error", -1 );
					}
					fPanels.Unlock();
				}
			}
			message->SendReply( &reply );
		}
		return;
	}

	if ( message->WasDropped() )
	{
		BPoint point = message->DropPoint();
		ConvertFromScreen( &point );
		TInnerPanel *panel = PanelAt( point );
		if ( message->what == 'IDRG' )
		{
			TPanelIcon *item;
			if ( message->FindPointer( "source", (void**)&item ) == B_OK )
			{
				TRaisingIconPanel *previous_parent = item->fParent;
				TRaisingIconPanel *rpanel;
				if ( modifiers() & B_CONTROL_KEY )
				{
					rpanel = new TShortcutPanel(this);
					bool left = false;
					if ( point.x < (panel->Frame().left+(panel->Frame().Width()/2) ) )
					{
						left = true;
					}
						
					rpanel = new TShortcutPanel(this);
					AddPanel( rpanel, left ? panel : 0 );
				}
				else
					rpanel = dynamic_cast<TRaisingIconPanel*>(panel);
				if ( rpanel && rpanel != fRunningAppPanel )
				{
					TPanelIcon *icon = rpanel->IconAt( point, true );
					if ( previous_parent == fRunningAppPanel && dynamic_cast<TShortcutPanel*>(rpanel) )
					{
						int32 index = rpanel->IndexOf(icon);
						AddShortcut( dynamic_cast<TShortcutPanel*>(rpanel), dynamic_cast<TAppPanelIcon*>(item)->Ref(), index );
					}
					else if ( !dynamic_cast<TTrashIcon*>(icon) || (modifiers() & B_SHIFT_KEY) )
					{
						previous_parent->RemoveItem( item, false );
						int32 index = rpanel->IndexOf(icon);
						rpanel->AddItem( item, index );
					}
					else
					{
						if ( item->Removable() )
							RemoveShortcut(item);
					}
					if ( previous_parent->CountItems() == 0 && previous_parent != fRunningAppPanel )
						RemovePanel( previous_parent );
				}
			}
		}
		else
		{
			if ( panel && panel->HitsFrame( point ) )
			{
				panel->HandleDroppedMessage( message, point );
			}
			else
			{
				HandleDroppedMessage( message, point );
			}
		}

		return;
	}

	switch ( message->what )
	{
	case kPanelWindowViewTimer:
		{
			if ( DoIconSmallerWithTime() == 0 )
			{
				fTimer->SetInterval( 999999999 );
			}
			break;
		}
	case T_MENU_CLOSED:
		{
			DebugCall( 8, "Got T_MENU_CLOSED" );

			TAwarePopupMenu *source;
			if ( message->FindPointer( "source", (void**)&source ) == B_OK )
			{
				if ( source == fOpenMenu )
				{
					DebugCall( 9, "fOpenMenu is now 0" );

					fOpenMenu = 0;
					BPoint point;
					uint32 buttons;
					GetMouse( &point, &buttons );
					if ( !Bounds().Contains( point ) )
						Window()->PostMessage(B_EXITED_VIEW);
				}
			}
			break;
		}
	case B_SOME_APP_LAUNCHED:
		{
			team_id tid;
			if ( message->FindInt32( "be:team", &tid ) != B_OK )
				break;
			const char *sig;
			if ( message->FindString( "be:signature", &sig ) != B_OK )
				break;
			entry_ref ref;
			if ( message->FindRef( "be:ref", &ref ) != B_OK )
				break;
			int32 flags;
			if ( message->FindInt32( "be:flags", &flags ) != B_OK )
				break;

			if ( sig && strlen(sig) && ( ( flags & B_BACKGROUND_APP ) == 0 ) )
				AddTeam( tid, sig, ref );

			break;
		}
	case B_SOME_APP_QUIT:
		{
			team_id tid;
			if ( message->FindInt32( "be:team", &tid ) != B_OK )
				break;

			RemoveTeam( tid );

			break;
		}
	case B_SOME_APP_ACTIVATED:
		{
			team_id tid;
			if ( message->FindInt32( "be:team", &tid ) == B_OK )
			{
				TAppPanelIcon *icon = ItemWith( tid );

				if ( icon != fLastActiveAppIcon )
				{
					DebugCall( 10, "B_SOME_APP_ACTIVATED %p[..]->%p[%i]", fLastActiveAppIcon, icon, tid );

					if ( fLastActiveAppIcon )
						fLastActiveAppIcon->SetActive( false );
					if ( icon )
						icon->SetActive( true );
					fLastActiveAppIcon = icon;
				}
			}

			BString temp;
			message->FindString( "be:signature", &temp );

			if ( temp != "application/x-vnd.Be-TSKB" )
				fLastActivatedAppSig = temp;

			break;
		}
	case kDoBubbleHelp:
		{
			BPoint point;
			uint32 buttons;
			GetMouse(&point, &buttons);
			if ( fPreviousMousePosition != point )
			{
				if ( fBubbleCounter > 0 )
				{
					if ( fBubbleCounter >= 6 )
					{
						fBubbleHelp->HideBubble();
					}
					fBubbleCounter = 0;
				}
				fPreviousMousePosition = point;
			}
			else
			{
				BRegion region;
				GetClippingRegion(&region);
				if ( region.Contains( point ) )
				{
					fBubbleCounter ++;
					if ( fBubbleCounter == 6 )
					{
						ConvertToScreen(&point);
						TBubbleTarget *target = fBubbleHelp->TargetAt( point );
						if (dynamic_cast<TTrackerIcon*>(target)) {
							TTrackerIcon *trackerIcon = dynamic_cast<TTrackerIcon*>(target);
							point.x = Window()->Frame().left + trackerIcon->ContentLocation().x + trackerIcon->Frame().Width() + 4;
							point.y = Window()->Frame().top;
							float height = TBubbleHelp::BubbleHeight(target);
							point.y += height;
							point.y += (Window()->Frame().Height() - height)/2 -4;
						}
						fBubbleHelp->ShowBubble( point, target );
					}
//					else if ( fBubbleCounter == 12 )
//					{
//						fBubbleHelp->HideBubble();
//					}
				}
			}
			break;
		}
	case 'flsh':
		{
			BMessenger target;
			if ( message->FindMessenger("source", &target ) == B_OK && target.IsValid() )
			{
				TAppPanelIcon *teamicon = ItemWith( target.Team() );
				if ( teamicon )
				{
//					todo: flashing
					teamicon->Flash();
				}
			}
			break;
		}
	case 'mctx':
		{
//			todo: context menus
			break;
		}
	default:
		BView::MessageReceived(message);
	}
}

void TPanelWindowView::MouseDown( BPoint point )
{
	uint32 buttons;
	GetMouse( &point, &buttons );

	if ( !fIsDraggingFromOutside )
	{
		if ( modifiers() & B_SHIFT_KEY )
		{
			int count = 5;
			while ( count )
			{
				count --;
				BPoint pw;
				uint32 mods;
				GetMouse( &pw, &mods );
				if ( !mods )
					break;
				if ( !(modifiers() & B_SHIFT_KEY ) )
					break;
				if ( pw != point )
				{
//					TODO: fix this, we want this on separate thread so drawing doesnt block
//					thread_id tid = spawn_thread( _rearrange_tabs_thread_entry, "ReArrangeTabsThread", B_NORMAL_PRIORITY, this );
//					resume_thread(tid);
					ReArrangeTabsThread();

					return;
				}
				snooze( 500000 / 5 );
			}
		}

		TInnerPanel *panel = PanelAt( point );
		if ( panel )
			panel->MouseDown(point, buttons);
	}
}

void TPanelWindowView::MouseUp( BPoint point )
{
	if ( !fIsDraggingFromOutside )
	{
		uint32 buttons;
		GetMouse( &point, &buttons );
		TInnerPanel *panel = PanelAt( point );
		if ( panel )
			panel->MouseUp(point, buttons);
	}
	else
	{
		fIsDraggingFromOutside = false;
		if ( fPreviousMouseMovedPanel )
		{
			fPreviousMouseMovedPanel->IsDragging( point, B_ENDED_DRAGGING );
			fPreviousMouseMovedPanel = 0;
		}
	}
}

void TPanelWindowView::MouseMoved(BPoint point, uint32 transit, const BMessage * /* message */ )
{
	uint32 buttons;
	GetMouse( &point, &buttons );

	switch(transit)
	{
		case B_ENTERED_VIEW:
			Window()->PostMessage(B_ENTERED_VIEW);
			break;
		case B_EXITED_VIEW:
			if ( !fOpenMenu ) 
				Window()->PostMessage(B_EXITED_VIEW);
			break;
	}

	if ( buttons & ( transit == B_ENTERED_VIEW ) )
		fIsDraggingFromOutside = true;
	else if ( transit == B_EXITED_VIEW && fIsDraggingFromOutside)
	{
		fIsDraggingFromOutside = false;
		if ( fPreviousMouseMovedPanel )
		{
			fPreviousMouseMovedPanel->IsDragging( point, B_ENDED_DRAGGING );
			fPreviousMouseMovedPanel = 0;
		}
	}

	if ( transit == B_EXITED_VIEW )
	{
		if ( fHighlightedIcon )
		{
			SetHighlightedIcon( fHighlightedIcon->fParent, 0 );
		}
		if ( fPreviousMouseMovedPanel )
		{
			fPreviousMouseMovedPanel->MouseMoved( point, B_EXITED_VIEW );
			fPreviousMouseMovedPanel = 0;
		}
	}
	else
	{
		TInnerPanel *panel = PanelAt( point );

		if ( fIsDraggingFromOutside )
		{
			if ( panel == fPreviousMouseMovedPanel )
			{
				if ( panel )
					panel->IsDragging( point, B_IS_DRAGGING );
			}
			else
			{
				if ( panel )
					panel->IsDragging( point, B_STARTED_DRAGGING );
				if ( fPreviousMouseMovedPanel )
					fPreviousMouseMovedPanel->IsDragging( point, B_ENDED_DRAGGING );
			}
	
			fPreviousMouseMovedPanel = panel;
		}
		else
		{
			if ( panel == fPreviousMouseMovedPanel )
			{
				if ( panel )
					panel->MouseMoved( point, B_INSIDE_VIEW );
			}
			else
			{
				if ( panel )
					panel->MouseMoved( point, B_ENTERED_VIEW );
				if ( fPreviousMouseMovedPanel )
					fPreviousMouseMovedPanel->MouseMoved( point, B_EXITED_VIEW );
			}

			fPreviousMouseMovedPanel = panel;
		}
	}
}

void TPanelWindowView::Draw( BRect updateRect )
{
	BRegion Region;
	Region.Include(updateRect);
	ConstrainClippingRegion(&Region);

	float w, h;

	w = Bounds().Width();
	h = Bounds().Height();

	SetHighColor( fColor2 );
	FillRect( BRect( 0, 0, h, h ) );
	FillRect( BRect( w-h, 0, w, h ) );

	FillRect( BRect( h/2, 0, w-(h/2), h/2) );
	FillRect( BRect( 0, h/2, w, h ) );

	SetHighColor( 196, 196, 176 );
	FillRect( BRect( 0, h - 2, w, h ) );

	fPanels.Lock();
	for ( int i=0; i<fPanels.CountItems(); i++ )
	{
		TInnerPanel *panel = fPanels.ItemAt(i);
		if ( panel->Frame().Intersects( updateRect ) )
		{
			BRect rect( panel->Frame() );
			if ( rect.left < updateRect.left )
				rect.left = updateRect.left;
			if ( rect.right > updateRect.right )
				rect.right = updateRect.right;
			if ( rect.top < updateRect.top )
				rect.top = updateRect.top;
			if ( rect.bottom > updateRect.bottom )
				rect.bottom = updateRect.bottom;
			panel->Draw(rect);
		}
	}
	fPanels.Unlock();

	if ( fDrawOuterFrame )
	{
		SetDrawingMode( B_OP_COPY );
		SetHighColor( fOuterFrameColor );

		BRect bounds = Bounds();
		StrokeLine( bounds.LeftTop(), bounds.RightTop() );
		StrokeLine( bounds.LeftTop(), bounds.LeftBottom() );
		StrokeLine( bounds.RightTop(), bounds.RightBottom() );
	}

	ConstrainClippingRegion(NULL);
}

BHandler *TPanelWindowView::ResolveSpecifier( BMessage *message, int32 index, BMessage *specifier, int32 what, const char *property )
{
	BPropertyInfo prop_info( _dock_property_list );
	if ( !strcasecmp( property, "tab" ) )
	{
		if ( message->what == B_CREATE_PROPERTY || message->what == B_DELETE_PROPERTY )
			return this;
		int32 index;
		if ( specifier->FindInt32( "index", &index ) == B_OK )
		{
			fPanels.Lock();
			TInnerPanel *panel = fPanels.ItemAt(index);
			fPanels.Unlock();
			if ( panel )
			{
				message->PopSpecifier();
				return panel;
			}
		}
	}
	if ( prop_info.FindMatch( message, index, specifier, what, property ) >= 0 )
		return this;
	return BView::ResolveSpecifier( message, index, specifier, what, property );
}

status_t TPanelWindowView::GetSupportedSuites( BMessage *message )
{
	GetDockbertSuites( message );
	return BView::GetSupportedSuites( message );
}

void TPanelWindowView::GetDockbertSuites( BMessage *message )
{
	message->AddString( "suites", "suite/vnd.DM-dockbert" );
	BPropertyInfo prop_info( const_cast<property_info *>(_dock_property_list) );
	message->AddFlat( "messages", &prop_info );
}

int TPanelWindowView::DoIconSmallerWithTime()
{
//	fHighlightIconsLock.Lock();
	fHighlightedIconList.Lock();
	int i, count = fHighlightedIconList.CountItems();
	BList icons_for_removal;

	for ( i=0; i<count; i++ )
	{
		TPanelIcon *icon = fHighlightedIconList.ItemAt(i);

		if ( icon == fHighlightedIcon )
		{
			if ( icon->fZoomStep == 16 )
			{
				icons_for_removal.AddItem(icon);
				continue;
			}
			else
			{
				icon->fZoomStep ++;
			}
		}
		else
		{
			if ( icon->fZoomStep == 0 )
			{
				icons_for_removal.AddItem(icon);
				continue;
			}
			else
			{
				icon->fZoomStep --;
			}
		}
		icon->fParent->Invalidate(icon);
	}

	count = icons_for_removal.CountItems();
	for ( i=0; i<count; i++ )
	{
		fHighlightedIconList.RemoveItem( (TPanelIcon*)icons_for_removal.ItemAt(i) );
	}

	int32 c = fHighlightedIconList.CountItems();

	fHighlightedIconList.Unlock();
//	fHighlightIconsLock.Unlock();

	return c;
}

status_t TPanelWindowView::Archive( BMessage *into, bool deep ) const
{
	BView::Archive( into, deep );

	into->AddBool( "TransparentMenus", fUseTransparentMenus );
	into->AddInt32( "Location", fLocation );

	into->AddData( "BackColor", B_RGB_COLOR_TYPE, &fColor2, sizeof(rgb_color) );
	into->AddData( "OuterFrameColor", B_RGB_COLOR_TYPE, &fOuterFrameColor, sizeof(rgb_color) );

	into->AddBool( "DrawOuterFrame", fDrawOuterFrame );
	into->AddBool( "AutoHide", ((TPanelWindow*)Window())->IsAutoHiding() );
	into->AddBool( "AlwaysOnTop", fAlwaysOnTop );
	into->AddBool( "HideStandardDeskbar", fHideStandardDeskbar );
#ifdef USE_WINDOW_SHAPING
	into->AddBool( "UseWindowShapping", fUseWindowShapping );
#endif
	into->AddInt32( "HideEffectDelay", dynamic_cast<TPanelWindow*>(Window())->CurrentHideEffectDelay() );

	if ( deep )
	{
		int count = fPanels.CountItems();
		for ( int i=0; i<count; i++ )
		{
			TInnerPanel *panel = fPanels.ItemAt(i);
			if ( panel->WorthArchiving() )
			{
				BMessage archive;
				panel->Archive(&archive);
				if ( panel == fRunningAppPanel )
					archive.AddBool( "IsRunningAppPanel", true );
				into->AddMessage( "tabs", &archive );
			}
		}
	}

	return B_OK;
}

BArchivable *TPanelWindowView::Instantiate( BMessage *from )
{
	if ( validate_instantiation(from, "TPanelWindowView"))
		return new TPanelWindowView(from);
	return NULL;
}

bool TPanelWindowView::SetOptions( const char *option, const BMessage *msg )
{
	if ( !strcasecmp( option, "TransparentMenus" ) )
	{
		if ( msg->FindBool( "data", &fUseTransparentMenus ) != B_OK )
			return false;
	}
	else if ( !strcasecmp( option, "DrawOuterFrame" ) )
	{
		if ( msg->FindBool( "data", &fDrawOuterFrame ) != B_OK )
			return false;
		Invalidate();
	}
	else if ( !strcasecmp( option, "DockLocation" ) )
	{
		BString where;
		if ( msg->FindString( "data", &where ) != B_OK )
			return false;

		int32 fPreviousLocation = fLocation;

		if ( where == "top" )
			fLocation = kLocationTop;
		else if ( where == "bottom" )
			fLocation = kLocationBottom;
		else if ( where == "left" )
			fLocation = kLocationLeft;
		else if ( where == "right" )
			fLocation = kLocationRight;

		if ( fLocation != kLocationBottom )
		{
			fLocation = fPreviousLocation;
			(new BAlert("Warning", "Locations other than bottom aren't implemented in this build.", "Bummer"))->Go(0);
		}
		else
		{
			if ( fPreviousLocation != fLocation )
				((TPanelWindow*)Window())->DockChangedPlaces(fPreviousLocation);
		}
	}
	else if ( !strcasecmp( option, "BackColor" ) )
	{
		rgb_color *temp_c;
		ssize_t _size;
		if ( msg->FindData( "data", B_RGB_COLOR_TYPE, (const void**)&temp_c, &_size ) != B_OK)
			return false;
		fColor2 = *temp_c;
		Invalidate();
	}
	else if ( !strcasecmp( option, "OuterFrameColor" ) )
	{
		rgb_color *temp_c;
		ssize_t _size;
		if ( msg->FindData( "data", B_RGB_COLOR_TYPE, (const void**)&temp_c, &_size ) != B_OK)
			return false;
		fOuterFrameColor = *temp_c;
		Invalidate();
	}
	else if ( !strcasecmp( option, "AutoHide" ) )
	{
		bool autohide;
		if ( msg->FindBool( "data", &autohide ) != B_OK )
			return false;
		((TPanelWindow*)Window())->SetAutoHide(autohide);
	}
	else if ( !strcasecmp( option, "HideEffectDelay" ) )
	{
		int32 delay;
		if ( msg->FindInt32( "data", &delay ) != B_OK )
			return false;
		((TPanelWindow*)Window())->SetHideEffectDelay( delay );
	}
	else if ( !strcasecmp( option, "AlwaysOnTop" ) )
	{
		if ( msg->FindBool( "data", &fAlwaysOnTop ) != B_OK )
			return false;
		if ( fAlwaysOnTop )
			Window()->SetFeel( B_FLOATING_ALL_WINDOW_FEEL );
		else
			Window()->SetFeel( B_NORMAL_WINDOW_FEEL );
	}
	else if ( !strcasecmp( option, "HideStandardDeskbar" ) )
	{
		if ( msg->FindBool( "data", &fHideStandardDeskbar ) != B_OK )
			return false;
		
		BMessage msg(B_SET_PROPERTY);
		msg.AddBool( "data", fHideStandardDeskbar );
		msg.AddSpecifier( "Hidden" );
		msg.AddSpecifier( "Window", "Deskbar" );
		be_app->PostMessage(&msg);
	}
#ifdef USE_WINDOW_SHAPING
	else if ( !strcasecmp( option, "UseWindowShapping" ) )
	{
		if ( msg->FindBool( "data", &fUseWindowShapping ) != B_OK )
			return false;

		((TPanelWindow*)Window())->SetShowHideLimit( fUseWindowShapping ? 0.1f : 0.f );
		ChangedSize(0);
	}
#endif
	else if ( !strcasecmp( option, "DebugLevel" ) )
	{
		if ( msg->FindInt32( "data", &fDebugLevel ) != B_OK )
			return false;
	}
	else
		return false;

	return true;
}

bool TPanelWindowView::GetOptions( const char *option, BMessage *msg )
{
	if ( !strcasecmp( option, "TransparentMenus" ) )
		msg->AddBool( "response", fUseTransparentMenus );
	else if ( !strcasecmp( option, "DrawOuterFrame" ) )
		msg->AddBool( "response", fDrawOuterFrame );
	else if ( !strcasecmp( option, "DockLocation" ) )
	{
		BString where;
		switch ( fLocation )
		{
		case kLocationBottom:
			where = "bottom"; break;
		case kLocationTop:
			where = "top"; break;
		case kLocationLeft:
			where = "left"; break;
		case kLocationRight:
			where = "right"; break;
		}
		msg->AddString( "response", where );
	}
	else if ( !strcasecmp( option, "BackColor" ) )
		msg->AddData( "response", B_RGB_COLOR_TYPE, &fColor2, sizeof(fColor2) );
	else if ( !strcasecmp( option, "OuterFrameColor" ) )
		msg->AddData( "response", B_RGB_COLOR_TYPE, &fOuterFrameColor, sizeof(fOuterFrameColor) );
	else if ( !strcasecmp( option, "AutoHide" ) )
		msg->AddBool( "response", ((TPanelWindow*)Window())->IsAutoHiding() );
	else if ( !strcasecmp( option, "HideEffectDelay" ) )
		msg->AddInt32( "response", ((TPanelWindow*)Window())->CurrentHideEffectDelay() );
	else if ( !strcasecmp( option, "AlwaysOnTop" ) )
		msg->AddBool( "response", fAlwaysOnTop );
	else if ( !strcasecmp( option, "HideStandardDeskbar" ) )
		msg->AddBool( "response", fHideStandardDeskbar );
#ifdef USE_WINDOW_SHAPING
	else if ( !strcasecmp( option, "UseWindowShapping" ) )
		msg->AddBool( "response", fUseWindowShapping );
#endif
	else if ( !strcasecmp( option, "DebugLevel" ) )
		msg->AddInt32( "response", fDebugLevel );
	else if ( !strcasecmp( option, "DebugDump" ) )
		DoDebugDump();
	else if ( !strcasecmp( option, "DoCheckup" ) )
		DoCheckup();
	else if ( !strcasecmp( option, "tabs" ) )
	{
//		fPanelListLock.Lock();
		fPanels.Lock();
		msg->AddInt32( "response", fPanels.CountItems() );
		fPanels.Unlock();
//		fPanelListLock.Unlock();
	}
	else if ( !strcasecmp( option, "AboutInfo" ) )
		msg->AddString( "about", BString("DockBert - by Hugo Santos (linn) and Daniel T. Bender (HarvestMoon)") );
	else if ( !strcasecmp( option, "VersionInfo" ) )
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

void TPanelWindowView::HandleDroppedMessage( BMessage *message, BPoint point )
{
	if ( message->what == B_PASTE )
	{
		rgb_color *r;
		ssize_t s;

		if ( message->FindData( "RGBColor", B_RGB_COLOR_TYPE, (const void**)&r, &s ) == B_OK )
		{
			BMessage msg;
			msg.AddData( "data", B_RGB_COLOR_TYPE, r, sizeof(rgb_color) );
			SetOptions( "BackColor", &msg );
		}
	}
	else if ( message->what == B_SIMPLE_DATA || message->what == 'MIME' )
	{
		TInnerPanel *panel = PanelAt( point );

		if ( (fShortcutPanelList.CountItems() == 0) || ( modifiers() & B_CONTROL_KEY ) )
		{
			bool left = false;
			if ( point.x < (panel->Frame().left+(panel->Frame().Width()/2) ) )
			{
				left = true;
			}
						
			TShortcutPanel *cpanel = new TShortcutPanel(this);
			AddPanel( cpanel, left ? panel : 0 );
			panel = cpanel;
		}

		TShortcutPanel *pn = dynamic_cast<TShortcutPanel*>( panel );

		if ( !pn )
			pn = static_cast<TShortcutPanel*>( fShortcutPanelList.FirstItem() );

		if ( pn )
		{
			entry_ref ref;
			char *data;
			ssize_t size;
			int32 index = pn->IndexOf(pn->IconAt(point, true));
			if ( message->FindRef( "refs", &ref ) == B_OK )
			{
				AddShortcut( pn, ref, index );
			}
			else if ( message->FindData( "text/plain", 'MIME', (const void**)&data, &size ) == B_OK )
			{
				char _data[256];
				strncpy( _data, data, size );
				data[size] = 0;
				BString string( data );

				if ( string == "clock" )
					pn->AddItem( new TClockIcon(), index );
				else if ( string == "trash" )
					pn->AddItem( new TTrashIcon(), index );
				else if ( string == "workspaces" )
					pn->AddItem( new TWorkspacesIcon(), index );
				else if ( string == "separator" )
					pn->AddItem( new TSeparatorIcon(), index );
				else if ( string == "showdesktop" )
					pn->AddItem( new TShowDesktopIcon(), index );
				else if ( string == "!experimental!replicant_tab" )
				{
// todo: Fix this and the header thingo
//					AddPanel( new TReplicantShelfPanel(this) );
//					ChangedSize(0);
				}
				else if ( !strcasecmp( string.String(), "beos") )
				{
					BAlert *alert = new BAlert( "Easter Egg", "Is BeOS the best desktop operating system ever?", "Of Course!!!", "Don't think so...", "What's BeOS?" );
					int32 what = alert->Go();
					if ( what == 1 )
						(new BAlert("Answer", "BLASPHEMY!!!", "I'm evil"))->Go();
					else if ( what == 2 )
						(new BAlert("Answer", "MEDICCC! MEDICCC! We need a medic here!!", "Uh?"))->Go();
				}
				else if ( !strcasecmp( string.String(), "dockbert" ) )
				{
					char s[512];
					sprintf(s, "Build Info:\nVersion: %s %s [build %s]\nBuild Date: %s", BUILDVERSION, BUILDSTATUS, BUILDNR, BUILDDATE );
					(new BAlert( "Build Info", s, "Nice" ))->Go();
				}
				else if ( !strcasecmp( string.String(), "black-karma" ) )
				{
					(new BAlert( "Easter Egg", "hm.. two nice people developing for the best OS ever.. nice and simple.. even logic :-) Say hi to Hugo (hugo@black-karma.de) or to Daniel (daniel@black-karma.de)", "Uh?!" ))->Go();
				}
				else if ( !strcasecmp( string.String(), "microsoft" ) || !strcasecmp( string.String(), "apple" ) )
				{
					(new BAlert( "Easter Egg", "Evil!", "eckkkkk" ))->Go();
					be_app->PostMessage(B_QUIT_REQUESTED);
				}
				else
				{
					BEntry entry( string.String() );
					if ( entry.GetRef( &ref) == B_OK )
					{
						AddShortcut( pn, ref );
					}
				}
			}
		}
	}
}

void TPanelWindowView::RegisterWorkspaceChange( BMessenger *messenger )
{
	if ( !fWorkspaceChangeNotifyList.HasItem( messenger ) )
		fWorkspaceChangeNotifyList.AddItem( messenger );
}

void TPanelWindowView::WorkspaceChanged()
{
	BList to_remove;
	int i;

	for ( i=0; i<fWorkspaceChangeNotifyList.CountItems(); i++ )
	{
		BMessenger *mess = static_cast<BMessenger*>(fWorkspaceChangeNotifyList.ItemAt(i));
		if ( !mess->IsValid() )
			to_remove.AddItem(mess);
		else
			mess->SendMessage( B_WORKSPACE_ACTIVATED );
	}

	for ( i=0; i<to_remove.CountItems(); i++ )
	{
		fWorkspaceChangeNotifyList.RemoveItem( to_remove.ItemAt(i) );
	}
}

void TPanelWindowView::ChangedSize( TInnerPanel *panel )
{
	BAutolock( &fPanels.lock );

	if ( !panel )
	{
		if ( fPanels.FirstItem() )
			ChangedSize( fPanels.FirstItem() );
		return;
	}

	if ( !fPanels.HasItem( panel ) )
	{
		return;
	}

	int index = fPanels.IndexOf(panel);
	TInnerPanel *prev = fPanels.ItemAt(index-1);

	BRect rect = panel->Frame();
	float w = rect.Width();
	rect.left = prev ? prev->Frame().right : 0;
	rect.right = rect.left + w;
	// hm.. :
/*	rect.bottom = rect.Height();
	rect.top = Bounds().Height() - rect.Height();
	rect.bottom += rect.top;*/
	Window()->Lock();
	panel->SetFrame(rect, false);
	panel->Invalidate();
	Window()->Unlock();

	if ( (index+1) < fPanels.CountItems() )
	{
		ChangedSize( fPanels.ItemAt(index+1) );
	}

	float width = 0;
	float height = 0;

	for ( index=0; index<fPanels.CountItems(); index++ )
	{
		panel = fPanels.ItemAt(index);
		width += panel->Frame().Width();
		if ( panel->Frame().Height() > height )
			height = panel->Frame().Height();
	}

	Window()->Lock();

	Window()->ResizeBy( width - Window()->Bounds().Width(), height - Window()->Bounds().Height() );

#ifdef USE_WINDOW_SHAPING
	BuildViewsPicture(fUseWindowShapping);
	Window()->ClipWindowToPicture( fMyPicture, BPoint(0, 0), 0 );
#endif

	dynamic_cast<TPanelWindow*>(Window())->CenterCorrectly();

	Window()->Unlock();
}

void TPanelWindowView::SetHighlightedIcon( TRaisingIconPanel *parent, TPanelIcon *icon )
{
	BAutolock( &fHighlightedIconList.lock );

	bool activate_timer = false;

	if ( icon == 0 )
	{
		if ( fHighlightedIcon )
		{
			if ( fHighlightedIcon->fParent == parent )
			{
				fHighlightedIconList.AddItem( fHighlightedIcon );
				fHighlightedIcon = 0;
				activate_timer = true;
			}
		}
	}
	else
	{
		if ( icon != fHighlightedIcon )
		{
			if ( fHighlightedIcon )
				fHighlightedIconList.AddItem( fHighlightedIcon );
			fHighlightedIconList.AddItem( icon );
			fHighlightedIcon = icon;
			activate_timer = true;
		}
	}

	if ( activate_timer )
		fTimer->SetInterval( kIconHiddingSpeed );
}

void TPanelWindowView::AddToIconAnimationList( TPanelIcon *icon )
{
	BAutolock( &fHighlightedIconList.lock );
	if ( !fHighlightedIconList.HasItem(icon) )
	{
		fHighlightedIconList.AddItem( icon );
		fTimer->SetInterval( kIconHiddingSpeed );
	}
}

void TPanelWindowView::RemoveIconFromAnimationList( TPanelIcon *icon )
{
	BAutolock( &fHighlightedIconList.lock );
	fHighlightedIconList.RemoveItem( icon );
	if ( icon == fHighlightedIcon )
	{
		fHighlightedIcon = 0;
		icon->fZoomStep = 0;
		icon->fParent->Invalidate(icon);
	}
}

TInnerPanel *TPanelWindowView::PanelAt( BPoint point )
{
	BAutolock( &fPanels.lock );

	int count = fPanels.CountItems();
	TInnerPanel *panel;

	for ( int i=0; i<count; i++ )
	{
		panel = fPanels.ItemAt(i);
		if ( panel->Frame().Contains( point ) )
		{
			return panel;
		}
	}

	return 0;
}

void TPanelWindowView::BuildViewsPicture( bool mode )
{
	BAutolock(Window());
	BeginPicture( fMyPicture );

	float w, h;

	w = Bounds().Width();
	h = Bounds().Height();

/*	FillArc( BRect( 0, 0, h, h ), 90, 90 );
	FillArc( BRect( w-h, 0, w, h ), 0, 90 );

	FillRect( BRect( h/2, 0, w-(h/2), h/2) );
	FillRect( BRect( 0, h/2, w, h ) );*/

	if ( mode )
	{
		FillRect( BRect( 0, h - 2, w, h ) );

		fPanels.Lock();
		for ( int i=0; i<fPanels.CountItems(); i++ )
		{
			TInnerPanel *panel = fPanels.ItemAt(i);
			if ( panel->Frame().Width() > 30 )
				panel->DrawBackFrame();
		}
		fPanels.Unlock();
	}
	else
		FillRect( BRect( 0, 0, w, h ) );

	EndPicture();
}

void TPanelWindowView::AddPanel( TInnerPanel *panel, TInnerPanel *before )
{
	BAutolock( &fPanels.lock );

	float width = 0;
	TInnerPanel *p;
	int index, count;
	int where = fPanels.IndexOf( before );

	if ( !before )
		count = fPanels.CountItems();
	else
		count = where;

	for ( index=0; index<count; index++ )
	{
		p = fPanels.ItemAt(index);
		width += p->Frame().Width();
	}

	BRect rect = panel->Frame();
	rect.left += width;
	rect.right += width;

	panel->SetFrame( rect );

	if ( where == -1 )
		fPanels.AddItem( panel );
	else
		fPanels.AddItem( panel, where );

	Invalidate();
}

void TPanelWindowView::RemovePanel( TInnerPanel *panel )
{
	BAutolock( &fPanels.lock );
	fPanels.RemoveItem( panel );
	if ( fPreviousMouseMovedPanel == panel )
		fPreviousMouseMovedPanel = 0;
	delete panel;
	ChangedSize(0); // force reposition of other panels
}

int32 TPanelWindowView::IndexOf( TInnerPanel *panel )
{
	BAutolock( &fPanels.lock );
	int32 index = fPanels.IndexOf(panel);
	return index;
}

bool TPanelWindowView::MoveTabTo( TInnerPanel *panel, int32 index )
{
	BAutolock( &fPanels.lock );
	int32 prev_index = fPanels.IndexOf(panel);
	if ( prev_index < 0 )
	{
		return false;
	}
	fPanels.RemoveItem(panel);
	fPanels.AddItem(panel, index);

	if ( index > prev_index )
		panel = fPanels.ItemAt(prev_index);

	ChangedSize( panel );

	return true;
}

int32 TPanelWindowView::CountPanels()
{
	BAutolock( &fPanels.lock );
	int32 count = fPanels.CountItems();
	return count;
}

void TPanelWindowView::AddRunning( TApplicationPanel *panel, TAppPanelIcon *icon )
{
	DebugCall( 5, "TPanelWindowView::AddRunning( %p, %p [parent: %p] )", panel, icon, icon->fParent );

	if ( !panel || !icon )
		return;

//	fPanelListLock.Lock();

/*	int count = fShortcutPanelList.CountItems();
	TAppPanelIcon *ali;

	for ( int i=0; i<count; i++ )
	{
		TShortcutPanel *spanel = static_cast<TShortcutPanel*>(fShortcutPanelList.ItemAt(i));
		if ( spanel )
		{
			ali = spanel->ItemWith( icon->Signature() );

			if ( ali )
			{
				DebugCall( 6, "\tfound icon with same signature, going to add teams to that" );

				ali->Teams()->AddList( icon->Teams() );
				delete icon;
				spanel->Invalidate(ali, true);

				fPanelListLock.Unlock();

				return;
			}
		}
		else
			DebugCall( 1, "AddRunning(): getting NULLs from fShortcutPanelList?!" );
	}

	if ( panel == fRunningAppPanel )
	{
		ali = panel->ItemWith( icon->Signature() );
		if ( ali )
		{
			ali->Teams()->AddList( icon->Teams() );
			delete icon;
			panel->Invalidate(ali);

			fPanelListLock.Unlock();

			return;
		}
	}*/

	TAppPanelIcon *previtem = ItemWith( icon->Signature() );
	if ( previtem )
	{
		previtem->Teams()->AddList( icon->Teams() );
		delete icon;
		previtem->fParent->Invalidate(previtem);
	}
	else
		panel->AddItem( icon );

//	fPanelListLock.Unlock();
}

void TPanelWindowView::AddShortcut( TShortcutPanel *panel, entry_ref ref, int32 index )
{
	if ( !fShortcutPanelList.HasItem( panel ) )
		fShortcutPanelList.AddItem( panel );

	DebugCall( 5, "TPanelWindowView::AddShortcut( %p, ref, %i )", panel, (int)index );

	if ( fRunningAppPanel )
	{
		entry_ref aref;
		be_roster->FindApp( &ref, &aref );
		TTrackerIcon *ali = fRunningAppPanel->ItemWith( aref );
		DebugCall( 6, "\tfound this already in RunningAppPanel" );
		if ( ali )
		{
			dynamic_cast<TRaisingIconPanel*>(fRunningAppPanel)->RemoveItem( ali, false );
			dynamic_cast<TRaisingIconPanel*>(panel)->AddItem( ali, index );
			return;
		}
	}

	panel->AddItem( ref, index );
}

void TPanelWindowView::AddShortcut( TShortcutPanel *panel, const char *sig )
{
	entry_ref ref;
	if ( be_roster->FindApp( sig, &ref ) == B_OK )
		AddShortcut( panel, ref );
}

void TPanelWindowView::RemoveShortcut( TPanelIcon *hicon )
{
	DebugCall( 5, "TPanelWindowVoew::RemoveShortcut( %p [parent: %p] )", hicon, hicon->fParent );

	if ( hicon )
	{
		TInnerPanel *parent = hicon->fParent->CountItems() > 1 ? 0 : hicon->fParent;
		TAppPanelIcon *icon = dynamic_cast<TAppPanelIcon*>(hicon);

		if ( fLastActiveAppIcon == hicon )
		{
			fLastActiveAppIcon->SetActive(false);
			fLastActiveAppIcon = 0;
		}

		hicon->fParent->RemoveItem( hicon, false );

		if ( icon && icon->Teams()->CountItems() > 0 )
			AddRunning( fRunningAppPanel, icon );
		else
			delete hicon;

		if ( parent )
			RemovePanel(parent);
	}
}

void TPanelWindowView::GoMenu( TAwarePopupMenu *menu, BPoint point, BRect *rect )
{
	menu->SetTargetMessenger( BMessenger(this) );

	fOpenMenu = menu;

	if ( rect )
		menu->Go( point, true, true, *rect, true );
	else
		menu->Go( point, true, true, true );
}

void TPanelWindowView::DraggingItem( TPanelIcon *item )
{
	if ( item )
	{
		BMessage message( 'IDRG' );
		message.AddPointer( "source", item );
		DragMessage( &message, item->fFrame, this );
	}
}

TAppPanelIcon *TPanelWindowView::ItemWith( team_id tid )
{
	BAutolock( &fPanels.lock );

	TAppPanelIcon *icon = 0;

	int32 count = fPanels.CountItems();
	for ( int32 i=0; i<count; i++ )
	{
		TApplicationPanel *panel = dynamic_cast<TApplicationPanel*>(fPanels.ItemAt(i));
		if ( panel )
		{
			icon = panel->ItemWith(tid);
			if ( icon )
				break;
		}
	}

	return icon;
}

TAppPanelIcon *TPanelWindowView::ItemWith( const char *sig )
{
	BAutolock( &fPanels.lock );

	TAppPanelIcon *icon = 0;

	int32 count = fPanels.CountItems();
	for ( int32 i=0; i<count; i++ )
	{
		TApplicationPanel *panel = dynamic_cast<TApplicationPanel*>(fPanels.ItemAt(i));
		if ( panel )
		{
			icon = panel->ItemWith(sig);
			if ( icon )
				break;
		}
	}

	DebugCall( 9, "TPanelWindowView::ItemWith( %s ) returned %p [parent: %p]", sig, icon, icon ? icon->fParent : 0);

	return icon;
}

#include <stdarg.h>

void TPanelWindowView::DebugCall( int32 level, const char *format, ... )
{
	if ( level < fDebugLevel )
	{
		va_list vl;
		va_start( vl, format );
		char out[256];
		vsprintf(out, format, vl );
		va_end(vl);

		printf("DebugCall[%i]( %s )\n", (int)level, out );

		FILE *fl = fopen("/boot/home/dockbert_debug_log", "a" );
		if ( fl )
		{
			fprintf( fl, "%s\n", out );
			fflush(fl);
			fclose(fl);
		}
	}
}

int32 TPanelWindowView::_rearrange_tabs_thread_entry( void *arg )
{
	((TPanelWindowView*)arg)->ReArrangeTabsThread();
	return 0;
}

void TPanelWindowView::ReArrangeTabsThread()
{
	BPoint pw;
	uint32 mods;

	Window()->Lock();
	GetMouse(&pw, &mods);
	Window()->Unlock();

	TInnerPanel *dragged = PanelAt(pw);
	int draggedindex = IndexOf(dragged);
	uint32 dragged_but = mods;

	while (1)
	{
		Window()->Lock();
		GetMouse(&pw, &mods);
		Window()->Unlock();
		if ( mods != dragged_but )
			break;
		pw.y = dragged->Frame().top +1;

		TInnerPanel *panel = PanelAt(pw);
		int index = -1;
		if ( panel )
			index = IndexOf(panel);
		if ( index < 0 || (index+2) == draggedindex || (index-2) == draggedindex )
		{
			int targetindex = -1;
			if ( index == -1 )
			{
				if ( pw.x < dragged->Frame().left )
					targetindex = 0;
				else
					targetindex = CountPanels()-1;
			}
			else if ( index < draggedindex )
				targetindex = index+1;
			else if ( index > draggedindex )
				targetindex = index-1;
			if ( targetindex != draggedindex )
			{
				MoveTabTo( dragged, targetindex );
				draggedindex = targetindex;
			}
		}
		snooze(50000);
	}
}

void TPanelWindowView::DoDebugDump()
{
	BAutolock( &fPanels.lock );

	int count = fShortcutPanelList.CountItems();

	DebugCall( 1, "**** Doing a Debug Dump ****" );

	DebugCall( 1, "Build Info: %s %s [build %s] (%s)", BUILDVERSION, BUILDSTATUS, BUILDNR, BUILDDATE );

	DebugCall( 1, "Number of Tabs: %i", count );
	DebugCall( 1, "\tAutoHide: %s", ((TPanelWindow*)Window())->IsAutoHiding() ? "true":"false" );
	DebugCall( 1, "\tAlwaysOnTop: %s", fAlwaysOnTop?"true":"false" );

	for ( int i=0; i<count; i++ )
	{
		DebugCall( 1, "\tDebugDump for tab %i", i+1 );
		fShortcutPanelList.ItemAt(i)->DoDebugDump();
	}

	DebugCall( 1, "\tDebugDump for fRunningAppPanel" );
	fRunningAppPanel->DoDebugDump();

	DebugCall( 1, "**** EndOf Debug Dump ****" );
}

void TPanelWindowView::DoCheckup()
{
	BAutolock( &fPanels.lock );

	int count = fShortcutPanelList.CountItems();

	DebugCall( 1, "**** Starting Checkup ****" );

	for ( int i=0; i<count; i++ )
	{
		DebugCall( 1, "\tDoCheckup for tab %i", i+1 );
		fShortcutPanelList.ItemAt(i)->DoCheckup();
	}

	DebugCall( 1, "\tDoCheckup for fRunningAppPanel" );
	fRunningAppPanel->DoCheckup();

	DebugCall( 1, "**** EndOf Checkup ****" );
}
