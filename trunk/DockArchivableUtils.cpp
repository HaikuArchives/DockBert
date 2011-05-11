#include <Archivable.h>
#include <String.h>
#include <Message.h>
#include <Messenger.h>
#include <Node.h>

#include <FindDirectory.h>

#include "ResourceSet.h"
#include "othericons.h"

#include "DockArchivableUtils.h"

#include "InnerPanel.h"
#include "InnerPanelIcons.h"

#include "tracker_private.h"

static struct
{
	const char *classname;
	instantiation_func instantiate;
} kStaticDockInstantiationList[] = {
#define LENTRY( x ) { #x, x::Instantiate }
	LENTRY( TInnerPanel ),
	LENTRY( TRaisingIconPanel ),
	LENTRY( TApplicationPanel ),
	LENTRY( TShortcutPanel ),
	LENTRY( TTrackerIcon ),
	LENTRY( TAppPanelIcon ),
	LENTRY( TDockbertIcon ),
	LENTRY( TClockIcon ),
	LENTRY( TTrashIcon ),
	LENTRY( TWorkspacesIcon ),
	LENTRY( TSeparatorIcon ),
	LENTRY( TShowDesktopIcon )
};

const int kStaticDockInstantiationListSize = 12;

BArchivable *instantiate_dock_object( BMessage *msg )
{
	BString classname;
	if ( msg->FindString( "class", &classname ) == B_OK )
	{
		for ( int i=0; i<kStaticDockInstantiationListSize; i++ )
		{
			if ( classname == kStaticDockInstantiationList[i].classname )
				return kStaticDockInstantiationList[i].instantiate( msg );
		}
	}

	return 0;
}


status_t IndependentLaunch( entry_ref &ref )
{
	BMessage message(B_REFS_RECEIVED);
	message.AddRef( "refs", &ref );
	return BMessenger( kTrackerSignature ).SendMessage( &message );
}
