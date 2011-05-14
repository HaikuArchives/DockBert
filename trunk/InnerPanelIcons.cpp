#include <Bitmap.h>
#include <Directory.h>
#include <Message.h>
#include <List.h>
#include <NodeInfo.h>
#include <String.h>
#include <Roster.h>
#include <Alert.h>
#include <NodeMonitor.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <MessageRunner.h>
#include <Region.h>
#include <malloc.h>
#include <locale/Locale.h>

#include "InnerPanelIcons.h"
#include "PanelWindow.h"
#include "TrackerMenus.h"
#include "InnerPanel.h"
#include "PanelWindowView.h"

#include "OffscreenView.h"

#include "WindowMenuItem.h" // for nasty stuff

#include <ObjectList.h>
#include "FSUtils.h"
#include "Commands.h"

#include "tracker_private.h"

#include "ResourceSet.h"
#include "othericons.h"

#include <Catalog.h>
#include <Locale.h>

#define B_TRANSLATE_CONTEXT "inner-panel-icons"

using namespace BPrivate;

bool
WindowShouldBeListed(uint32 behavior);

BBitmap* GetTrackerIcon(BEntry *e, icon_size which)
{
    entry_ref ref;    
    BBitmap *bmp = NULL;

    if(which==kDefaultSmallIconSize)
        bmp = new BBitmap(BRect(0,0,15,15), B_RGBA32);
    else
        bmp = new BBitmap(BRect(0,0,47,47), B_RGBA32);

    if(e->GetRef(&ref) == B_OK) {
        if(BNodeInfo::GetTrackerIcon(&ref, bmp, which) == B_OK) {
            return bmp;
        }
    }
    return NULL;
} 


TPanelIcon::~TPanelIcon()
{
	if ( Looper() )
		Looper()->RemoveHandler(this);
}

void TPanelIcon::DoDebugDump()
{
	fParent->Parent()->DebugCall( 1, "\t\tclass name: %s", class_name(this) );
}

bool TPanelIcon::Touches( BPoint p ) const
{
	BPoint point(p);
	BWindow *w = fParent->Parent()->Window();
	w->Lock();
	w->ConvertFromScreen( &point );
	BRegion region;
	fParent->Parent()->GetClippingRegion(&region);
	if ( region.Contains(point) && fFrame.Contains(point) && !fParent->Parent()->IsAMenuOpen() )
	{
		w->Unlock();
		return true;
	}
	w->Unlock();
	return false;
}

void TPanelIcon::DoMenu()
{
	TAwarePopupMenu *menu = Menu();

	if ( !menu )
		return;

	BPoint topleft = ContentLocation();

	menu->SetTargetForItems( this );
	menu->SetAsyncAutoDestruct(true);

	float w, h;
	menu->GetPreferredSize( &w, &h );

	BPoint where( topleft.x, 1 );
	fParent->Parent()->ConvertToScreen( &where );
	where.y -= h;

	if ( where.y < 0 )
	{
		where.x = topleft.x;
		where.y = 0;
		fParent->Parent()->ConvertToScreen(&where);
	}

	BRect icon_rect = BRect( topleft, BPoint( topleft.x + Frame().Width(), topleft.y + Frame().Height() ) );

	fParent->Parent()->ConvertToScreen(&icon_rect);

	fParent->Parent()->GoMenu( menu, where, &icon_rect );
}

TZoomableIcon::TZoomableIcon()
	: TPanelIcon(), fSmallIcon(0), fBigIcon(0),
					fDimmedSmallIcon(0), fDimmedBigIcon(0),
					fDeleteBitmaps(true), fDisabled(false)
{
}

TZoomableIcon::TZoomableIcon( BBitmap *small, BBitmap *big )
	: TPanelIcon(), fSmallIcon(small), fBigIcon(big),
					fDimmedSmallIcon(0), fDimmedBigIcon(0),
					fDeleteBitmaps(true), fDisabled(false)
{
}

TZoomableIcon::TZoomableIcon( BMessage *archive )
	: TPanelIcon(archive), fSmallIcon(0), fBigIcon(0),
					fDimmedSmallIcon(0), fDimmedBigIcon(0),
					fDeleteBitmaps(true), fDisabled(false)
{
}

TZoomableIcon::~TZoomableIcon()
{
	if ( fDeleteBitmaps )
	{
		delete fSmallIcon;
		delete fBigIcon;
	}

	delete fDimmedSmallIcon;
	delete fDimmedBigIcon;
}

void TZoomableIcon::SetDestroyBitmaps( bool b )
{
	fDeleteBitmaps = b;
}

void TZoomableIcon::GetPreferredSize( float *width, float *height )
{
	*width = kDefaultBigIconSize;
	*height = kDefaultBigIconSize;
}

float _zoom_factors[16+1] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
//float _zoom_factors[16+1] = { 0, 0.19372, 0.42497, 0.70102, 1.0305, 1.4239, 1.8935, 2.454, 3.1231, 3.9218, 4.8753, 6.0135, 7.3721, 8.994, 10.93, 13.241, 16 };

void TZoomableIcon::Draw()
{
	PrepareDrawing();
	BPoint where = ContentLocation();

//	float animation_factor = fZoomStep/2;
	float animation_factor = _zoom_factors[fZoomStep]/2;

	int _e = ((kDefaultBigIconSize-kDefaultSmallIconSize)/2);
	animation_factor *= ((float)_e) / 8.0f;

	float _dl = _e - animation_factor;
	where.x += _dl;
	where.y += _dl;

	float factor = kDefaultSmallIconSize + animation_factor*2;

	BRect size( where, BPoint( where.x + factor -1, where.y + factor -1) );

	BView *canvas = fParent->Parent();

	BBitmap *which = Bitmap();

	if ( which )
		canvas->DrawBitmap( which, size );
}

void TZoomableIcon::PrepareDrawing()
{
	fParent->Parent()->SetDrawingMode( B_OP_ALPHA );
	fParent->Parent()->SetBlendingMode( B_PIXEL_ALPHA, B_ALPHA_OVERLAY );
}

BBitmap *TZoomableIcon::Bitmap()
{
	if (fZoomStep > 0) {
		if (!fDisabled)
			return fBigIcon;
		if (!fDimmedBigIcon) {
			fDimmedBigIcon = DimmBitmap(fBigIcon);
		}
		return fDimmedBigIcon;
	} else {
		if (!fDisabled)
			return fSmallIcon;
		if (!fDimmedSmallIcon) {
			fDimmedSmallIcon = DimmBitmap(fSmallIcon);
		}
		return fDimmedSmallIcon;
	}
}

