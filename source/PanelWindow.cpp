#include <Screen.h>
#include <Roster.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Bitmap.h>
#include <Region.h>
#include <Messenger.h>
#include <MessageRunner.h>
#include <File.h>
#include <Font.h>
#include <Alert.h>

#include <ClassInfo.h>

#include <locale/Locale.h>

#include "PanelWindow.h"
#include "PanelWindowView.h"
#include "InnerPanel.h"
#include "BarApp.h"

#include "ResourceSet.h"
#include "icons.h"

#include "tracker_private.h"

#include <stdio.h>
#include <strings.h>
#include <malloc.h>

#include <Debug.h>

#include <Catalog.h>
#include <Locale.h>

#define B_TRANSLATION_CONTEXT "panel-window"

#define _MNITEM_W 4
#define _MNITEM_H 4
#define TASK_BAR_MIME_SIG "application/x-vnd.Be-TSKB "

TPanelWindow::TPanelWindow()
	: BWindow( BRect( 0, 0, 24, 24 ), "Dock", B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL, B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_WILL_ACCEPT_FIRST_CLICK | B_AVOID_FOCUS, B_ALL_WORKSPACES ),
	 fPreferencesWindow(NULL)
{
	fAutoHide = false;
	fHideEffectDelay = kWindowHidderAnimationStaleSpeed;

	fIsVisible = true;

	fHidderTimer = 0;

	fAlmostHidding = false;
	fHidderAnimationCurrentFrame = kWindowHidderNumberFrames;
	SetShowHideLimit( 0.f );

	BMessage msg;
	BFile settings( "/boot/home/config/settings/dock_settings", B_READ_ONLY );

	fTeamListView = 0;

	if ( msg.Unflatten( &settings ) == B_OK && msg.FindInt32("SettingsVersion") > 0 )
	{
		BMessage dock_settings, *dockb = 0;
		if ( msg.FindMessage( "dock", &dock_settings ) == B_OK )
			dockb = &dock_settings;

		fTeamListView = (TPanelWindowView*)TPanelWindowView::Instantiate( dockb );
	}
	else
	{
		// first time using
		(new BAlert( B_TRANSLATE("First Time"), B_TRANSLATE("This is your first time running Dockbert so we'll give you a small introduction to it.\nBy now there should be a small window in the bottom of your screen with some icons, that's Dockbert.\nWe'll now give you a small info about each icon, from left to right:\n- The trash, drag files to it to move them to trash, left clicking it opens the trash window, right clicking shows you a handy menu\n-The workspaces, left click to forward a workspace, right click to backward a workspace, the middle button gives you a list of the workspaces\n-The clock, right click it for options\n-The Tracker, right click the tracker for the tracker menu, with some useful options\n-The Be menu, right click it to see the Be Menu\n-the other two icons are shortcuts, read the readme for aditional information"), B_TRANSLATE("Ok thanks!")))->Go(0);
	}

	if ( !fTeamListView )
		fTeamListView = new TPanelWindowView();

	AddChild( fTeamListView );

	// force resize
	fTeamListView->ChangedSize(0);

	ResizeTo( fTeamListView->Bounds().Width(), fTeamListView->Bounds().Height() );

	if ( fTeamListView->WhereAmI() == kLocationTop || fTeamListView->WhereAmI() == kLocationBottom )
	{
		if ( Bounds().Height() < 40 )
			ResizeBy( 0, 40 - Bounds().Height() );
	}
	else
	{
		if ( Bounds().Width() < 40 )
			ResizeBy( 0, 40 - Bounds().Width() );
	}

/*	BMessenger self(this);
	BList teamList;
	TBarApp::Subscribe(self, &teamList);

	int32 count = teamList.CountItems();
	for (int i = 0; i < count; i++) {
		BarTeamInfo *barInfo = (BarTeamInfo *)teamList.ItemAt(i);

		if ( ( barInfo->flags & B_BACKGROUND_APP ) == 0 && strcasecmp( barInfo->sig, TASK_BAR_MIME_SIG ) != 0 )
			fTeamListView->AddTeam( barInfo->teams, barInfo->icon, barInfo->name, barInfo->sig );
	}*/

	BList teamList;
	be_roster->GetAppList( &teamList );

	int32 count = teamList.CountItems();
	for ( int i=0; i < count; i++ )
	{
		app_info appinfo;
		if ( be_roster->GetRunningAppInfo( (team_id)teamList.ItemAt(i), &appinfo ) == B_OK
			 && ( appinfo.flags & B_BACKGROUND_APP ) == 0
			 && strcasecmp( appinfo.signature, TASK_BAR_MIME_SIG ) )
			fTeamListView->AddTeam( appinfo.team, appinfo.signature, appinfo.ref );
	}
	
	CenterCorrectly();

	Show();

	rename_thread( Thread(), THREAD_NAME );
}

