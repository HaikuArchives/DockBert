#include "TrackerMenus.h"

//#include <sys_apps/Tracker/Icons.h>
#include <Entry.h>
#include <Directory.h>
#include <Path.h>
#include <NodeInfo.h>
#include <Bitmap.h>
#include <Roster.h>
#include <FindDirectory.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <locale/Locale.h>

#include "tracker_private.h"

#include "DockArchivableUtils.h"


#include <stdio.h>
#include <strings.h>

#include "FSUtils.h"
#include "Commands.h"
//#include "IconCache.h"

#include "ResourceSet.h"
#include "icons.h"
#include "othericons.h"

#include "WindowMenuItem.h"

#include "IconMenuItem.h"

#include <Catalog.h>
#include <Locale.h>

#define B_TRANSLATION_CONTEXT "tracker-menus"

//#include <private/IconLoader.h>

const int32 kDesktopWindow = 1024;
const int32 kMenuWindow	= 1025;
const uint32 kWindowScreen = 1026;
const uint32 kNormalWindow = 0;
const int32 kTeamFloater = 4;
const int32 kListFloater = 5;
const int32 kSystemFloater = 6;

BBitmap* GetTrackerIcon(BEntry *e, icon_size which);

bool
WindowShouldBeListed(uint32 behavior)
{
	if (behavior == kNormalWindow || behavior == kWindowScreen)
		return true;

	return false;
}

using namespace BPrivate;

TDockWindowMenuItem::TDockWindowMenuItem( TMenu *menu, int32 id, bool mini, bool current_ws, uint32 workspaces )
	: TBitmapMenuItem(menu, (BBitmap*)0), fID(id), fMini(mini), fCurrentWS(current_ws)
{
	SetBitmapAutoDestruct(false);

	if (fMini)
 		fBitmap = const_cast<BBitmap*>( fCurrentWS
			? AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_WindowHiddenIcon)
			: AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_WindowHiddenSwitchIcon)
			);
	else
 		fBitmap = const_cast<BBitmap*>( fCurrentWS
			? AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_WindowShownIcon)
			: AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_WindowShownSwitchIcon)
			);

	fBitmapDeslX = fBitmap->Bounds().Width() + 6;
	fBitmapDeslY = 3;

	if ( !fCurrentWS )
	{
		int32 nworkspace = -1;
		for ( int32 i=0; i<32; i++ )
		{
			if ( (workspaces >> i) & 1 )
			{
				if ( nworkspace != -1 ) // multiple
				{
					nworkspace = -1;
					break;
				}
				nworkspace = i;
			}
		}

		if ( nworkspace == -1 )
			strcpy( fWorkspaceString, "M" );
		else
			sprintf( fWorkspaceString, "%i", (int)nworkspace+1 );

		fWorkspaceBitmap = AppResSet()->FindBitmap( 'BBMP', R_WorkspacesIcon );
	}
}

void TDockWindowMenuItem::GetContentSize( float *width, float *height )
{
	TBitmapMenuItem::GetContentSize( width, height );
	if ( !fCurrentWS )
	{
		*width += 2 + 16 + 2;
		if ( *height < 20 )
			*height = 20;
	}
}

const int32 kSubmenuHandleSize = 20;

void TDockWindowMenuItem::DrawContent()
{
	if ( !fCurrentWS )
	{
		rgb_color C = Menu()->HighColor();
		C.alpha = 50;
		Menu()->SetHighColor( C );
		Menu()->SetDrawingMode( B_OP_ALPHA );
		Menu()->SetBlendingMode( B_CONSTANT_ALPHA, B_ALPHA_OVERLAY );
		Menu()->DrawBitmap( fWorkspaceBitmap, ContentLocation() + BPoint( Frame().Width() - 16 - 16 - kSubmenuHandleSize, 2 ) );
		float wssw = Menu()->StringWidth( fWorkspaceString );
		Menu()->SetHighColor( 0, 0, 0, 180 );
		Menu()->DrawString( fWorkspaceString, ContentLocation() + BPoint( Frame().Width() - ((32-wssw)/2) - 15 - kSubmenuHandleSize, 12 ) );
		Menu()->SetDrawingMode( B_OP_COPY );
	}
	TBitmapMenuItem::DrawContent();
}