BBitmap *TZoomableIcon::DimmBitmap(BBitmap *orig)
{
	if (!orig)
		return 0;

	BBitmap *result = new BBitmap(orig->Bounds(), B_RGBA32);
	memcpy(result->Bits(), orig->Bits(), orig->BitsLength());
	uchar *bits = (uchar *)result->Bits();
	for (int32 index = 0; index < result->BitsLength(); index+=4) {
		bits[index+3] = bits[index+3] * 30 / 100;
	}
	return result;
}

TTrackerIcon::TTrackerIcon( entry_ref ref )
	: TZoomableIcon(), fRef(ref)
{
	ReloadIcons();

	BEntry entry( &fRef );
	entry.GetNodeRef( &fNode );
	entry.GetPath( &fPath );
}

TTrackerIcon::TTrackerIcon( BMessage *msg )
	: TZoomableIcon( msg )
{
	if ( msg )
	{
		msg->FindRef( "ref", &fRef );
		BEntry entry( &fRef );
		BString string;
		if ( msg->FindString( "path", &string ) == B_OK )
			fPath = string.String();
		else
			entry.GetPath(&fPath);
		if ( !entry.Exists() )
		{
			BEntry entry2( fPath.Path() );
			entry2.GetRef( &fRef );
		}
	}
	ReloadIcons();

	BEntry entry( &fRef );
	entry.GetNodeRef( &fNode );
}

TTrackerIcon::~TTrackerIcon()
{
	watch_node( &fNode, B_STOP_WATCHING, this );
}

status_t TTrackerIcon::InitCheck()
{
	BEntry entry( &fRef, true );
	return entry.Exists() ? B_OK : B_ERROR;
}

void TTrackerIcon::AttachedToPanel()
{
	TZoomableIcon::AttachedToPanel();
	watch_node( &fNode, B_WATCH_ATTR|B_WATCH_NAME, this );
}

void TTrackerIcon::DoDebugDump()
{
	TPanelIcon::DoDebugDump();
	fParent->Parent()->DebugCall( 1, "\t\tentry: %s", fPath.Path() );
}

void TTrackerIcon::DoCheckup()
{
	BEntry entry( &fRef );
	if ( !entry.Exists() )
		fParent->Parent()->DebugCall( 1, "my entry doesn't really exist!" );
}

void TTrackerIcon::MessageReceived( BMessage *message )
{
	switch ( message->what )
	{
	case B_NODE_MONITOR:
		{
			int32 opcode;
			if ( message->FindInt32( "opcode", &opcode ) == B_OK )
			{
				if ( opcode == B_ENTRY_MOVED )
				{
					node_ref nref;
					dev_t dev;
					message->FindInt32( "device", &dev );
					nref.device = dev;
					message->FindInt64( "to directory", &nref.node );
					BDirectory dir( &nref );
					BPath path( &dir, NULL, false );
					BVolume vol( dev );
					BPath trashpath;
					find_directory( B_TRASH_DIRECTORY, &trashpath, false, &vol );
					if ( trashpath == path )
						opcode = B_ENTRY_REMOVED;
				}

				if ( opcode == B_ATTR_CHANGED )
				{
					node_ref nref;
					message->FindInt32( "device", &nref.device );
					message->FindInt64( "node", &nref.node );

					if ( nref == fNode )
					{
						ReloadIcons();
						fParent->Invalidate(this);
					}
				}
				else if ( opcode == B_ENTRY_REMOVED )
				{
					EntryRemoved();
				}
				else
				{
					entry_ref ref;
					const char *name;

					message->FindInt32( "device", &ref.device );
					message->FindInt64( "directory", &ref.directory );
					message->FindString( "name", &name );
					ref.set_name( name );

					SetRefTo( ref );
				}
			}
			break;
		}
	default:
		TZoomableIcon::MessageReceived(message);
	}
}

TAwarePopupMenu *TTrackerIcon::Menu()
{
	BEntry entry( &fRef, true );
	if ( entry.IsDirectory() )
		return TDockMenus::TrackerMenu( fRef );
	return 0;
}

bool TTrackerIcon::AcceptsDrop( bool /*force*/ ) const
{
	BEntry entry( &fRef, true );
	return entry.IsDirectory();
}

status_t TTrackerIcon::DroppedSomething( const BMessage *message )
{
	if ( message->what == B_SIMPLE_DATA )
	{
		BEntry entry( &fRef, true );
		if ( entry.IsDirectory() )
		{
			BObjectList<entry_ref> *list = new BObjectList<entry_ref>();
			entry_ref ref;
			int i;
			for ( i=0; ; i++ )
			{
				if ( message->FindRef( "refs", i, &ref ) != B_OK )
					break;
				list->AddItem( new entry_ref(ref) );
			}

			FSMoveToFolder( list, new BEntry( entry ), kMoveSelectionTo, 0 );
		}
		else
			return B_ERROR;
	}
	else
		return TZoomableIcon::DroppedSomething( message );

	return B_OK;
}

void TTrackerIcon::MouseDown( BPoint where, uint32 modifiers, uint32 buttons )
{
	if ( buttons & B_PRIMARY_MOUSE_BUTTON )
	{
		Launch();
	}
	else if ( (buttons & B_SECONDARY_MOUSE_BUTTON) && (modifiers & B_CONTROL_KEY) )
	{
		Quit();
	}
	else
		TZoomableIcon::MouseDown( where, modifiers, buttons );
}

void TTrackerIcon::ReloadIcons()
{
	delete fSmallIcon;
	BEntry ent(&fRef, true);
	
	if( ent.InitCheck() == B_OK )
	{
		fSmallIcon = new BBitmap( GetTrackerIcon(&ent, B_MINI_ICON));
		
		delete fBigIcon;
		fBigIcon = new BBitmap( GetTrackerIcon(&ent, (icon_size)3));
	}
}

void TTrackerIcon::SetRefTo( entry_ref ref )
{
	watch_node( &fNode, B_STOP_WATCHING, this );

	fRef = ref;

	ReloadIcons();

	BEntry entry( &fRef );
	entry.GetNodeRef( &fNode );
	entry.GetPath(&fPath);

	watch_node( &fNode, B_WATCH_ATTR|B_WATCH_NAME, this );
}

void TTrackerIcon::Launch()
{
	BMessage message(B_REFS_RECEIVED);
	message.AddRef( "refs", &fRef );
	if ( BMessenger( kTrackerSignature ).SendMessage( &message ) != B_OK )
	{
		be_roster->Launch( kTrackerSignature );
		snooze( 500000 );
		BMessage message(B_REFS_RECEIVED);
		message.AddRef( "refs", &fRef );
		BMessenger( kTrackerSignature ).SendMessage( &message );
	}
}

