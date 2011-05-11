#include "NMenu.h"

#include <Bitmap.h>

const float	kHPad = 10.0f;
const float	kVPad = 2.0f;
const float	kLabelOffset = 8.0f;

TBitmapMenuItem::TBitmapMenuItem( const char *label, BBitmap *bitmap, BMessage *message )
	: TMenuItem( label, message ), fBitmap( bitmap ), fBitmapAutoDestruct( true )
{
	fBitmapDeslX = 0;
	fBitmapDeslY = 3;
	if ( fBitmap )
	{
		int addWidth = 6;
		if (strcmp(label, "") == 0)
			addWidth = 2;
		fBitmapDeslX = fBitmap->Bounds().Width() + addWidth;
	}
}

TBitmapMenuItem::TBitmapMenuItem( TMenu *submenu, BBitmap *bitmap, BMessage *message )
	: TMenuItem( submenu, message ), fBitmap( bitmap ), fBitmapAutoDestruct( true )
{
	fBitmapDeslX = 0;
	fBitmapDeslY = 3;
	if ( fBitmap )
	{
		fBitmapDeslX = fBitmap->Bounds().Width() + 6;
	}
}

TBitmapMenuItem::~TBitmapMenuItem()
{
	if ( fBitmapAutoDestruct )
		if ( fBitmap )
			delete fBitmap;
}

void TBitmapMenuItem::GetContentSize( float *width, float *height )
{
	TMenuItem::GetContentSize( width, height );
	*width += fBitmapDeslX;
	if ( fBitmap && *height < fBitmap->Bounds().Height() )
		*height = fBitmap->Bounds().Height();
	*height += fBitmapDeslY;

	BFont font;
	Menu()->GetFont(&font);
	font_height fontHeight;
	font.GetHeight(&fontHeight);
	fTitleAscent = ceilf(fontHeight.ascent);
	fTitleDescent = ceilf(fontHeight.descent + fontHeight.leading);
}

void TBitmapMenuItem::DrawContent()
{
	TMenu *menu = Menu();
	BRect frame(Frame());
	
	BPoint drawPoint(ContentLocation());
	drawPoint.x += fBitmapDeslX + ( fBitmap ? ( fBitmap->Bounds().Width() > 16 ? - 8 : 0 ) : 0 );
	drawPoint.y += (frame.Height()/2) - (fTitleAscent - fTitleDescent);
	menu->MovePenTo(drawPoint);
	TMenuItem::DrawContent();
	
	if (fBitmap)
	{
		BPoint where(ContentLocation());
		where.y = frame.top + ( ( frame.Height() - fBitmap->Bounds().Height() ) / 2) -1;

		if ( fBitmap->Bounds().Width() > 16 )
			where.x -= 8;

		menu->SetDrawingMode( B_OP_ALPHA );
		if ( !IsEnabled() )
		{
			menu->SetBlendingMode( B_CONSTANT_ALPHA, B_ALPHA_OVERLAY );
			menu->SetHighColor( 255, 255, 255, 30 );
		}
		else
		{
			menu->SetHighColor( 255, 255, 255 );
			menu->SetBlendingMode( B_PIXEL_ALPHA, B_ALPHA_OVERLAY );
		}

		menu->DrawBitmap(fBitmap, where);

		menu->SetHighColor( 0, 0, 0);
		menu->SetDrawingMode( B_OP_COPY );
	}
}

void TBitmapMenuItem::TruncateLabel( float maxWidth, char *newLabel )
{
	TMenuItem::TruncateLabel( maxWidth - ( fBitmap ? ( fBitmap->Bounds().Width() - 8 ) : 0 ), newLabel );
}

void TBitmapMenuItem::SetBitmapAutoDestruct( bool state )
{
	fBitmapAutoDestruct = state;
}

bool TBitmapMenuItem::BitmapAutoDestruct() const
{
	return fBitmapAutoDestruct;
}

TAwarePopupMenu::TAwarePopupMenu( const char *label )
	: TPopupMenu(label)
{
/*	BFont font;
	GetFont(&font);
	font.SetSize(10.f);
	SetFont(&font);*/
	SetFont(be_plain_font);
}

TAwarePopupMenu::~TAwarePopupMenu()
{
	BMessage msg(T_MENU_CLOSED);
	msg.AddPointer( "source", this );
	fMessenger.SendMessage( &msg );
}

void TAwarePopupMenu::SetTargetMessenger( BMessenger messenger )
{
	fMessenger = messenger;
}
