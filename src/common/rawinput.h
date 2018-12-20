#include <windows.h>
#include <setupapi.h>
#include "hidapi.h"

namespace common{ namespace rawinput{
	typedef void (*pParseRawInput)(PRAWINPUT pRawInput);

	int Initialize(void *hWnd);
	void Uninitialize();

	void RegisterCallback(pParseRawInput cb);
	void UnregisterCallback(pParseRawInput cb);
}};