void TTrackerIcon::Quit()
{
}

void TTrackerIcon::EntryRemoved()
{
	entry_ref ref;
	BEntry entry( fPath.Path() );
	if ( entry.GetRef( &ref ) == B_OK )
		SetRefTo(ref);
	else
		fParent->Parent()->RemoveShortcut( this );
}

const char *TTrackerIcon::BubbleText() const
{
	const char *text = fPath.Leaf();
	if ( strlen(text) == 0 )
	{
		if ( !strcmp( fPath.Path(), "/" ) )
			return B_TRANSLATE("Disks");
		else
			return 0;
	}
	return text;
}

TAppPanelIcon::TAppPanelIcon( entry_ref &ref, BList *team, const char *sig )
	: TTrackerIcon(ref), fTeam(team), fSig(sig), fActive(false), fFlashed(false)
{
	fActiveBitmapAlpha = 0;
	fAlphaAnimationFloatingMessages = 0;

	fPrevKernTime = fPrevUserTime = 0;
	fPreviousTiming = system_time();

	fWindowCyclerIndex = 0;
}

TAppPanelIcon::TAppPanelIcon( BMessage *msg )
	: TTrackerIcon(msg), fTeam(0), fSig(0), fActive(false), fFlashed(false)
{
	fActiveBitmapAlpha = 0;
	fAlphaAnimationFloatingMessages = 0;

	fTeam = new BList();

	BString signature;
	msg->FindString( "signature", &signature );
	fSig = strdup( signature.String() );

	BEntry entry( &fRef, true );
	if ( !entry.Exists() )
	{
		entry.SetTo( fPath.Path() );
		if ( !entry.Exists() )
			be_roster->FindApp( fSig, &fRef );
		else
			entry.GetRef(&fRef);

		SetRefTo( fRef );
		ReloadIcons();
	}

	fPrevKernTime = fPrevUserTime = 0;
	fPreviousTiming = system_time();

	fWindowCyclerIndex = 0;
}

TAppPanelIcon::~TAppPanelIcon()
{
	delete fTeam;
	if ( fSig )
		free((void*)fSig);
}

void TAppPanelIcon::DoDebugDump()
{
	TTrackerIcon::DoDebugDump();
	fParent->Parent()->DebugCall( 1, "\t\tsignature: %s, %i teams", fSig, (int)fTeam->CountItems() );
}

void TAppPanelIcon::DoCheckup()
{
	TTrackerIcon::DoCheckup();

	BList forRemoval;

	int i, count = fTeam->CountItems();
	for ( i=0; i<count; i++ )
	{
		team_id tid = (team_id)fTeam->ItemAt(i);
		app_info appinfo;
		if ( be_roster->GetRunningAppInfo( tid, &appinfo ) != B_OK
			 || strcasecmp( appinfo.signature, fSig ) )
		{
			 fParent->Parent()->DebugCall( 1, "I have a team (%i) that doesnt belong to me (%s)", (int)tid, fSig );
			 forRemoval.AddItem( (void*) tid );
		}
		if ( !BMessenger( 0, tid ).IsValid() )
			 fParent->Parent()->DebugCall( 1, "the messenger for team (%i) isnt valid", (int)tid );
	}

	count = forRemoval.CountItems();
	for ( i=0; i<count; i++ )
	{
		fParent->Parent()->RemoveTeam( (team_id) forRemoval.ItemAt(i) );
	}

	BList reallist;
	be_roster->GetAppList( fSig, &reallist );
	count = reallist.CountItems();
	for ( i=0; i<count; i++ )
	{
		if ( !fTeam->HasItem( reallist.ItemAt(i) ) )
			 fParent->Parent()->DebugCall( 1, "There's a running team for this signature that i dont have on my list!!! [team %i]", (int)reallist.ItemAt(i) );
	}
}

TAwarePopupMenu *TAppPanelIcon::Menu()
{
	if ( fTeam->CountItems() > 0 )
		return TDockMenus::WindowListMenu( fTeam, fSig );
	return 0;
}

void TAppPanelIcon::MessageReceived( BMessage *message )
{
	if ( message->what == kActiveBitmapBlendingAnimation )
	{
		fAlphaAnimationFloatingMessages --;
		if ( fAlphaAnimationFloatingMessages <= kActiveBitmapBlendingAnimationStepCount )
		{
			if ( fParent->DoAlphaBlendedActiveHandles() )
			{
				if ( fActive && fActiveBitmapAlpha < 1.f )
					fActiveBitmapAlpha += 1.f / (float)kActiveBitmapBlendingAnimationStepCount;
				else if ( !fActive && fActiveBitmapAlpha > 0.f )
					fActiveBitmapAlpha -= 1.f / (float)kActiveBitmapBlendingAnimationStepCount;

				BBitmap *bm;
				if ( fFlashed )
					bm = fParent->FlashedAppHandleBitmap();
				else
					bm = fParent->SmallSignHandleBitmap();

				BRect r;
				r.left = (Frame().Width() - bm->Bounds().Width()) /2;
				r.top = Frame().Height() - bm->Bounds().Height();
				r.right = r.left + bm->Bounds().Width() + 1;
				r.bottom = r.top + bm->Bounds().Height() + 1;

				fParent->Invalidate(this, r);
			}
		}
	}
	else if ( message->what == kFlashAnimation )
	{
/*		if ( fZoomStep < 16 )
			fZoomStep ++;
		fParent->Invalidate(this);*/
	}
	else
		TTrackerIcon::MessageReceived(message);
}

status_t TAppPanelIcon::DroppedSomething( const BMessage *message )
{
	if ( message->what == B_SIMPLE_DATA )
	{
		BMessage msg2app( B_REFS_RECEIVED );
		entry_ref ref;
		int i;
		for ( i=0; ; i++ )
		{
			if ( message->FindRef( "refs", i, &ref ) != B_OK )
				break;
			msg2app.AddRef( "refs", &ref );
		}

		if ( i > 0 )
		{
			if ( fTeam->CountItems() == 0 )
			{
				Launch();
//				todo: fix this
				snooze(500000);
			}
			BMessenger( fSig ).SendMessage( &msg2app );
		}
	}
	else
		return TTrackerIcon::DroppedSomething( message );
	return B_OK;
}

