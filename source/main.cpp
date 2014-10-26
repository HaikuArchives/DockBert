#include "PanelWindow.h"

#include <app/Application.h>

int main()
{
	BApplication app("application/x-vnd.Dockbert");
	TPanelWindow* main = new TPanelWindow;
	main->Show();
	app.Run();
	return 0;	
}
