#ifndef DOCK_ARCHIVABLE_UTILS_H
#define DOCK_ARCHIVABLE_UTILS_H

#include <Entry.h>
#include <NodeInfo.h>

class BArchivable;
class BMessage;

class BBitmap;

BArchivable *instantiate_dock_object( BMessage *msg );
status_t IndependentLaunch( entry_ref& );

#define INSTANTIATE_OBJECT(x) \
	static BArchivable *Instantiate(BMessage *from) \
	{ \
		if ( validate_instantiation(from, #x)) \
			return new x(from); \
		return NULL; \
	}

#endif