void TAppPanelIcon::Draw()
{
	TZoomableIcon::Draw();

	bool draw_handle = false;
	if ( fParent->DrawSmallSign() )
	{
		if ( fTeam->CountItems() == 0 && fParent->DrawSignWhenNotRunning() )
			draw_handle = true;
		else if ( fTeam->CountItems() > 0 && !fParent->DrawSignWhenNotRunning() )
			draw_handle = true;
	}

	BPoint cl = ContentLocation();

	BView *canvas = fParent->Parent();
	BBitmap *bm;

	if ( fFlashed )
		bm = fParent->FlashedAppHandleBitmap();
	else
		bm = fParent->SmallSignHandleBitmap();

//	if ( draw_handle )
//	{
		canvas->SetDrawingMode( B_OP_ALPHA );
		canvas->SetBlendingMode( B_CONSTANT_ALPHA, B_ALPHA_OVERLAY );
		canvas->SetHighColor( 255, 255, 255, 255 * fActiveBitmapAlpha );
		canvas->DrawBitmap( bm, cl + BPoint( (Frame().Width() - bm->Bounds().Width()) /2, Frame().Height() - bm->Bounds().Height() ) );
//	}

	if ( fParent->DrawCpuUsageBars() )
	{
		float cpuusage = CpuUsage();
		if ( cpuusage >= 1 )
		{
			cpuusage /= 100.f;
			rgb_color C;
			C.alpha = 128;
			C.red = (uint8) (cpuusage * 255);
			C.blue = (uint8) ((1-cpuusage) * 255);
			C.green = 0;
			float size = cpuusage * 26;
			if ( size > 26 )
				size = 26;
			canvas->SetDrawingMode( B_OP_ALPHA );
			canvas->SetBlendingMode( B_CONSTANT_ALPHA, B_ALPHA_OVERLAY );
			canvas->SetHighColor( C );
			canvas->FillRect( BRect( cl.x, cl.y+6+(26-size), cl.x+3, cl.y+32 ) );
		}
	}
}

void TAppPanelIcon::Launch()
{
	if ( fTeam->CountItems() == 0 || ( modifiers() & B_SHIFT_KEY ) )
	{
		if ( strcasecmp( fSig, kTrackerSignature ) )
			IndependentLaunch( fRef );
		else
			be_roster->Launch( &fRef );
		fParent->Invalidate(this, true);
	}
	else
	{
		const char *last_mime = fParent->Parent()->LastActivatedAppSig();
		if ( strcmp( last_mime, fSig ) )
		{
			fWindowCyclerIndex = 0;
		}

		BList fWindowIDList;

		int32 numTeams = fTeam->CountItems();
		for (int32 i = 0; i < numTeams; i++) 
		{
			team_id	theTeam = (team_id)fTeam->ItemAt(i);
			int32 count = 0;
			int32 *tokens = get_token_list(theTeam, &count);

			for ( int j=0; j<count; j++ )
			{
				window_info *wInfo = get_window_info(tokens[j]);
				if ( wInfo && WindowShouldBeListed(wInfo->w_type) )
				{
					fWindowIDList.AddItem(wInfo);
				}
			}

			free(tokens);
		}

		if ( fWindowCyclerIndex >= fWindowIDList.CountItems() )
		{
			if ( fWindowIDList.CountItems() > 1 )
			{
				window_info *w1 = (window_info*)fWindowIDList.ItemAt(0);
				window_info *w2 = (window_info*)fWindowIDList.ItemAt(1);
	
				if ( w1->team == w2->team )
					fWindowCyclerIndex = 1;
				else
					fWindowCyclerIndex = 0;
			}
			else
				fWindowCyclerIndex = 0;
		}

		int starting_cycle = fWindowCyclerIndex;
		int nlaps = 0;
		int32 first_mini = -1;

		while ( 1 )
		{
			if ( starting_cycle == fWindowCyclerIndex )
				nlaps++;
			if ( nlaps > 1 )
				break;

			if ( fWindowCyclerIndex >= fWindowIDList.CountItems() )
			{
/*				if ( fWindowIDList.CountItems() > 1 )
				{
					window_info *w1 = (window_info*)fWindowIDList.ItemAt(0);
					window_info *w2 = (window_info*)fWindowIDList.ItemAt(1);
	
					if ( w1->team == w2->team )
						fWindowCyclerIndex = 1;
					else
						fWindowCyclerIndex = 0;
				}
				else*/
					fWindowCyclerIndex = 0;
			}

			window_info *wInfo = (window_info*)fWindowIDList.ItemAt(fWindowCyclerIndex);

			if ( wInfo && wInfo->id >= 0 )
			{
//				if ( !wInfo->is_mini )
//				{
					do_window_action(wInfo->id, B_BRING_TO_FRONT, BRect(0,0,0,0), false);
					fWindowCyclerIndex ++;
//				}
//				else
//					first_mini = wInfo->id;
				break;
			}

			fWindowCyclerIndex ++;					
		}

		if ( nlaps > 1 && first_mini > 0)
			do_window_action( first_mini, B_BRING_TO_FRONT, BRect(0,0,0,0), false);

		int32 widcount = fWindowIDList.CountItems();
		for (int32 k=0; k<widcount; k++ )
		{
			free( fWindowIDList.ItemAt(k) );
		}
	}
}

void TAppPanelIcon::Quit()
{
	if ( !strcasecmp( fSig, kTrackerSignature ) )
		return;

	for (int32 index = 0; index < fTeam->CountItems(); index++) {
		team_id team = (team_id)fTeam->ItemAt(index);

		BMessenger((char *)NULL, team).SendMessage(B_QUIT_REQUESTED);
	}
}

void TAppPanelIcon::EntryRemoved()
{
	if ( be_roster->FindApp( fSig, &fRef ) == B_OK )
	{
		SetRefTo( fRef );
		ReloadIcons();
	}
	else
		TTrackerIcon::EntryRemoved();
}

void TAppPanelIcon::SetActive( bool b )
{
	if ( fFlashed )
	{
		fFlashed = false;
//		fZoomStep = 0;
	}

	fActive = b;
	if ( fAlphaAnimationFloatingMessages < kActiveBitmapBlendingAnimationStepCount )
	{
		if ( fParent->DoAlphaBlendedActiveHandles() )
		{
			new BMessageRunner( BMessenger(this), new BMessage(kActiveBitmapBlendingAnimation), kActiveBitmapBlendingAnimationLength/kActiveBitmapBlendingAnimationLength, kActiveBitmapBlendingAnimationStepCount);
			fAlphaAnimationFloatingMessages += kActiveBitmapBlendingAnimationStepCount;
		}
	}
	if ( fParent->DoAlphaBlendedActiveHandles() )
	{
		if ( fActive )
			fActiveBitmapAlpha = 0.f;
		else
			fActiveBitmapAlpha = 1.f;
	}
	else
	{
		if ( fActive )
			fActiveBitmapAlpha = 1.f;
		else
			fActiveBitmapAlpha = 0.f;
	}

	fParent->Invalidate(this);
}