status_t TDockWindowMenuItem::Invoke( BMessage *message )
{
	if (fID >= 0) {
		int32 action = (modifiers() & B_CONTROL_KEY)
			? B_MINIMIZE_WINDOW :B_BRING_TO_FRONT;

		bool doZoom = false;
		BRect zoomRect(0.0f, 0.0f, 0.0f, 0.0f);

		do_window_action(fID, action, zoomRect, doZoom);
	}

	return TBitmapMenuItem::Invoke(message);
}

TDockInnerWindowMenuItem::TDockInnerWindowMenuItem( const char *label, BBitmap *bitmap, int32 action, int32 window_id, team_id teamid, const char *name )
	: TBitmapMenuItem( label, bitmap ), fAction(action), fID(window_id), fParentTeamID(teamid), fWindowName(name)
{
	SetBitmapAutoDestruct(false);
}

status_t TDockInnerWindowMenuItem::Invoke( BMessage *message )
{
	switch (fAction) {
	case B_MINIMIZE_WINDOW:
	case B_BRING_TO_FRONT:
		do_window_action( fID, fAction, BRect( 0,0,0,0 ), false );
		break;

	case B_QUIT_REQUESTED:
		{
			BMessage msg(B_QUIT_REQUESTED);
			msg.AddSpecifier( "Window", fWindowName.String() );
			BMessenger(0, fParentTeamID).SendMessage(&msg);

			break;
		}
	
	case T_BRING_HERE:
		{
			BMessage msg(B_SET_PROPERTY);
			msg.AddInt32( "data", 1<<current_workspace() );
			msg.AddSpecifier( "Workspaces" );
			msg.AddSpecifier( "Window", fWindowName.String() );
			BMessenger(0, fParentTeamID).SendMessage(&msg);

			break;
		}
	}

	return TBitmapMenuItem::Invoke(message);
}

TDockShowHideMenuItem::TDockShowHideMenuItem( const char *title, BBitmap *bitmap, const BList *teams, uint32 action )
	: TBitmapMenuItem( title, bitmap ), fTeams(teams), fAction(action)
{
	SetBitmapAutoDestruct(false);
}

status_t TDockShowHideMenuItem::Invoke( BMessage *message )
{
	if (fTeams != NULL)
	{
	
		bool doZoom = false;
		BRect zoomRect(0, 0, 0, 0);
	
		int32 count = fTeams->CountItems();
		for (int32 index = 0; index < count; index++) {
			team_id team = (team_id)fTeams->ItemAt(index);
	
			switch (fAction) {
				case B_MINIMIZE_WINDOW:
					do_minimize_team(zoomRect, team, doZoom && index == 0);
					break;
	
				case B_BRING_TO_FRONT:
					do_bring_to_front_team(zoomRect, team, doZoom && index == 0);
					break;
	
				case B_QUIT_REQUESTED:
					{
						BMessenger messenger((char *)NULL, team);
						uint32 command = B_QUIT_REQUESTED;
						app_info aInfo;
						be_roster->GetRunningAppInfo(team, &aInfo);
		
						if (strcasecmp(aInfo.signature, "application/x-vnd.Be-TRAK") == 0)
							command = 'Tall';
						
						messenger.SendMessage(command);
						break;
					}
				case T_BRING_HERE:
					{
						int32 count;
						int32 *tokens = get_token_list(team, &count);

						for (int32 j = 0; j < count; j++) {
							client_window_info *wInfo = get_window_info(tokens[j]);
							if (wInfo == NULL)
								continue;
							if ( !WindowShouldBeListed(wInfo->w_type) || wInfo->workspaces == 0xffffffff )
							{
								free(wInfo);
								continue;
							}
							BMessage msg(B_SET_PROPERTY);
							msg.AddInt32( "data", 1<<current_workspace() );
							msg.AddSpecifier( "Workspaces" );
							msg.AddSpecifier( "Window", wInfo->name );
							BMessenger(0, team).SendMessage(&msg);
							free(wInfo);
						}

						free(tokens);

						break;
					}
			}
		}
	}

	return TBitmapMenuItem::Invoke(message);
}

