
#include"win_application.h"


#if 1

using namespace ar3d;

i32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, i32 nCmdShow)
{
	GInstance = hInstance;
	i32 argc = 0;
	LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

	if (!::AttachConsole(ATTACH_PARENT_PROCESS) && ::IsDebuggerPresent()) {
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
		char* buf = new char[2048];
		wcstombs(buf, argv[i], 2048);
		//GCmdLines.push_back(buf);
	}
	return Application::GuardMain();
}


#endif 