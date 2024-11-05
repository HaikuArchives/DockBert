/*
 * Copyright 2024 Nexus6 <nexus6@disroot.org>
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include "PreferencesWindow.h"
#include "Preferences.h"
#include "TabListView.h"

#include <Window.h>

class BButton;
class BCheckBox;
class BColorControl;
class BListView;
class BRadioButton;
class BSpinner;
class BScrollView;
class BStringView;
class BTabView;
class BTextControl;


static const int kTimeout = 1000000;

static const uint32 kMsgAlwaysOnTop = 'alot';
static const uint32 kMsgAutoHide = 'auhi';
static const uint32 kMsgDrawOuterFrame = 'drof';
static const uint32 kMsgHideEffectDelay = 'hied';
static const uint32 kMsgBackgroundColor = 'bgco';
static const uint32 kMsgTabColor = 'taco';
static const uint32 kMsgChangeTabName = 'tana';
static const uint32 kMsgModifyName = 'mona';
static const uint32 kMsgAddTab = 'adta';
static const uint32 kMsgRemoveTab = 'reta';
static const uint32 kMsgSelectTab = 'seta';
static const uint32 kMsgOuterFrameColor = 'ofco';
static const uint32 kMsgPreferencesChangedFromOutside = 'scfo';
static const uint32 kMsgItemDragged = 'itdr';


class PreferencesWindow : public BWindow
{
public:
						PreferencesWindow(BRect frame, BView* mainView);
						~PreferencesWindow();

	virtual void		MessageReceived(BMessage* message);
	virtual void		WindowActivated(bool active);
	virtual bool		QuitRequested();

private:
	void				_LoadSettings();
	void				_InitControls();
	void				_SetWatch(bool watch);

	bool				fLoadSettings;
	Preferences*		fPreferences;
	int32				fDebugLevel;

	BColorControl*		fBackgroundColorControl;
	BColorControl*		fOuterFrameColorControl;
	BCheckBox*			fAlwaysOnTopControl;
	BCheckBox*			fAutoHideControl;
	BCheckBox*			fDrawOuterFrameControl;
	BSpinner*			fHideEffectDelayControl;

	TabListView*		fTabListView;
	BScrollView* 		fScrollListView;
	BButton*			fAddTabButton;
	BButton*			fRemoveTabButton;
	BColorControl*		fTabColorControl;
	BTextControl*		fTabNameControl;

	BView*				fGeneralPrefsView;
	BView*				fTabPrefsView;
	BTabView*			fTabView;

	BMessageRunner*		fMessageRunner;

	BView*			fMainView;
};