TTrackerMenuItem::TTrackerMenuItem( const char *label, BBitmap *bitmap, entry_ref ref )
	: TBitmapMenuItem( label, bitmap ), fRef(ref)
{
}

TTrackerMenuItem::TTrackerMenuItem( TMenu *submenu, BBitmap *bitmap, entry_ref ref )
	: TBitmapMenuItem( submenu, bitmap ), fRef(ref)
{
}

status_t TTrackerMenuItem::Invoke( BMessage *message )
{
	IndependentLaunch( fRef );
	return TBitmapMenuItem::Invoke(message);
}

TTrackerMenu::TTrackerMenu( const char *label, entry_ref ref )
	: TAwarePopupMenu( label ), fBaseRef( ref ), fCached(false)
{
	Build();
}

void TTrackerMenu::Show()
{
	TAwarePopupMenu::Show();
	Build();
}

void TTrackerMenu::Build()
{
	if ( fCached )
		return;

	TDockMenus::BuildTrackerMenu( this, fBaseRef );

	fCached = true;
}

TAwarePopupMenu *TDockMenus::WindowListMenu(const BList *fTeam, const char *signature, bool add_workspace_functions)
{
	BString fApplicationSignature(signature);

	TAwarePopupMenu *menu = new TAwarePopupMenu( "WindowListMenu" );

	int32 miniCount = 0;
	int32 in_other_workspaces_count = 0;

	int32 numTeams = fTeam->CountItems();
	for (int32 i = 0; i < numTeams; i++) {
		team_id	theTeam = (team_id)fTeam->ItemAt(i);
		int32 count = 0;
		int32 *tokens = get_token_list(theTeam, &count);

		for (int32 j = 0; j < count; j++) {
			client_window_info *wInfo = get_window_info(tokens[j]);
			if (wInfo == NULL)
				continue;

			if (WindowShouldBeListed(wInfo->w_type)
				&& (wInfo->show_hide_level <= 0 || wInfo->is_mini)) {
/*				int32 numItems = fMenu->CountItems();
				int32 addIndex = 0;
				for (; addIndex < numItems; addIndex++)
					if (strcasecmp(ItemAt(addIndex)->Label(), wInfo->name) > 0)
						break;*/

				TMenu *submenu = new TMenu( wInfo->name );
				submenu->SetFont( be_plain_font );

				if ( !(wInfo->workspaces & (1<<current_workspace()) ) )
				{
					in_other_workspaces_count++;

					submenu->AddItem( new TDockInnerWindowMenuItem( B_TRANSLATE("Bring here"), 0, T_BRING_HERE, wInfo->id, theTeam, wInfo->name ) );
					submenu->AddSeparatorItem();
				}

				const char *bring_to_front_label = B_TRANSLATE("Bring to front");
				if (wInfo->is_mini)
				{
					miniCount++;
					bring_to_front_label = B_TRANSLATE("Show");
				}
				else
					submenu->AddItem( new TDockInnerWindowMenuItem( B_TRANSLATE("Hide"), const_cast<BBitmap*>( AppResSet()->FindBitmap('BBMP', R_HideAllButton) ), B_MINIMIZE_WINDOW, wInfo->id, theTeam, wInfo->name ) );
				submenu->AddItem( new TDockInnerWindowMenuItem( bring_to_front_label, const_cast<BBitmap*>( AppResSet()->FindBitmap('BBMP', R_ShowAllButton) ), B_BRING_TO_FRONT, wInfo->id, theTeam, wInfo->name ) );
				submenu->AddItem( new TDockInnerWindowMenuItem( B_TRANSLATE("Close"), const_cast<BBitmap*>( AppResSet()->FindBitmap('BBMP', R_CloseButton) ), B_QUIT_REQUESTED, wInfo->id, theTeam, wInfo->name ) );

				TDockWindowMenuItem* item = new TDockWindowMenuItem(submenu, wInfo->id, 
					wInfo->is_mini, ((1 << current_workspace()) & wInfo->workspaces) != 0, wInfo->workspaces);

				menu->AddItem(item);
			}
			free(wInfo);
		}
		free(tokens);
	}

	int32 itemCount = menu->CountItems();
	if (itemCount < 1) {
		TMenuItem *noWindowsItem = new TMenuItem(B_TRANSLATE("No windows"), 0);

		noWindowsItem->SetEnabled(false);

		menu->AddItem(noWindowsItem);
 		
		// if an application has no windows, this feature makes it easy to quit it.
 		// (but we only add this option if the application is not Tracker.)
 		if (fApplicationSignature.Compare(kTrackerSignature) != 0) {
 			menu->AddSeparatorItem();
			menu->AddItem(new TDockShowHideMenuItem(B_TRANSLATE("Quit application"), 0, fTeam, B_QUIT_REQUESTED));
 		}
	} else {

		menu->AddSeparatorItem();

		if ( add_workspace_functions && in_other_workspaces_count > 0)
		{
//			menu->AddItem( new TDockShowHideMenuItem( "Bring All Here", const_cast<BBitmap*>( AppResSet()->FindBitmap('BBMP', R_BringAllHereButton) ), fTeam, T_BRING_ALL_HERE ) );
			menu->AddItem( new TDockShowHideMenuItem(B_TRANSLATE("Bring all here"), 0, fTeam, T_BRING_HERE ) );
			menu->AddSeparatorItem();
		}

		TDockShowHideMenuItem *hide =
			new TDockShowHideMenuItem(B_TRANSLATE("Hide all"), const_cast<BBitmap*>( AppResSet()->FindBitmap('BBMP', R_HideAllButton) ), fTeam, B_MINIMIZE_WINDOW);
		TDockShowHideMenuItem *show =
			new TDockShowHideMenuItem(B_TRANSLATE("Show all"), const_cast<BBitmap*>( AppResSet()->FindBitmap('BBMP', R_ShowAllButton) ), fTeam, B_BRING_TO_FRONT);
		TDockShowHideMenuItem* close =
			new TDockShowHideMenuItem(B_TRANSLATE("Close all"), const_cast<BBitmap*>( AppResSet()->FindBitmap('BBMP', R_CloseButton) ), fTeam, B_QUIT_REQUESTED);

		if (miniCount == itemCount)
			hide->SetEnabled(false);
		else if (miniCount == 0)
			show->SetEnabled(false);
			
		menu->AddItem(hide);
		menu->AddItem(show);
		menu->AddItem(close);
	}

	return menu;
}