TPanelWindow::~TPanelWindow()
{
	if ( fHidderTimer )
		delete fHidderTimer;
	SaveSettings();
}

void TPanelWindow::MessageReceived( BMessage *message )
{
	switch ( message->what )
	{
	case B_ENTERED_VIEW:
		ShowHide(true);
		break;
	case B_EXITED_VIEW:
		ShowHide(false);
		break;
	case B_SOME_APP_LAUNCHED:
		{
			BList *teams = NULL;
			message->FindPointer("teams", (void **)&teams);

			BBitmap *icon = NULL;
			message->FindPointer("icon", (void **)&icon);

			const char *sig;
			if (message->FindString("sig", &sig) == B_OK
				&&strcasecmp(sig, TASK_BAR_MIME_SIG) == 0) {
				delete teams;
				delete icon;
				break;
			}

			uint32 flags;
			if (message->FindInt32("flags", ((int32*) &flags)) == B_OK
				&& (flags & B_BACKGROUND_APP) != 0) {
				delete teams;
				delete icon;
				break;
			}

			const char *name = NULL;
			message->FindString("name", &name);

			fTeamListView->AddTeam(teams, icon, strdup(name), strdup(sig));
			break;
		}
	case B_SOME_APP_QUIT:
		{	
			team_id team = -1;
			message->FindInt32("team", &team);
			fTeamListView->RemoveTeam(team);

			break;
		}
	case kDockbertPreferences:
            ShowPreferencesWindow();
            break;
	case msg_AddTeam:
		{
			const char *sig = message->FindString("sig");
			team_id team = message->FindInt32("team");			

			BList *list = new BList;
			list->AddItem( (void*) team );

			fTeamListView->AddTeam(list, 0, 0, strdup(sig));

			break;
		}
	case msg_RemoveTeam:
		{
			team_id team = message->FindInt32("team");
			fTeamListView->RemoveTeam(team);

			break;
		}
	case kPanelWindowViewTimer:
		{
			fTeamListView->MessageReceived( message );
			break;
		}
	case kPanelWindowHidderTimer:
		{
			if ( !DoShowHideFrame() )
			{
				delete fHidderTimer;
				fHidderTimer = 0;
			}
			break;
		}
	case kPrepareWindowHidder:
		{
			fTeamListView->DebugCall( 15, "TPanelWindow::MessageReceived(): kPrepareWindowHidder" );
			bigtime_t time = fHiddingPipe.Pop();
			bigtime_t now = system_time();
			if ( (time <= now) && fAlmostHidding )
			{
				fIsVisible = false;
				fAlmostHidding = false;
				if ( fHidderTimer )
					delete fHidderTimer;
				fHidderTimer = new MESSAGERUNNER(  BMessenger(this), 
									new BMessage(kPanelWindowHidderTimer),
									kWindowHidderAnimationSpeed );
			}
			else
				fTeamListView->DebugCall( 15, "\tkPrepareWindowHidder was invalid" );
			break;
		}
	case 'sett':
	{
		fTeamListView->MessageReceived( message );
		break;
	}
	case 'nmcl':
	{
		fTeamListView->MessageReceived(message);
		break;
	}
	case 'flsh':
	{
		fTeamListView->MessageReceived(message);
		break;
	}
	case kInnelPanelTimerMessage:
	{
		void *ptr;
		if ( message->FindPointer( "panel_target", &ptr ) == B_OK )
		{
			TInnerPanel *panel = static_cast<TInnerPanel*> ( ptr );
			if ( panel )
			{
				panel->TimerFrame();
			}
		}
		break;
	}
	default:
		BWindow::MessageReceived(message);
		break;
	}
}

void TPanelWindow::FrameResized(float /* width */, float /* height */ )
{
	CenterCorrectly();
}

void TPanelWindow::WorkspaceActivated(int32 /* workspace */, bool flag)
{
	if (flag) CenterCorrectly();
	fTeamListView->WorkspaceChanged();
}

extern property_info _dock_property_list[];

// do DockView's detection here so we dont have to specify another specifier
BHandler *TPanelWindow::ResolveSpecifier( BMessage *message, int32 index, BMessage *specifier, int32 what, const char *property )
{
	BPropertyInfo prop_info( _dock_property_list );
	if ( prop_info.FindMatch( message, index, specifier, what, property ) >= 0 )
		return fTeamListView;
	if ( !strcasecmp( property, "tab" ) )
		return fTeamListView;

	return BWindow::ResolveSpecifier( message, index, specifier, what, property );
}

