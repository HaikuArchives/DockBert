#include "Bubble.h"

#include <Window.h>
#include <Messenger.h>
#include <Application.h>
#include <Region.h>
#include <Autolock.h>
#include <Screen.h>
#include <Bitmap.h>

namespace TPrivate
{
	TBubbleHelpView::TBubbleHelpView( BRect rect )
		: BView( rect, 0, B_FOLLOW_ALL, B_WILL_DRAW )
	{
		//BFont font( be_bold_font );
		BFont font;
		font.SetSize( 12 );
		SetFont( &font );

		font_height fontHeight;
		font.GetHeight(&fontHeight);
		float fTitleAscent = ceilf(fontHeight.ascent);
		float fTitleDescent = ceilf(fontHeight.descent + fontHeight.leading);
		fLineHeight = 2 * fTitleAscent + fTitleDescent;

		//SetViewColor(B_TRANSPARENT_COLOR);
	}

	void TBubbleHelpView::SetLabel( const char *label )
	{
		int nlines = 0;
		const char *prevp = label;
		float label_width = 0;

		for ( ; *label; label ++ )
		{
			if ( *label == '\n' )
			{
				char thisline[256];
				strncpy( thisline, prevp, label - prevp );
				thisline[ label - prevp ] = 0;
				float thiswidth = StringWidth( thisline );
				if ( thiswidth > label_width )
					label_width = thiswidth;
				fLabel[nlines] = thisline;
				nlines ++;
				prevp = label+1;
			}
		}

		if ( !(*(label-2) == '\n') )
		{
			char thisline[256];
			strcpy( thisline, prevp );
			float thiswidth = StringWidth( thisline );
			if ( thiswidth > label_width )
				label_width = thiswidth;
			fLabel[nlines] = thisline;
			nlines ++;
		}

		fLabelRect.right = label_width;
		fLabelRect.bottom = nlines * fLineHeight;
		fNumberOfLines = nlines;
	}

	void TBubbleHelpView::Draw( BRect /* updateRect */ )
	{
		float w = Bounds().Width();
		float h = Bounds().Height();

		BRect rect(1, 1, w-2, h-2);

		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		SetHighColor(255, 255, 204, 180);
		FillRect(rect);

		SetHighColor(102,102,102,180);
		StrokeLine(BPoint(1,0), BPoint(w-2,0) );
		StrokeLine(BPoint(0,1), BPoint(0,h-2) );
		StrokeLine(BPoint(1,h-1), BPoint(w-1,h-1) );
		StrokeLine(BPoint(w-1,1), BPoint(w-1,h-2) );
		SetHighColor(0,0,0,180);
		StrokeLine(BPoint(w,2), BPoint(w,h-1) );
		StrokeLine(BPoint(2,h), BPoint(w-1,h) );
		StrokeLine(BPoint(w-1,h-1),BPoint(w-1,h-1));

		SetDrawingMode(B_OP_COPY);

		float hy = 0;

		SetLowColor(255,255,204);

		for (int i=0; i<fNumberOfLines; i++ )
		{
			DrawString( fLabel[i].String(), BPoint( (w-StringWidth(fLabel[i].String()))/2, hy+5+fLineHeight/2 ) );
			hy += fLineHeight;
		}
	}

	void TBubbleHelpView::MessageReceived( BMessage *msg )
	{
		// todo: do opening animation
		switch ( msg->what )
		{
		default:
			BView::MessageReceived(msg);
		}
	}

};

TBubbleHelp::TBubbleHelp()
{
	BubbleInit();
}

TBubbleHelp::~TBubbleHelp()
{
	if ( fBubbleWindow->Lock() )
	{
		fBubbleWindow->PostMessage(B_QUIT_REQUESTED);
		fBubbleWindow->Unlock();
	}
}

void TBubbleHelp::AddBubble( TBubbleTarget *target )
{
	BAutolock( &fTargetList.lock );
	fTargetList.AddItem(target);
}

void TBubbleHelp::DeleteBubble( TBubbleTarget *target )
{
	BAutolock( &fTargetList.lock );
	fTargetList.RemoveItem(target);
}

TBubbleTarget *TBubbleHelp::TargetAt( BPoint where )
{
	TBubbleTarget *target = 0;

	int32 count = fTargetList.CountItems();
	for ( int32 i=0; i<count; i++ )
	{
		TBubbleTarget *t = (TBubbleTarget*)fTargetList.ItemAt(i);
		if ( t->Touches( where ) )
		{
			target = t;
			break;
		}
	}

	return target;
}

void TBubbleHelp::ShowBubble( BPoint where, TBubbleTarget *target )
{
	if ( !target )
		return;

	if ( fBubbleWindow->Lock() )
	{
		HideBubble();

		const char *bubbletext = target->BubbleText();
		if ( !bubbletext )
		{
			fBubbleWindow->Unlock();
			return;
		}

		fBubbleHelpView->SetLabel( bubbletext );
		BRect r = fBubbleHelpView->LabelRect();
		fBubbleWindow->ResizeTo( r.Width()+10, r.Height() +3);
//		fBubbleWindow->MoveTo( where.x - r.Width()/2, where.y - r.Height() - 10);
		fBubbleWindow->MoveTo( where.x - 5, where.y - r.Height() - 5);

		fBubbleWindow->Show();

		fBubbleWindow->Unlock();
	}
}

void TBubbleHelp::HideBubble()
{
	if ( fBubbleWindow->Lock() )
	{
		if ( !fBubbleWindow->IsHidden() )
			fBubbleWindow->Hide();
		fBubbleWindow->Unlock();
	}
}

float TBubbleHelp::BubbleHeight(TBubbleTarget *target)
{
	const char *label = target->BubbleText();
	if (!label)
		return 0;

	int nlines = 0;

	for ( ; *label; label ++ )
	{
		if ( *label == '\n' )
		{
			nlines ++;
		}
	}

	if (nlines == 0)
		nlines ++;

	BFont font( be_bold_font );
	font.SetSize( 12 );

	font_height fontHeight;
	font.GetHeight(&fontHeight);
	float fTitleAscent = ceilf(fontHeight.ascent);
	float fTitleDescent = ceilf(fontHeight.descent + fontHeight.leading);
	float fLineHeight = fTitleAscent + fTitleDescent;

	return nlines * fLineHeight + 4;
}

void TBubbleHelp::BubbleInit()
{
	fBubbleWindow = new BWindow( BRect( -100, -100, -50, -50 ), "Bubbles.. hm.. yummy", B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL, B_AVOID_FOCUS|B_NOT_MOVABLE, B_ALL_WORKSPACES );
	fBubbleHelpView = new TPrivate::TBubbleHelpView( BRect( 0, 0, 50, 50 ) );
	fBubbleWindow->AddChild(fBubbleHelpView);
	fBubbleWindow->Run();
	fBubbleWindow->Activate(false);
}
