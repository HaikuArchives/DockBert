#ifndef OFFSCREEN_VIEW_H
#define OFFSCREEN_VIEW_H

#include <View.h>
#include <Bitmap.h>
#include <Locker.h>

class TOffscreenView : public BView
{
public:
	TOffscreenView( BRect rect )
		: BView( rect, "TOffscreenView", B_FOLLOW_ALL, B_WILL_DRAW )
	{
		fBitmapBuffer = new BBitmap( rect, B_RGBA32, true );
		fBitmapBuffer->AddChild( this );
	}

	virtual ~TOffscreenView()
	{
		fBitmapBuffer->RemoveChild(this);
		delete fBitmapBuffer;
	}

	void SetBackBufferSize( float width, float height )
	{
		fLock.Lock();

		fBitmapBuffer->Lock();
		ResizeTo( width, height );
		fBitmapBuffer->Unlock();

		BRect r = fBitmapBuffer->Bounds();
		if ( width > r.Width() )
			r.right *= 2;
		if ( height > r.Height() )
			r.bottom *= 2;
		if ( r != fBitmapBuffer->Bounds() )
		{
			BBitmap *tmp = new BBitmap( r, B_RGBA32, true );
			fBitmapBuffer->RemoveChild( this );
			tmp->AddChild( this );

			tmp->Lock();
			DrawBitmap( fBitmapBuffer );
			tmp->Unlock();

			delete fBitmapBuffer;

			fBitmapBuffer = tmp;
		}

		fLock.Unlock();
	}

	inline void Lock()
	{
		fLock.Lock();
		fBitmapBuffer->Lock();
	}

	inline void Unlock()
	{
		Sync();
//		Flush();
		fBitmapBuffer->Unlock();
		fLock.Unlock();
	}

	BBitmap *OffscreenBuffer() const { return fBitmapBuffer; }

protected:
	BBitmap *fBitmapBuffer;
	BLocker fLock;
};

#endif