float TAppPanelIcon::CpuUsage()
{
	float usage = 0;
	bigtime_t now = system_time();
	bigtime_t ukern = 0, uuser = 0;
	for ( int i=0; i<fTeam->CountItems(); i++ )
	{
		team_id tid = (team_id)fTeam->ItemAt(i);

		int32 cookie = 0;
		thread_info tinfo;
	
		while ( get_next_thread_info( tid, &cookie, &tinfo ) == B_OK )
		{
			ukern += tinfo.kernel_time;
			uuser += tinfo.user_time;
		}
	}

	bigtime_t totaltime = ukern + uuser - fPrevKernTime - fPrevUserTime;
	fPrevKernTime = ukern;
	fPrevUserTime = uuser;

	usage = (float) ( double(totaltime) * 100 / ( now - fPreviousTiming ) );

	fPreviousTiming = now;

	return usage;
}

void TAppPanelIcon::Flash()
{
	if ( !fActive && !fFlashed )
	{
		fFlashed = true;
//		new BMessageRunner( BMessenger(this), new BMessage(kFlashAnimation), 50000, 16);
		fParent->Invalidate(this);
	}
}

void TAppPanelIcon::PrepareDrawing()
{
	TZoomableIcon::PrepareDrawing();
	fDisabled = fTeam->CountItems() == 0;
}

TClockIcon::TClockIcon()
	: TPanelIcon()
{
	fShowDate = false;
	fShow24h = false;
	fShowSeconds = true;
}

TClockIcon::TClockIcon( BMessage *msg )
	: TPanelIcon( msg )
{
	if ( msg->FindBool( "ShowDate", &fShowDate ) != B_OK )
		fShowDate = false;
	if ( msg->FindBool( "Show24h", &fShow24h ) != B_OK )
		fShow24h = false;
	if ( msg->FindBool( "ShowSeconds", &fShowSeconds ) != B_OK )
		fShowSeconds = true;
}

TClockIcon::~TClockIcon()
{
	delete fTimer;
}

void TClockIcon::AttachedToPanel()
{
	fFont = BFont( be_bold_font );
	fFont.SetSize( kDefaultBigIconSize/3+1 );

	fTimer = new BMessageRunner( BMessenger(this), new BMessage(kClockTimerMessage), 500000 );
}

void TClockIcon::GetPreferredSize( float *width, float *height )
{
	fIWidth = fFont.StringWidth( "00:00:00" ) + 16;
	fIHeight = kDefaultBigIconSize;

	*width = fIWidth;
	*height = fIHeight;
}

TAwarePopupMenu *TClockIcon::Menu()
{
	TAwarePopupMenu *menu = new TAwarePopupMenu("ClockMenu");

	menu->AddItem( new TMenuItem( fShowDate ? B_TRANSLATE("Show clock") : B_TRANSLATE("Show date"), new BMessage( kClockToggleClockDate ) ) );

	if ( !fShowDate )
		menu->AddItem( new TMenuItem(B_TRANSLATE("24 hour clock"), new BMessage(kClockToggle24hDisplay) ) );

	if ( fShow24h && !fShowDate )
		menu->AddItem( new TMenuItem( fShowSeconds ? B_TRANSLATE("Hide seconds") : B_TRANSLATE("Show seconds"), new BMessage(kClockToggleShowSeconds) ) );

	menu->AddSeparatorItem();

	entry_ref ref;
	if ( be_roster->FindApp( "application/x-vnd.Be-TIME", &ref ) == B_OK )
	{
		BEntry e(&ref, true);
		BBitmap* icon = new BBitmap( GetTrackerIcon(&e, B_MINI_ICON) );
		menu->AddItem( new TBitmapMenuItem( B_TRANSLATE("Change time"), icon, new BMessage(kLaunchTimePrefs) ) );
	}
	else
	{
		delete menu;

		return 0;
	}

	return menu;
}

void TClockIcon::MouseDown( BPoint where, uint32 modifiers, uint32 buttons )
{
	if ( buttons & B_PRIMARY_MOUSE_BUTTON )
	{
		fShowDate = !fShowDate;
	}
	else
		TPanelIcon::MouseDown( where, modifiers, buttons );
}

void TClockIcon::Draw()
{
	DrawTime();
}

void TClockIcon::MessageReceived( BMessage *message )
{
	if ( message->what == kClockTimerMessage )
	{
		fParent->Invalidate(this);
	}
	else if ( message->what == kLaunchTimePrefs )
	{
		be_roster->Launch( "application/x-vnd.Be-TIME" );
	}
	else if ( message->what == kClockToggleClockDate )
	{
		fShowDate = !fShowDate;
		fParent->Invalidate(this);
	}
	else if ( message->what == kClockToggle24hDisplay )
	{
		fShow24h = !fShow24h;
		fParent->Invalidate(this);
	}
	else if ( message->what == kClockToggleShowSeconds )
	{
		fShowSeconds = !fShowSeconds;
		fParent->Invalidate(this);
	}
	else
		TPanelIcon::MessageReceived(message);
}

void TClockIcon::DrawTime()
{
	time_t fCurrentTime;

	BView *canvas = fParent->Parent();

	fCurrentTime = time(NULL);

	canvas->SetHighColor( 0, 0, 0 );
	canvas->SetLowColor( fParent->FrameColor() );
	struct tm *_time = localtime( &fCurrentTime );
	char what[64];

	const char *str;

	if ( fShowDate )
	{
		if ( fShow24h )
			strftime(what, 64, "%d/%m/%y", _time );
		else
			strftime(what, 64, "%m/%d/%y", _time );

		str = what;
	}
	else
	{
		if ( fShow24h )
		{
			if ( fShowSeconds )
				strftime(what, 64, "%H:%M:%S", _time);
			else
				strftime(what, 64, "%H:%M", _time);
		}
		else
			strftime(what, 64, "%I:%M %p", _time );

		if ( what[0] == '0' )
			str = &what[1];
		else
			str = what;
	}

	float s_width = fFont.StringWidth( str );

	BPoint where = ContentLocation();
	where.x += (fIWidth - s_width) / 2;
	where.y += (fIHeight - fFont.Size());
	BFont font;
	canvas->GetFont(&font);
	canvas->SetFont(&fFont);
	canvas->DrawString( str, where );
	canvas->SetFont(&font);
}

// this functions were taken from FLITE's source

