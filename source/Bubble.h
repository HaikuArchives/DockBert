#ifndef BUBBLE_H
#define BUBBLE_H

#include <OS.h>
#include <Rect.h>
#include <String.h>
#include <View.h>

#include "LockingList2.h"

class BWindow;

namespace TPrivate
{
	class TBubbleHelpView : public BView
	{
	public:
		TBubbleHelpView( BRect rect );

		void SetLabel( const char *label );
		BRect LabelRect() const { return fLabelRect; }

		virtual void Draw( BRect updateRect );
		virtual void MessageReceived( BMessage *msg );

	private:
		BString fLabel[16];
		int fNumberOfLines;
		float fLineHeight;
		BRect fLabelRect;
	};
};

class TBubbleTarget
{
public:
	TBubbleTarget() {}
	virtual ~TBubbleTarget() {}

	virtual const char *BubbleText() const { return 0; }
	virtual bool Touches( BPoint ) const { return false; }
	virtual BRect BubbleTargetRect() const { return BRect(0,0,0,0); }
	virtual bool DeleteMe() const { return true; }
};

class TBubbleHelp
{
public:
	TBubbleHelp();
	virtual ~TBubbleHelp();

	void AddBubble( TBubbleTarget * );
	void DeleteBubble( TBubbleTarget * );

	TBubbleTarget *TargetAt( BPoint );

	void ShowBubble( BPoint, TBubbleTarget * );
	void HideBubble();

	static float BubbleHeight(TBubbleTarget *);

private:
	void BubbleInit();

	BWindow *fBubbleWindow;
	TPrivate::TBubbleHelpView *fBubbleHelpView;

	LockingList2<TBubbleTarget> fTargetList;
};

#endif