status_t TPanelWindow::GetSupportedSuites( BMessage *message )
{
	fTeamListView->GetDockbertSuites( message );
	return BWindow::GetSupportedSuites( message );
}

void TPanelWindow::ScreenChanged( BRect /* frame */, color_space /* mode */ )
{
	CenterCorrectly();
}

void TPanelWindow::CenterCorrectly()
{
	int loc = fTeamListView->WhereAmI();

	BRect screenFrame( BScreen(B_MAIN_SCREEN_ID).Frame() );
	BPoint mpt;

	if ( loc == kLocationTop || loc == kLocationBottom )
	{
		mpt.x = screenFrame.Width() /2 - Frame().Width() /2;
		if ( loc == kLocationTop )
			mpt.y = 0;
		else
			mpt.y = screenFrame.Height() - Frame().Height();
	}
	else if ( loc == kLocationLeft || loc == kLocationRight )
	{
		mpt.y = screenFrame.Height() / 2 - Frame().Height() / 2;
		if ( loc == kLocationLeft )
			mpt.x = 0;
		else
			mpt.x = screenFrame.Width() - Frame().Width();
	}

	if ( fAutoHide )
	{
		if ( loc == kLocationTop )
		{
			mpt.y = Frame().Height() * kWindowHidderFrames[fHidderAnimationCurrentFrame] - Frame().Height();
			if ( mpt.y < 1 )
				mpt.y = 1;
		}
		else if ( loc == kLocationBottom )
		{
			mpt.y = Frame().Height() * kWindowHidderFrames[kWindowHidderNumberFrames-fHidderAnimationCurrentFrame] - Frame().Height() + screenFrame.Height();
			if ( mpt.y > (screenFrame.Height()-1) )
				mpt.y = screenFrame.Height()-1;
		}
		else if ( loc == kLocationLeft )
		{
			mpt.x = Frame().Width() * kWindowHidderFrames[fHidderAnimationCurrentFrame] - Frame().Width();
			if ( mpt.x < 1 )
				mpt.x = 1;
		}
		else if ( loc == kLocationRight )
		{
			mpt.x = Frame().Width() * kWindowHidderFrames[kWindowHidderNumberFrames-fHidderAnimationCurrentFrame] - Frame().Width() + screenFrame.Width();
			if ( mpt.x > (screenFrame.Width()-1) )
				mpt.x = screenFrame.Width()-1;
		}

	}		

	MoveTo( mpt );
}

void TPanelWindow::ShowHide(bool flag)
{
	fTeamListView->DebugCall( 15, "TPanelWindow::ShowHide( fAutoHide:%s, flag:%s, fIsVisible: %s, fAlmostHidding: %s )", fAutoHide?"true":"false", flag?"true":"false", fIsVisible?"true":"false", fAlmostHidding?"true":"false" );

	if (fAutoHide)
	{
		if ( !flag )
		{
			if ( fIsVisible )
			{
				fAlmostHidding = true;
				new MESSAGERUNNER( BMessenger(this), new BMessage(kPrepareWindowHidder), fHideEffectDelay, 1 );
				bigtime_t time = system_time()+fHideEffectDelay;
				fHiddingPipe.Push(time);
			}
		}
		else
		{
			fHiddingPipe.Pop();
			fAlmostHidding = false;
			fIsVisible = true;
			if ( fHidderTimer )
				delete fHidderTimer;
			fHidderTimer = new MESSAGERUNNER(  BMessenger(this), 
								new BMessage(kPanelWindowHidderTimer),
								kWindowHidderAnimationSpeed );
		}
	}
}

void TPanelWindow::SaveSettings()
{
	BMessage msg;
	BMessage dock_settings;

	Lock();

	if ( fTeamListView->Archive( &dock_settings ) == B_OK )
		msg.AddMessage( "dock", &dock_settings );

	BFile settings( "/boot/home/config/settings/dock_settings", B_READ_WRITE | B_CREATE_FILE);

	msg.AddInt32( "SettingsVersion", 1 );

	msg.Flatten( &settings );
	printf("Save settings\n");

	Unlock();
}