TAwarePopupMenu *TDockMenus::TrackerMenu( entry_ref &ref )
{
	return new TTrackerMenu( "TrackerMenu", ref );
}

int StaticCompareFilesFoldersFirst( const void *i1, const void *i2 )
{
	TTrackerMenuItem *item1 = static_cast<TTrackerMenuItem*>( *((void**)i1) );
	TTrackerMenuItem *item2 = static_cast<TTrackerMenuItem*>( *((void**)i2) );

	if ( !item1 && item2 )
		return -1;
	else if ( !item2 )
		return 1;
	else if ( !item1 && !item2 )
		return 0;

	BEntry entry1( &item1->fRef, true );
	BEntry entry2( &item2->fRef, true );

	bool is_dir1 = entry1.IsDirectory();
	bool is_dir2 = entry2.IsDirectory();

	if ( is_dir1 && !is_dir2 )
		return -1;
	else if ( !is_dir1 && is_dir2 )
		return 1;
	else
	{
		if ( !item1->Label() && item2->Label() )
			return -1;
		else if ( !item2->Label() )
			return 1;
		else if ( !item1->Label() && !item2->Label() )
			return 0;
		return strcasecmp( item1->Label(), item2->Label() );
	}
}

int TDockMenus::BuildTrackerMenu( TMenu *menu, entry_ref &ref )
{
	BList itemlist;

	entry_ref eref;

	BDirectory bdir( &ref );

	BPath path_R(&bdir, 0);
	if ( !strcmp( path_R.Path(), "/" ) )
	{
		BVolumeRoster vr;
		BVolume vlm;
		while ( vr.GetNextVolume( &vlm ) == B_OK )
		{
			if ( vlm.IsPersistent() )
			{
//				BBitmap* bitmap = new BBitmap( BRect( 0, 0, B_MINI_ICON-1, B_MINI_ICON-1), B_RGBA32);
				char name[256];
				vlm.GetName(name);
				BDirectory rootdir;
				vlm.GetRootDirectory(&rootdir);
				BEntry rootentry( &rootdir, "." );
				rootentry.GetRef(&eref);
//				BIcon::Ref Icon = BIconLoader::GetVolumeIcon(&vlm);
				BBitmap* bitmap = new BBitmap(GetTrackerIcon(&rootentry, B_MINI_ICON));
				TAwarePopupMenu *submenu = new TTrackerMenu( name, eref );
				BEntry entryref(&eref);
				Model model(&entryref);
				itemlist.AddItem( new ModelMenuItem( &model, submenu ) );
			}
		}
	}

	char buf[4096];
	dirent *dent;
	int count;
	while( ( count = bdir.GetNextDirents( (dirent*)buf, 4096)) > 0 )
	{
		dent = (dirent*)buf;
		while ( count -- )
		{
			if ( !strcmp( dent->d_name, "." ) || !strcmp( dent->d_name, ".." ) )
				continue;
			eref.device = dent->d_pdev;
			eref.directory = dent->d_pino;
			eref.set_name( dent->d_name );

			BPath path( &eref );
			BEntry e(&eref, true);
			BBitmap *bitmap = new BBitmap( GetTrackerIcon(&e , B_MINI_ICON) );

			BPath realpath;
			BEntry entry( &eref, true );
			entry.GetRef(&eref);
			entry.GetPath(&realpath);
			const char *rootpath = realpath.Path();

			BEntry entryref(&eref);
			Model model(&entryref);

			if ( entry.IsDirectory() )
			{
				BNavMenu *submenu = new BNavMenu( path.Leaf(), B_REFS_RECEIVED, BMessenger(kTrackerSignature ) );
				submenu->SetNavDir( &eref );
				itemlist.AddItem( new TTrackerMenuItem( submenu, bitmap, eref ) );
			}
			else
				itemlist.AddItem( new TTrackerMenuItem( path.Leaf(), bitmap, eref ) );

			dent = (dirent *)((char *)dent + dent->d_reclen);
		}
	}

	bool empty = itemlist.CountItems() == 0;

	if ( empty )
	{
		TMenuItem *empty = new TMenuItem( B_TRANSLATE("No entries"), 0 );
		empty->SetEnabled(false);
		menu->AddItem( empty );
	}
	else
	{
		itemlist.SortItems( StaticCompareFilesFoldersFirst );
		int32 count = itemlist.CountItems();
		for ( int32 i=0; i<count; i++ )
			menu->AddItem( static_cast<TMenuItem*>(itemlist.ItemAt(i)) );
	}

	return itemlist.CountItems();
}