static const char *time_approx(int hour, int minute)

{
    int mm;

    mm = minute % 5;

    if ((mm == 0) || (mm == 4))
	return B_TRANSLATE("Exactly");
    else if (mm == 1)
	return B_TRANSLATE("Just after");
    else if (mm == 2)
	return B_TRANSLATE("A little after");
    else
	return B_TRANSLATE("Almost");
}

static const char *time_min(int hour, int minute)
{
    int mm;

    mm = minute / 5;
    if ((minute % 5) > 2)
	mm += 1;
    mm = mm * 5;
    if (mm > 55)
	mm = 0;

    if (mm == 0)
	return "";
    else if (mm == 5)
	return B_TRANSLATE("five past");
    else if (mm == 10)
	return B_TRANSLATE("ten past");
    else if (mm == 15)
	return B_TRANSLATE("quarter past");
    else if (mm == 20)
	return B_TRANSLATE("twenty past");
    else if (mm == 25)
	return B_TRANSLATE("twenty-five past");
    else if (mm == 30)
	return B_TRANSLATE("half past");
    else if (mm == 35)
	return B_TRANSLATE("twenty-five to");
    else if (mm == 40)
	return B_TRANSLATE("twenty to");
    else if (mm == 45)
	return B_TRANSLATE("quarter to");
    else if (mm == 50)
	return B_TRANSLATE("ten to");
    else if (mm == 55)
	return B_TRANSLATE("five to");
    else
	return B_TRANSLATE("five to");
}

static const char *time_hour(int hour, int minute)
{
    int hh;

    hh = hour;
    if (minute > 33)
	hh += 1;
    if (hh == 24)
	hh = 0;
    if (hh > 12)
	hh -= 12;

    if (hh == 0)
	return B_TRANSLATE("midnight");
    else if (hh == 1)
	return B_TRANSLATE("one");
    else if (hh == 2)
	return B_TRANSLATE("two");
    else if (hh == 3)
	return B_TRANSLATE("three");
    else if (hh == 4)
	return B_TRANSLATE("four");
    else if (hh == 5)
	return B_TRANSLATE("five");
    else if (hh == 6)
	return B_TRANSLATE("six");
    else if (hh == 7)
	return B_TRANSLATE("seven");
    else if (hh == 8)
	return B_TRANSLATE("eight");
    else if (hh == 9)
	return B_TRANSLATE("nine");
    else if (hh == 10)
	return B_TRANSLATE("ten");
    else if (hh == 11)
	return B_TRANSLATE("eleven");
    else if (hh == 12)
	return B_TRANSLATE("twelve");
    else
	return B_TRANSLATE("twelve");
}

static const char *time_tod(int hour, int minute)
{
    int hh = hour;

    if (minute > 58)
	hh++;

    if (hh == 24)
	return "";
    else if (hh > 17)
	return B_TRANSLATE("In the evening");
    if (hh > 11)
	return B_TRANSLATE("In the afternoon");
    else
	return B_TRANSLATE("In the morning");
}

const char *TClockIcon::BubbleText() const
{
	time_t fCurrentTime;
	fCurrentTime = time(NULL);
	struct tm *_time = localtime( &fCurrentTime );

	int hour = _time->tm_hour;
	int min = _time->tm_min;

	char thetime[512];
	char formatedday[256];
	strftime( formatedday, 256, "Today is %A, the %d %B of %Y", _time );

    sprintf(thetime,
	    "%s, %s %s %s, %s", B_TRANSLATE("The time is now"),
	    //"The time is now, %s %s %s, %s",
	    time_approx(hour,min),
	    time_min(hour,min),
	    time_hour(hour,min),
	    time_tod(hour,min));

//	sprintf( thetime, "%s%s", thetime, formatedday );

	return thetime;
}

TTrashIcon::TTrashIcon()
	: TTrackerIcon( 0 )
{
	entry_ref ref;
	BPath path;

	find_directory( B_TRASH_DIRECTORY, &path );
	get_ref_for_path(path.Path(), &ref);

	SetRefTo( ref );
}

TTrashIcon::TTrashIcon(BMessage *message)
	: TTrackerIcon( message )
{
	entry_ref ref;
	BPath path;

	find_directory( B_TRASH_DIRECTORY, &path );
	get_ref_for_path(path.Path(), &ref);

	SetRefTo( ref );
}

TTrashIcon::~TTrashIcon()
{
}

bool TTrashIcon::Removable() const
{
	BAlert *alert = new BAlert( "Question", B_TRANSLATE("Do you really wish to remove the Trash icon?"), B_TRANSLATE("Yes"), B_TRANSLATE("No"));
	int32 res = alert->Go();
	return res == 0;
}

TAwarePopupMenu *TTrashIcon::Menu()
{
	bool first = true;

	BVolumeRoster vroster;
	BVolume volume;

	TAwarePopupMenu *menu = new TAwarePopupMenu("TrashMenu");

	int entrycount = 0;

	vroster.Rewind();
	while ( vroster.GetNextVolume( &volume ) == B_OK )
	{
		if ( volume.IsRemovable() || volume.IsReadOnly() || volume.IsShared() || !volume.IsPersistent() )
			continue;
	
		BPath path;
		if ( find_directory( B_TRASH_DIRECTORY, &path, false, &volume ) == B_OK )
		{
			if ( !first )
				menu->AddSeparatorItem();
			else
				first = false;

			entry_ref ref;
			get_ref_for_path(path.Path(), &ref);
			BDirectory bdir( &ref );

			char buf[4096];
			dirent *dent;
			int count;
			int thiscount = 0;
			while( ( count = bdir.GetNextDirents( (dirent*)buf, 4096)) > 0 )
			{
				dent = (dirent*)buf;
				while ( count -- )
				{
					if ( !strcmp( dent->d_name, "." ) || !strcmp( dent->d_name, ".." ) )
						continue;
					entry_ref eref;
					eref.device = dent->d_pdev;
					eref.directory = dent->d_pino;
					eref.set_name( dent->d_name );
		
					BPath path( &eref );
					BEntry e(&eref, true);
					BBitmap *bitmap = new BBitmap( GetTrackerIcon(&e, B_MINI_ICON) ); 

					menu->AddItem( new TTrackerMenuItem( path.Leaf(), bitmap, eref ) );
					entrycount ++;
					thiscount ++;
		
					dent = (dirent *)((char *)dent + dent->d_reclen);
				}
			}

			if ( thiscount == 0 )
			{
				TMenuItem *empty = new TMenuItem( B_TRANSLATE("No entries"), 0 );
				empty->SetEnabled(false);
				menu->AddItem( empty );
			}			
		}
	}

	TMenuItem *trashitem = new TMenuItem( B_TRANSLATE("Empty trash"), new BMessage(kTrackerEmptyTrash) );
	if ( entrycount == 0 )
		trashitem->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem( trashitem );

	return menu;
}

