#ifndef NMENU_H
#define NMENU_H

#include <interface/Menu.h>
#include <support/String.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

typedef BMenu TMenu;
typedef BMenuItem TMenuItem;
typedef BPopUpMenu TPopupMenu;

class BBitmap;
class BMessage;

const uint32 T_MENU_CLOSED	= 'mcls';

class TBitmapMenuItem : public TMenuItem
{
public:
	TBitmapMenuItem( const char *label, BBitmap *bitmap, BMessage * = 0);
	TBitmapMenuItem( TMenu *submenu, BBitmap *bitmap, BMessage * = 0);
	virtual ~TBitmapMenuItem();

	virtual void GetContentSize( float *width, float *height );
	virtual void DrawContent();

	virtual void TruncateLabel( float maxWidth, char *newLabel );

	void SetBitmapAutoDestruct( bool state );
	bool BitmapAutoDestruct() const;

protected:
	BBitmap *fBitmap;
	float fBitmapDeslX, fBitmapDeslY;

private:
	bool fBitmapAutoDestruct;
	float fTitleAscent;
	float fTitleDescent;
};

class TAwarePopupMenu : public TPopupMenu
{
public:
	TAwarePopupMenu( const char *label );
	virtual ~TAwarePopupMenu();

	void SetTargetMessenger( BMessenger );

private:
	BMessenger fMessenger;
};

#endif
