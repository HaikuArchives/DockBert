/*
 * Copyright 2024 Nexus6 <nexus6@disroot.org>
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include <OutlineListView.h>

class TabListView : public BOutlineListView {
public:
							TabListView(const char* name, list_view_type type);
	virtual					~TabListView();

			void			SetDeleteMessage(BMessage* message);
			void			SetDragMessage(BMessage* message);
			void			SetGlobalDropTargetIndicator(bool isGlobal);

	virtual	void			Draw(BRect updateRect);
	virtual	bool 			InitiateDrag(BPoint point, int32 index,	bool wasSelected);
	virtual	void 			MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage);
	virtual void			MouseUp(BPoint point);
	virtual	void 			AttachedToWindow();
	virtual	void 			MessageReceived(BMessage* message);
	virtual	void			KeyDown(const char* bytes, int32 numBytes);

private:
			bool			_AcceptsDragMessage(const BMessage* message) const;

private:
			int32			fDropIndex;
			BRect			fDropTargetHighlightFrame;
			bool			fGlobalDropTargetIndicator;
			BMessage*		fDeleteMessage;
			BMessage*		fDragMessage;
};