status_t TTrashIcon::DroppedSomething( const BMessage *message )
{
	if ( message->what == B_SIMPLE_DATA )
	{
		BObjectList<entry_ref> *list = new BObjectList<entry_ref>();
		entry_ref ref;
		int i;
		for ( i=0; ; i++ )
		{
			if ( message->FindRef( "refs", i, &ref ) != B_OK )
				break;
			list->AddItem( new entry_ref(ref) );
		}

		FSMoveToTrash( list, 0, true );
	}
	else
		return TTrackerIcon::DroppedSomething( message );

	return B_OK;
}

void TTrashIcon::MessageReceived( BMessage *message )
{
	if ( message->what == kTrackerEmptyTrash )
	{
		BMessage emi( B_DELETE_PROPERTY );
		emi.AddSpecifier( "Trash" );
		BMessenger( kTrackerSignature ).SendMessage( &emi ); 
	}
	else
		TTrackerIcon::MessageReceived(message);
}

TWorkspacesIcon::TWorkspacesIcon()
	: TPanelIcon()
{
	entry_ref ref;
	be_roster->FindApp( "application/x-vnd.Be-WORK", &ref );
	BEntry e(&ref, true);

	fWorkspaceImage = new BBitmap(GetTrackerIcon(&e, B_MINI_ICON));	

	fFont = BFont( be_bold_font );
	fFont.SetSize( 11 );
}

TWorkspacesIcon::TWorkspacesIcon( BMessage *message )
	: TPanelIcon( message )
{
	entry_ref ref;
	be_roster->FindApp( "application/x-vnd.Be-WORK", &ref );
	BEntry e(&ref, true);
	fWorkspaceImage = new BBitmap(GetTrackerIcon(&e, B_MINI_ICON));	

	fFont = BFont( be_bold_font );
	fFont.SetSize( 11 );
}

TWorkspacesIcon::~TWorkspacesIcon()
{
	delete fWorkspaceImage;
}

void TWorkspacesIcon::GetPreferredSize( float *width, float *height )
{
	*width = kDefaultBigIconSize;
	*height = kDefaultBigIconSize;
}

void TWorkspacesIcon::AttachedToPanel()
{
	fParent->Parent()->RegisterWorkspaceChange( new BMessenger(this) );
}

TAwarePopupMenu *TWorkspacesIcon::Menu()
{
	TAwarePopupMenu *menu = new TAwarePopupMenu("WorkspacesMenu");
	for ( int i=0; i<count_workspaces(); i++ )
	{
		char ws[4];
		sprintf( ws, "%i", i+1 );
		menu->AddItem( new TWorkspacesMenuItem( ws, i ) );
	}

	return menu;
}

void TWorkspacesIcon::MouseDown( BPoint /* where */, uint32 /* modifiers */, uint32 buttons )
{
	int32 current = current_workspace();
	if ( buttons & B_PRIMARY_MOUSE_BUTTON )
	{
		current ++;
		if ( current >= count_workspaces() )
			current = 0;
		activate_workspace(current);
	}
	else if ( buttons & B_SECONDARY_MOUSE_BUTTON )
	{
		current --;
		if ( current < 0 )
			current = count_workspaces() - 1;
		activate_workspace(current);
	}
	else if ( buttons & B_TERTIARY_MOUSE_BUTTON )
	{
		DoMenu();
	}
}

void TWorkspacesIcon::Draw()
{
	DrawIcon();
}

void TWorkspacesIcon::MessageReceived( BMessage *message )
{
	if ( message->what == B_WORKSPACE_ACTIVATED )
		fParent->Invalidate(this);
	else
		TPanelIcon::MessageReceived(message);
}

static char _message[256];

const char *TWorkspacesIcon::BubbleText() const
{
/*	int c = current_workspace();
	int n = c+1;
	if ( n > count_workspaces() )
		n = 0;
	int p = c-1;
	if ( p < 0 )
		p = count_workspaces()-1;
	sprintf(_message, "You are now in workspace %i,\nleft click to go to workspace %i and right click to go to %i", c+1, n+1, p+1 );
	return _message;*/
	return 0;
}

void TWorkspacesIcon::DrawIcon()
{
//	BPoint topleft = ContentLocation();
//	BRect thisframe = BRect( topleft, BPoint( topleft.x+Frame().Width(), topleft.y+Frame().Height() ) ) ;

	BView *canvas = fParent->Parent();

	char workspace_number[16];
	sprintf( workspace_number, "%i", (int)current_workspace()+1 );

	float s_width = fFont.StringWidth( workspace_number );
	s_width = (kDefaultBigIconSize - s_width) / 2;

	BPoint where = ContentLocation();

	canvas->SetDrawingMode( B_OP_ALPHA );
	canvas->SetHighColor( 0, 0, 0, 50 );
	canvas->SetBlendingMode( B_CONSTANT_ALPHA, B_ALPHA_OVERLAY );
	canvas->DrawBitmap( fWorkspaceImage, where + BPoint((kDefaultBigIconSize-B_MINI_ICON)/2+1,(kDefaultBigIconSize-B_MINI_ICON)/2) );
	canvas->SetDrawingMode( B_OP_COPY );
	BFont font;
	canvas->GetFont(&font);
	canvas->SetFont(&fFont);
	canvas->DrawString( workspace_number, where + BPoint( s_width, kDefaultBigIconSize-fFont.Size() ) );
	canvas->SetFont(&font);
}

TDockbertIcon::TDockbertIcon( entry_ref &ref )
	: TTrackerIcon( ref )
{
}

TDockbertIcon::TDockbertIcon( BMessage *message )
	: TTrackerIcon( message )
{
}

bool TDockbertIcon::Removable() const
{
	BAlert *alert = new BAlert( "Question", B_TRANSLATE("ReallyRemoveTheBeMenu"), B_TRANSLATE("Yes"), B_TRANSLATE("No"));
	int32 res = alert->Go();
	return res == 0;
}

