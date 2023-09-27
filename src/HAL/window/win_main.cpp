
#include"../application.h"


#if 1

using namespace ar3d;

HINSTANCE GWinInstance;

i32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR Cmd, i32 nCmdShow)
{
	GWinInstance = hInstance;
	i32 argc = 0;
	LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

	ar_open_leak_check();
	ar_dump_leak();


	bool bAllocateConsole = !::AttachConsole(ATTACH_PARENT_PROCESS) && ::IsDebuggerPresent();
	if (bAllocateConsole) {
		AllocConsole();
		AttachConsole(ATTACH_PARENT_PROCESS);
		FILE* fDummy;
		freopen_s(&fDummy, "CONIN$", "r", stdin);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
		printf("allocate console");
	}
	
	
	for (i32 i = 0; i < argc; ++i)
	{
		char buf[2048];
		wcstombs(buf, argv[i], 2048);
		StringView param{ buf };
		if (i == 0) {
			param.strip().findLastRemove("\\");
			GAppConfigs.set(AppConfigs::InnerDataDir, String{ param });
			GAppConfigs.set(AppConfigs::ExternalDataDir, String{ param });
			GAppConfigs.set(AppConfigs::TempDir, String{param});
		}
		else {
			param.stripLeft().startsThenRemove('-');
			if (!param.empty()) {
				auto pair = param.splitToTwo('=');
				if (pair.second.size())
				{
					String key{ pair.first.strip() };
					String second{ pair.second.strip() };
					GAppConfigs.set(std::move(key), std::move(second));
				}
				else {
					String key{ param.strip() };
					GAppConfigs.addSwitch(key);
				}
			}
		}
	}

	GAppConfigs.set(AppConfigs::AppHandle, (i64)hInstance);
	GAppConfigs.set(AppConfigs::Commandline, std::string(Cmd));
	GAppConfigs.set(AppConfigs::WinWidth, 1000);
	GAppConfigs.set(AppConfigs::WinHeight, 800);
	GAppConfigs.set(AppConfigs::WinResizable, true);
	GAppConfigs.set(AppConfigs::VSync, false);

	bool code = GuardMain();
	if(bAllocateConsole) FreeConsole();
	return code;
}


#endif 