bool TPanelWindow::DoShowHideFrame()
{
	int loc = fTeamListView->WhereAmI();

	BRect screenFrame( BScreen(B_MAIN_SCREEN_ID).Frame() );
	BPoint mpt;

	if ( fIsVisible )
		fHidderAnimationCurrentFrame++;
	else
		fHidderAnimationCurrentFrame--;

	if ( fHidderAnimationCurrentFrame < fLastShowHideFrame || fHidderAnimationCurrentFrame > kWindowHidderNumberFrames )
	{
		if ( fHidderAnimationCurrentFrame < fLastShowHideFrame )
			fHidderAnimationCurrentFrame = fLastShowHideFrame;
		if ( fHidderAnimationCurrentFrame > kWindowHidderNumberFrames )
			fHidderAnimationCurrentFrame = kWindowHidderNumberFrames;
		return false;
	}

	if ( loc == kLocationTop )
	{
		mpt.x = Frame().left;
		mpt.y = Frame().Height() * kWindowHidderFrames[fHidderAnimationCurrentFrame] - Frame().Height();
		if ( mpt.y < 1 )
			mpt.y = 1;
	}
	else if ( loc == kLocationBottom )
	{
		mpt.x = Frame().left;
		mpt.y = Frame().Height() * kWindowHidderFrames[kWindowHidderNumberFrames-fHidderAnimationCurrentFrame] - Frame().Height() + screenFrame.Height();
		if ( mpt.y > (screenFrame.Height()-1) )
			mpt.y = screenFrame.Height()-1;
	}
	else if ( loc == kLocationLeft )
	{
		mpt.x = Frame().Width() * kWindowHidderFrames[fHidderAnimationCurrentFrame] - Frame().Width();
		mpt.y = Frame().top;
		if ( mpt.x < 1 )
			mpt.x = 1;
	}
	else if ( loc == kLocationRight )
	{
		mpt.x = Frame().Width() * kWindowHidderFrames[kWindowHidderNumberFrames-fHidderAnimationCurrentFrame] - Frame().Width() + screenFrame.Width();
		mpt.y = Frame().top;
		if ( mpt.x > (screenFrame.Width()-1) )
			mpt.x = screenFrame.Width()-1;
	}

	MoveTo( mpt );

	return true;
}

void TPanelWindow::ForceHide()
{
	ShowHide(false);
}

void TPanelWindow::DockChangedPlaces( int32 prev )
{
	int32 loc = fTeamListView->WhereAmI();

	if ( (loc == kLocationTop || loc == kLocationBottom) && ( prev == kLocationLeft || prev == kLocationRight ) )
	{
		ResizeTo( Bounds().Height(), Bounds().Width() );
	} 
	if ( (loc == kLocationLeft || loc == kLocationRight) && ( prev == kLocationTop || prev == kLocationBottom ) )
	{
		ResizeTo( Bounds().Height(), Bounds().Width() );
	} 

	CenterCorrectly();
}

void TPanelWindow::SetAutoHide( bool b )
{
	if ( fAutoHide != b )
	{
		if ( b )
			fAutoHide = true;
		ShowHide(!b);
		if ( !b )
			fAutoHide = false;
	}
}

void TPanelWindow::SetHideEffectDelay( int newvalue )
{
	fHideEffectDelay = newvalue;
}

void TPanelWindow::SetShowHideLimit( float f )
{
	fLastShowHideFrame = (int) ((float)kWindowHidderNumberFrames * f);
}

TMessageRunner::TMessageRunner( BMessenger messenger, BMessage *message, bigtime_t interval, int count )
	: fMessenger(messenger), fMessage(message), fInterval(interval), fCount(count)
{
	fThreadID = spawn_thread( _thread_instance, "TMessageRunner", B_NORMAL_PRIORITY, this );
	resume_thread( fThreadID );
}

TMessageRunner::~TMessageRunner()
{
	lock.Lock();
	kill_thread(fThreadID);
	delete fMessage;
}

status_t TMessageRunner::InitCheck()
{
	return fThreadID >= 0 ? B_OK : B_ERROR;
}

void TMessageRunner::SetInterval( bigtime_t delta )
{
	fInterval = delta;
}

void TMessageRunner::Loop()
{
	while ( fCount == -1 || fCount > 0 )
	{
		snooze( fInterval );
		lock.Lock();
		fMessenger.SendMessage( fMessage );
		lock.Unlock();
		if ( fCount != -1 )
			fCount--;
	}
}

int32 TMessageRunner::_thread_instance( void *p )
{
	((TMessageRunner*)p)->Loop();
	return 0;
}

void
TPanelWindow::ShowPreferencesWindow()
{
       if (fPreferencesWindow)
               fPreferencesWindow->Activate();
       else {
               fPreferencesWindow = new PreferencesWindow(BRect(0, 0, 320, 240));
               fPreferencesWindow->Show();
       }
}
