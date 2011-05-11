#include "PanelWindow.h"

#include <app/Application.h>

int main()
{
	BApplication app("application/x-vnd.BeClan-Dock");
	TPanelWindow* main = new TPanelWindow;
	main->Show();
	app.Run();
	return 0;	
}