TAwarePopupMenu *TDockbertIcon::Menu()
{
	entry_ref ref;
	BPath path;

	TAwarePopupMenu *menu = new TAwarePopupMenu("BeMenu");

	menu->AddItem( new TMenuItem( B_TRANSLATE("Restart"), new BMessage(CMD_REBOOT_SYSTEM) ) );
	menu->AddItem( new TMenuItem( B_TRANSLATE("Shut down"), new BMessage(CMD_SHUTDOWN_SYSTEM) ) );

	menu->AddSeparatorItem();

	find_directory (B_USER_DESKBAR_DIRECTORY, &path);
	get_ref_for_path(path.Path(), &ref);

	TDockMenus::BuildTrackerMenu( menu, ref );

	menu->AddSeparatorItem();

	BBitmap *icon = NULL; //new BBitmap( BRect( 0, 0, B_MINI_ICON-1, B_MINI_ICON-1), B_RGBA32);

	if ( be_roster->FindApp( "application/x-vnd.Be-MAGN", &ref ) == B_OK )
	{
		BEntry e(&ref, true);
		icon = new BBitmap( GetTrackerIcon(&e, B_MINI_ICON) );
	}

	menu->AddItem( new TBitmapMenuItem( B_TRANSLATE("Find"), icon, new BMessage(kFindButton) ) );
	BString s(B_TRANSLATE("About"));
	s << " Zeta";
	TBitmapMenuItem *item = new TBitmapMenuItem(s.String(), const_cast<BBitmap*>( AppResSet()->FindBitmap( 'BBMP', R_BeLogo ) ), new BMessage(kShowSplash) );
	item->SetBitmapAutoDestruct(false);
	menu->AddItem( item );

	return menu;
}

void TDockbertIcon::MessageReceived( BMessage *message )
{
	switch ( message->what )
	{
	case kShowSplash:
		system("/boot/beos/system/About &");
//		run_be_about();
		break;
	case kFindButton:
		BMessenger(kTrackerSignature).SendMessage(message);
		break;
	case kDeskbarPreferences:
		be_roster->Launch( "application/x-vnd.Deskbar-Preferences" );
		break;
	case CMD_REBOOT_SYSTEM:
		BMessenger(ROSTER_SIG).SendMessage(message);
		break;
	case CMD_SHUTDOWN_SYSTEM:
		BMessenger(ROSTER_SIG).SendMessage(message);
		break;
	default:
		TTrackerIcon::MessageReceived(message);
	}
}

void TDockbertIcon::MouseDown( BPoint point, uint32 modifiers, uint32 buttons )
{
	if ( buttons & B_PRIMARY_MOUSE_BUTTON )
	{
		DoMenu();
	}
	else
		TTrackerIcon::MouseDown( point, modifiers, buttons );
}

TSeparatorIcon::TSeparatorIcon()
	: TPanelIcon()
{
	fBitmap = const_cast<BBitmap*>( AppResSet()->FindBitmap( 'BBMP', R_SeparatorIcon ) );
}

TSeparatorIcon::TSeparatorIcon(BMessage *message)
	: TPanelIcon(message)
{
	fBitmap = const_cast<BBitmap*>( AppResSet()->FindBitmap( 'BBMP', R_SeparatorIcon ) );
}

void TSeparatorIcon::GetPreferredSize( float *width, float *height )
{
	*width = 19;
	*height = 32;
}

void TSeparatorIcon::Draw()
{
	BView *canvas = fParent->Parent();

	canvas->SetDrawingMode( B_OP_ALPHA );
	canvas->SetBlendingMode( B_PIXEL_ALPHA, B_ALPHA_OVERLAY );

	canvas->DrawBitmap( fBitmap, ContentLocation() + BPoint( 6, 8 ) );
}

TShowDesktopIcon::TShowDesktopIcon()
	: TZoomableIcon()
{
	fSmallIcon = const_cast<BBitmap*>( AppResSet()->FindBitmap( 'BBMP', R_ShowDesktopIconSmall ) );
	fBigIcon = const_cast<BBitmap*>( AppResSet()->FindBitmap( 'BBMP', R_ShowDesktopIcon ) );

	SetDestroyBitmaps(false);
}

TShowDesktopIcon::TShowDesktopIcon(BMessage *msg)
	: TZoomableIcon(msg)
{
	fSmallIcon = const_cast<BBitmap*>( AppResSet()->FindBitmap( 'BBMP', R_ShowDesktopIconSmall ) );
	fBigIcon = const_cast<BBitmap*>( AppResSet()->FindBitmap( 'BBMP', R_ShowDesktopIcon ) );

	SetDestroyBitmaps(false);
}

void TShowDesktopIcon::MouseDown( BPoint /* where */, uint32 /* modifiers */, uint32 buttons )
{
	if ( buttons & B_PRIMARY_MOUSE_BUTTON )
	{
		BList teamlist;
		be_roster->GetAppList( &teamlist );
		int32 count = teamlist.CountItems();
	
		team_id deskbar_team = -1;
		app_info nfo;
	
		if ( be_roster->GetAppInfo( "application/x-vnd.Be-TSKB", &nfo ) == B_OK )
			deskbar_team = nfo.team;
	
		fWindowList.MakeEmpty();
	
		for ( int32 i=0; i<count; i++ )
		{
			team_id	theTeam = (team_id)teamlist.ItemAt(i);	
	
			if ( theTeam == deskbar_team )
				continue;
	
			if ( (be_roster->GetRunningAppInfo( theTeam, &nfo ) != B_OK) || ( nfo.flags & B_BACKGROUND_APP ) )
				continue;
	
			int32 count = 0;
			int32 *tokens = get_token_list(theTeam, &count);
	
			for (int32 j = 0; j < count; j++)
			{
				window_info *wInfo = get_window_info(tokens[j]);
				if (wInfo == NULL)
					continue;
	
				if (WindowShouldBeListed(wInfo->w_type) && (wInfo->show_hide_level <= 0 || wInfo->is_mini))
				{
					if ( ((1 << current_workspace()) & wInfo->workspaces) != 0 )
					{
						do_window_action(wInfo->id, B_MINIMIZE_WINDOW, BRect(0.0f, 0.0f, 0.0f, 0.0f), false);
						fWindowList.AddItem( (void*) wInfo->id );
					}
				}
				free(wInfo);
			}
			free(tokens);
		}
	}
	else if ( buttons & B_SECONDARY_MOUSE_BUTTON )
	{
		int32 count = fWindowList.CountItems();
	
		for ( int32 i=0; i<count; i++ )
		{
			int32 wid = (int32)fWindowList.ItemAt( i );
			do_window_action( wid, B_BRING_TO_FRONT, BRect( 0, 0, 0, 0 ), false );
		}
	
		fWindowList.MakeEmpty();
	}
}

const char *TShowDesktopIcon::BubbleText() const
{
	return B_TRANSLATE("Left Click to Hide All Windows in this workspace,\nRight click to bring them back.");
}

