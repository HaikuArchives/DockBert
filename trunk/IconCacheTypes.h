#ifndef ICONCACHETYPES_H
#define ICONCACHETYPES_H

enum IconDrawMode {
	// Difrent states of icon drawing
	idmNormal						= 0x00,
	idmSelected 					= 0x01,
	idmDimmed						= 0x10,		// inactive nav menu entry
	idmLink							= 0x40,		// symbolic link
	idmTrackerSpecialized			= 0x80,
};

// Where did an icon come from
enum IconSource {
	kUnknownSource,
	kUnknownNotFromNode,	// icon origin not known but determined not to be from
							// the node itself
	kTrackerDefault,		// file has no type, Tracker provides generic, folder,
							// symlink or app
	kTrackerSupplied,		// home directory, boot volume, trash, etc.
	kMetaMime,				// from BMimeType
	kPreferredAppForType,	// have a preferred application for a type, has an icon
	kPreferredAppForNode,	// have a preferred application for this node,
							// has an icon
	kVolume,
	kNode
};


#endif

