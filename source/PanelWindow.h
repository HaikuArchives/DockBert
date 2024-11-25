#ifndef PANEL_WINDOW_H
#define PANEL_WINDOW_H

#define BUILDSTATUS		"development"
#define BUILDVERSION	"1.1.0"
#define BUILDNR			"111"
#define BUILDDATE		__DATE__ " " __TIME__
#define THREAD_NAME		"dock_thread"

#include <View.h>
#include <Window.h>
#include <String.h>
#include <List.h>
#include <Archivable.h>
#include <Entry.h>
#include <Locker.h>
#include <Messenger.h>
#include "PreferencesWindow.h"

class TInnerPanel;
class TPanelIcon;
class TRaisingIconPanel;
class TApplicationPanel;
class TAppPanelIcon;
class TShortcutPanel;
class TPossibleApplicationIcon;

const int kPanelWindowViewTimer = 'pvtm';
const int kPanelWindowHidderTimer = 'pwht';

const int kPrepareWindowHidder = 'kpwh';

const int kIconHiddingSpeed = 5500;
//const int kIconHiddingSpeed = 10000;

#define _ICON_SPACING	16
#define _G_ICON_SPACING (16 + _ICON_SPACING)

class BMessageRunner;

class TPanelWindowView;

// 0.0046f, 0.0066f, 0.0088f, 0.011f, 0.014f, 0.018f, 0.021f, 0.025f, 0.030f, 0.036f, 0.041f, 0.048f, 0.057f, 0.066f, 0.076f, 0.087f,
const float kWindowHidderFrames[] = { 0.0f, 0.0125f, 0.025f, 0.05f, 0.065f, 0.075f, 0.085f, 0.1f, 0.116f, 0.132f, 0.152f, 0.174f, 0.199f, 0.227f, 0.260f, 0.3f, 0.336f, 0.38f, 0.435f, 0.5f, 0.56f, 0.64f, 0.73f, 0.83f, 0.94f, 1.0f };
const int kWindowHidderNumberFrames = (sizeof(kWindowHidderFrames) / sizeof(float)) -1;
const float kWindowHidderSpeed = 200000.f;
const int kWindowHidderAnimationSpeed = (int) (kWindowHidderSpeed / (float)kWindowHidderNumberFrames);

const int kWindowHidderAnimationStaleSpeed = 1600000;

class TMessageRunner
{
public:
	TMessageRunner( BMessenger, BMessage *, bigtime_t, int count=-1 );
	~TMessageRunner();

	status_t InitCheck();
	void SetInterval( bigtime_t );

private:
	void Loop();
	static int32 _thread_instance( void * );

	BMessenger fMessenger;
	BMessage *fMessage;
	bigtime_t fInterval;
	int fCount;
	thread_id fThreadID;
	BLocker lock;
};

#define MESSAGERUNNER	TMessageRunner

namespace TPrivate
{
	class TTimePipe
	{
	private:
		struct Titem
		{
			bigtime_t data;
			Titem *next;
		};
		Titem *fFirst, *fLast;
	public:
		TTimePipe() : fFirst(0), fLast(0) {}

		void Push( bigtime_t time )
		{
			Titem *item = new Titem;
			if ( fLast )
				fLast->next = item;
			fLast = item;
			item->data = time;
			item->next = 0;
			if ( !fFirst )
				fFirst = fLast;
		}

		bigtime_t Pop()
		{
			if (!fFirst)
				return -1;
			Titem *item = fFirst;
			fFirst = item->next;
			bigtime_t time = item->data;
			delete item;
			if ( !fFirst )
				fLast = 0;
			return time;
		}
	};
};

class TPanelWindow : public BWindow {
public:
	TPanelWindow();
	virtual ~TPanelWindow();

	void MessageReceived( BMessage * );
	void FrameResized(float width, float height);
	void WorkspaceActivated( int32 , bool);
	virtual BHandler *ResolveSpecifier( BMessage *, int32, BMessage *, int32, const char * );
	virtual status_t GetSupportedSuites( BMessage * );

	virtual void ScreenChanged( BRect frame, color_space mode );

	void CenterCorrectly();
	void ShowHide(bool);
	void SaveSettings();

	void ForceHide();

	void DockChangedPlaces( int32 );

	void SetAutoHide( bool );
	void SetHideEffectDelay( int );

	bool IsAutoHiding() const { return fAutoHide; }
	int CurrentHideEffectDelay() const { return fHideEffectDelay; }

	void SetMaximumHeight( float );
	void SetShowHideLimit( float );

private:
	bool DoShowHideFrame();

	TPanelWindowView *fTeamListView;
	bool fIsVisible;
	int fHidderAnimationCurrentFrame;
	MESSAGERUNNER *fHidderTimer;
	TPrivate::TTimePipe fHiddingPipe;
	bool fAlmostHidding;
	float fMaximumHeight;
	int fLastShowHideFrame;

	bool fAutoHide;
	int fHideEffectDelay;
	void ShowPreferencesWindow();
    PreferencesWindow* fPreferencesWindow;
};

#endif /* PANEL_WINDOW_H */
