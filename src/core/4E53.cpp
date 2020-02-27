#include <Windows.h>
#include "../util/settings.h"
#include "../util/serviceprovider.h"

#define SETTINGS_FILE "cfg/settings.xml"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	/*create logger*/
	std::shared_ptr<Logger<VSLogPolicy>> vsLogger(new Logger<VSLogPolicy>(L""));
	vsLogger->setThreadName("mainThread");
	ServiceProvider::setVSLoggingService(vsLogger);

	ServiceProvider::getVSLogger()->print<Severity::Info>("Logger started successfully.");


	/*load settings file*/
	SettingsLoader settingsLoader;

	if (!settingsLoader.loadSettings(SETTINGS_FILE))
	{
		ServiceProvider::getVSLogger()->print<Severity::Error>("Failed to load settings.xml!");
		return -1;
	}
	ServiceProvider::setSettings(settingsLoader.get());

	ServiceProvider::getVSLogger()->print<Severity::Info>("Settings file loaded successfully.");

	/*initialize main window and directx12*/





	return 0;
}