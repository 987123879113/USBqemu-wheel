#define _WIN32_WINNT  0x0501 //Why are you on XP?
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdlib.h>

#include <algorithm>
#include <map>
#include "../../USB.h"
#include "raw-config.h"
#include "raw-config-res.h"

extern HINSTANCE hInst;
extern char *szIni;
extern std::wstring szIniDir;
extern void GetIniFile(std::wstring &iniFile);
#define MSG_PRESS_ESC(wnd) SendDlgItemMessageW(wnd, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)L"Capturing, press ESC to cancel")

inline bool MapExists(MapVector *maps, TCHAR* hid)
{
	MapVector::iterator it;
	for(it = maps->begin(); it != maps->end(); it++)
		if(!it->hidPath.compare(hid))
			return true;
	return false;
}

void LoadMappings(MapVector *maps)
{
	std::wstring szIniFile;

	GetIniFile(szIniFile);

	maps->clear();
	
	TCHAR sec[32] = {0}, tmp[16] = {0}, bind[32] = {0}, hid[MAX_PATH+1];
	int j = 0, v = 0;
	while(j < 25)
	{
		swprintf_s(sec, L"DEVICE %d", j++);
		if(GetPrivateProfileString(sec, L"HID", NULL, hid, MAX_PATH, szIniFile.c_str())
			&& hid && !MapExists(maps, hid))
		{
			Mappings m;// = new Mappings;
			ZeroMemory(&m, sizeof(Mappings));
			maps->push_back(m);
			Mappings *ptr = &maps->back();

			ptr->hidPath = hid;
			ptr->devName = hid;
			//ResetData(&ptr->data[0]);
			//ResetData(&ptr->data[1]);
			memset(&ptr->data[0], 0xFF, sizeof(wheel_data_t));
			memset(&ptr->data[1], 0xFF, sizeof(wheel_data_t));
			ptr->data[0].buttons = 0;
			ptr->data[1].buttons = 0;
			ptr->data[0].hatswitch = 0x8; //memset to 0xFF already or set to -1
			ptr->data[1].hatswitch = 0x8;

			for(int i = 0; i<MAX_BUTTONS; i++)
			{
				swprintf_s(bind, L"Button %d", i);
				if(GetPrivateProfileString(sec, bind, NULL, tmp, 16, szIniFile.c_str()))
					swscanf_s(tmp, L"%08X", &(ptr->btnMap[i]));
			}

			for(int i = 0; i<MAX_AXES; i++)
			{
				swprintf_s(bind, L"Axis %d", i);
				if(GetPrivateProfileString(sec, bind, NULL, tmp, 16, szIniFile.c_str()))
					swscanf_s(tmp, L"%08X", &ptr->axisMap[i]);
			}

			for(int i = 0; i<4/*PAD_HAT_COUNT*/; i++)
			{
				swprintf_s(bind, L"Hat %d", i);
				if(GetPrivateProfileString(sec, bind, NULL, tmp, 16, szIniFile.c_str()))
					swscanf_s(tmp, L"%08X", &ptr->hatMap[i]);
			}
			ptr = NULL;
		}
	}

	GetPrivateProfileString(L"Joystick", L"Player1", NULL, hid, MAX_PATH, szIniFile.c_str());
	player_joys[0] = hid;

	GetPrivateProfileString(L"Joystick", L"Player2", NULL, hid, MAX_PATH, szIniFile.c_str());
	player_joys[1] = hid;
	return;
}

void SaveMappings(MapVector *maps)
{
	std::wstring szIniFile;
	char szValue[256] = {0};

	GetIniFile(szIniFile);

	MapVector::iterator it;
	uint32_t numDevice = 0;
	for(it = maps->begin(); it != maps->end(); it++)
	{
		TCHAR dev[32] = {0}, tmp[16] = {0}, bind[32] = {0};

		swprintf_s(dev, L"DEVICE %d", numDevice++);
		WritePrivateProfileString(dev, L"HID", it->hidPath.c_str(), szIniFile.c_str());

		//writing everything separately, then string lengths are more predictable
		for(int i = 0; i<MAX_BUTTONS; i++)
		{
			swprintf_s(bind, L"Button %d", i);
			swprintf_s(tmp, L"%08X", it->btnMap[i]);
			WritePrivateProfileString(dev, bind, tmp, szIniFile.c_str());
		}

		for(int i = 0; i<MAX_AXES; i++)
		{
			swprintf_s(bind, L"Axis %d", i);
			swprintf_s(tmp, L"%08X", it->axisMap[i]);
			WritePrivateProfileString(dev, bind, tmp, szIniFile.c_str());
		}

		for(int i = 0; i<4/*PAD_HAT_COUNT*/; i++)
		{
			swprintf_s(bind, L"Hat %d", i);
			swprintf_s(tmp, L"%08X", it->hatMap[i]);
			WritePrivateProfileString(dev, bind, tmp, szIniFile.c_str());
		}
	}

	WritePrivateProfileString(L"Joystick", L"Player1", player_joys[0].c_str(), szIniFile.c_str());
	WritePrivateProfileString(L"Joystick", L"Player2", player_joys[1].c_str(), szIniFile.c_str());
	return;
}

/// Dialogs
#define TXT(x) (#x)
char *BTN2TXT[] = { 
	"Cross",
	"Square",
	"Circle",
	"Triangle",
	"R1",
	"L1",
	"R2",
	"L2",
	"Select",
	"Start",
	"R3",
	"L3"
};

char *AXIS2TXT[] = { 
	"Axis X",
	"Axis Y",
	"Axis Z",
	//"Axis RX",
	//"Axis RY",
	"Axis RZ",
	"Hat Switch"
};

void resetState(HWND hW);
HWND dgHwnd = NULL;
HINSTANCE hInst;
//std::vector<std::wstring> joysName;
std::vector<std::wstring> joysDev;
DWORD selectedJoy[2];
//std::vector<std::string>::iterator* tmpIter;

typedef struct _DevInfo
{
	int ply;
	RID_DEVICE_INFO_HID hid;

	bool operator==(const _DevInfo &t) const{
		if(ply == t.ply && hid.dwProductId == t.hid.dwProductId &&
			hid.dwVendorId == t.hid.dwVendorId &&
			hid.dwVersionNumber == t.hid.dwVersionNumber &&
			hid.usUsage == t.hid.usUsage &&
			hid.usUsagePage == t.hid.usUsagePage)
			return true;
		return false;
	}

	bool operator<(const _DevInfo &t) const{
		if(ply < t.ply) return true;
		if(hid.dwProductId < t.hid.dwProductId) return true;
		if(hid.dwVendorId < t.hid.dwVendorId) return true;
		if(hid.dwVersionNumber < t.hid.dwVersionNumber) return true;
		return false;
	}

} DevInfo_t;

//typedef std::map<const DevInfo_t, Mappings_t> MappingsMap;
//MappingsMap mappings;

uint32_t  axisDiff[MAX_AXES]; //previous axes values
bool      axisPass2 = false;

//eh, global var for currently selected player
static int plyCapturing = 0;
PS2Buttons btnCapturing  = PAD_BUTTON_COUNT;
PS2Axis    axisCapturing = PAD_AXIS_COUNT;
PS2HatSwitch hatCapturing = PAD_HAT_COUNT;

void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf_s(tmp,512,fmt,list);
	va_end(list);
	MessageBoxA(0, tmp, "Qemu USB Msg", 0);
}

void populate(HWND hW)
{
	//mappings.clear();
	//joysName.clear();
	//joysName.push_back("None");
	joysDev.clear();
	joysDev.push_back(L"");

	int i=0, sel_idx=1;
	HANDLE usbHandle = INVALID_HANDLE_VALUE;
	DWORD needed=0;
	HDEVINFO devInfo;
	GUID guid;
	SP_DEVICE_INTERFACE_DATA diData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA didData = NULL;
	HIDD_ATTRIBUTES attr;
	PHIDP_PREPARSED_DATA pPreparsedData = NULL;
	HIDP_CAPS caps;
	OVERLAPPED ovl;

	memset(&ovl, 0, sizeof(OVERLAPPED));
	ovl.hEvent = CreateEvent(0, 0, 0, 0);
	ovl.Offset = 0;
	ovl.OffsetHigh = 0;

	HidD_GetHidGuid(&guid);

	devInfo = SetupDiGetClassDevs(&guid, 0, 0, DIGCF_DEVICEINTERFACE);
	if(!devInfo) return;

	diData.cbSize = sizeof(diData);

	//Mappings listview
	LVCOLUMN LvCol;
	memset(&LvCol,0,sizeof(LvCol));
	LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	LvCol.pszText=TEXT("Device");
	LvCol.cx=0x4F;
	SendDlgItemMessage(hW, IDC_LIST1, LVM_SETEXTENDEDLISTVIEWSTYLE,
		0,LVS_EX_FULLROWSELECT); // Set style
	ListView_InsertColumn(GetDlgItem(hW, IDC_LIST1), 0, &LvCol);

	LvCol.pszText=TEXT("PC");
	ListView_InsertColumn(GetDlgItem(hW, IDC_LIST1), 1, &LvCol);

	LvCol.pszText=TEXT("PS2");
	ListView_InsertColumn(GetDlgItem(hW, IDC_LIST1), 2, &LvCol);

	//Tab control
	TCITEM tie;
	tie.pszText = TEXT("Player 1");
	tie.cchTextMax = 32;
	tie.mask = TCIF_TEXT;
	SendDlgItemMessage(hW, IDC_TAB1, TCM_INSERTITEM, 0, (LPARAM)&tie);

	tie.pszText = TEXT("Player 2");
	SendDlgItemMessage(hW, IDC_TAB1, TCM_INSERTITEM, 1, (LPARAM)&tie);

	//SendDlgItemMessageA(hW, IDC_COMBO_WHEEL_TYPE, CB_ADDSTRING, 0, (LPARAM)"DF / Generic Logitech Wheel");
	////SendDlgItemMessageA(hW, IDC_COMBO_WHEEL_TYPE, CB_ADDSTRING, 0, (LPARAM)"Driving Force");
	//SendDlgItemMessageA(hW, IDC_COMBO_WHEEL_TYPE, CB_ADDSTRING, 0, (LPARAM)"Driving Force Pro");
	//SendDlgItemMessage(hW, IDC_COMBO_WHEEL_TYPE, CB_SETCURSEL, conf.WheelType1, 0);
	////SendDlgItemMessageA(hW, IDC_COMBO_WHEEL_TYPE, CB_ADDSTRING, 0, (LPARAM)"Driving Force GT");

	//Selected FFB target device
	SendDlgItemMessageA(hW, IDC_COMBO_FFB, CB_ADDSTRING, 0, (LPARAM)"None");
	SendDlgItemMessage(hW, IDC_COMBO_FFB, CB_SETCURSEL, 0, 0);

	while(SetupDiEnumDeviceInterfaces(devInfo, 0, &guid, i, &diData)){
		if(usbHandle != INVALID_HANDLE_VALUE)
			CloseHandle(usbHandle);

		SetupDiGetDeviceInterfaceDetail(devInfo, &diData, 0, 0, &needed, 0);

		didData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(needed);
		didData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		if(!SetupDiGetDeviceInterfaceDetail(devInfo, &diData, didData, needed, 0, 0)){
			free(didData);
			break;
		}

		usbHandle = CreateFile(didData->DevicePath, GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

		if(usbHandle == INVALID_HANDLE_VALUE){
			fprintf(stderr,"Could not open device %i\n", i);
			free(didData);
			i++;
			continue;
		}

		HidD_GetAttributes(usbHandle, &attr);
		HidD_GetPreparsedData(usbHandle, &pPreparsedData);
		HidP_GetCaps(pPreparsedData, &caps);

		if(caps.UsagePage == HID_USAGE_PAGE_GENERIC && 
			caps.Usage == HID_USAGE_GENERIC_JOYSTICK)
		{
			fprintf(stderr, "Joystick found %04X:%04X\n", attr.VendorID, attr.ProductID);
			std::wstring strPath(didData->DevicePath);
			std::transform(strPath.begin(), strPath.end(), strPath.begin(), ::toupper);
			joysDev.push_back(strPath);

			wchar_t str[MAX_PATH+1];
			HidD_GetProductString(usbHandle, str, sizeof(str));//TODO HidD_GetProductString returns unicode always?
			SendDlgItemMessageW(hW, IDC_COMBO_FFB, CB_ADDSTRING, 0, (LPARAM)str);
			//joysName.push_back(str);
			//SendDlgItemMessageW(hW, IDC_COMBO2, CB_ADDSTRING, 0, (LPARAM)str);

			if(player_joys[0] == strPath)
			{
				SendDlgItemMessage(hW, IDC_COMBO_FFB, CB_SETCURSEL, sel_idx, 0);
				selectedJoy[0] = sel_idx;
			}
			else if(player_joys[1] == strPath)
			{
				selectedJoy[1] = sel_idx;
			}
			sel_idx++;
		}
		SAFE_FREE(didData);
		HidD_FreePreparsedData(pPreparsedData);
		i++;
	}
	if(usbHandle != INVALID_HANDLE_VALUE)
		CloseHandle(usbHandle);
}

void populateMappings(HWND hW)
{
	LVITEM lvItem;
	HWND lv = GetDlgItem(hW, IDC_LIST1);

	//LoadMappings(&mapVector);

	memset(&lvItem,0,sizeof(lvItem));
	
	lvItem.mask = LVIF_TEXT|LVIF_PARAM;
	lvItem.cchTextMax = 256;
	lvItem.iItem = 0;
	lvItem.iSubItem = 0;

	MapVector::iterator it;
	TCHAR tmp[256];
	int m[3];

	for(it = mapVector.begin(); it != mapVector.end(); it++)
	{
		//TODO feels a bit hacky
		bool isKB = (it->devName == TEXT("Keyboard"));
		if(isKB)
		{
			m[0] = PAD_BUTTON_COUNT;
			m[1] = PAD_AXIS_COUNT;
			m[2] = 4;
		}
		else
		{
			m[0] = MAX_BUTTONS;
			m[1] = MAX_AXES;
			m[2] = 4;
		}
		
		for(int i = 0; i<m[0]; i++)
		{
			//if((*it).btnMap[i] >= PAD_BUTTON_COUNT)
			//	continue;
			int btn = it->btnMap[i];
			int val = PLY_GET_VALUE(plyCapturing, btn);
		
			lvItem.iItem = ListView_GetItemCount(lv);
			if(PLY_IS_MAPPED(plyCapturing, btn) /*&& val < PAD_BUTTON_COUNT*/)
			{
				swprintf_s(tmp, 255, L"%s", it->devName.c_str()); //TODO
				lvItem.pszText = tmp;
				lvItem.lParam = (LPARAM)&(it->btnMap[i]);
				ListView_InsertItem(lv, &lvItem);
				swprintf_s(tmp, 255, L"P%d: Button %d", plyCapturing+1, isKB ? val : i);
				ListView_SetItemText(lv, lvItem.iItem, 1, tmp);
				
				swprintf_s(tmp, 255, L"%S", isKB ? BTN2TXT[i] : BTN2TXT[val]);
				ListView_SetItemText(lv, lvItem.iItem, 2, tmp);
			}
		}

		for(int i = 0; i<m[1]; i++)
		{
			//if((*it).axisMap[i] >= PAD_AXIS_COUNT)
			//	continue;
			int axis = it->axisMap[i];
			int val = PLY_GET_VALUE(plyCapturing, axis);

			if(PLY_IS_MAPPED(plyCapturing, axis)/* && val < PAD_AXIS_COUNT*/)
			{
				lvItem.iItem = ListView_GetItemCount(lv);
				//swprintf_s(tmp, 255, L"%s", it->devName.c_str()); //TODO
				lvItem.pszText = (LPWSTR)it->devName.c_str();
				lvItem.lParam = (LPARAM)&(it->axisMap[i]);
				ListView_InsertItem(lv, &lvItem);

				swprintf_s(tmp, 255, L"P%d: Axis %d", plyCapturing+1, isKB ? val : i);
				ListView_SetItemText(lv, lvItem.iItem, 1, tmp);

				swprintf_s(tmp, 255, L"%S", isKB ? AXIS2TXT[i] : AXIS2TXT[val]);
				ListView_SetItemText(lv, lvItem.iItem, 2, tmp);
			}
		}

		for(int i = 0; i<m[2]; i++)
		{
			int hat = it->hatMap[i];
			int val = PLY_GET_VALUE(plyCapturing, hat);

			if(PLY_IS_MAPPED(plyCapturing, hat) /*&& val < PAD_HAT_COUNT*/)
			{
				lvItem.iItem = ListView_GetItemCount(lv);
				lvItem.pszText = (LPWSTR)it->devName.c_str();
				lvItem.lParam = (LPARAM)&(it->hatMap[i]);
				ListView_InsertItem(lv, &lvItem);

				swprintf_s(tmp, 255, L"P%d: Hat %d", plyCapturing+1, isKB ? val : i);
				ListView_SetItemText(lv, lvItem.iItem, 1, tmp);

				swprintf_s(tmp, 255, L"Hat %d", isKB ? i : val);
				ListView_SetItemText(lv, lvItem.iItem, 2, tmp);
			}
		}
	}

}

#define CHECKDIFF(x) \
	if(axisPass2) {\
		if((uint32_t)abs((int)(axisDiff[(int)x] - value)) > (logical >> 2)){\
			mapping->axisMap[x] = PLY_SET_MAPPED(plyCapturing, axisCapturing);\
			axisPass2 = false;\
			fprintf(stderr, "Selected axis %d\n", x);\
			swprintf_s(buf, L"Captured wheel axis %d", x); \
			SendDlgItemMessageA(dgHwnd, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)buf); \
			axisCapturing = PAD_AXIS_COUNT;\
			goto Error;\
		}\
	break;}

static void ParseRawInput(PRAWINPUT pRawInput, HWND hW)
{
	PHIDP_PREPARSED_DATA pPreparsedData = NULL;
	HIDP_CAPS            Caps;
	PHIDP_BUTTON_CAPS    pButtonCaps = NULL;
	PHIDP_VALUE_CAPS     pValueCaps = NULL;
	USHORT               capsLength;
	UINT                 bufferSize;
	USAGE                usage[MAX_BUTTONS];
	ULONG                i, usageLength, value;
	TCHAR                 name[1024] = {0};
	UINT                 nameSize = 1024;
	UINT                 pSize;
	RID_DEVICE_INFO      devInfo;
	std::wstring          devName;
	//DevInfo_t            mapDevInfo;
	Mappings             *mapping = NULL;
	MapVector::iterator  it;
	TCHAR buf[256];
	//std::pair<MappingsMap::iterator, bool> iter;

	//
	// Get the preparsed data block
	//
	
	GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICENAME, name, &nameSize);
	pSize = sizeof(devInfo);
	GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICEINFO, &devInfo, &pSize);

	if(devInfo.dwType == RIM_TYPEKEYBOARD)
	{
		devName = TEXT("Keyboard");
	}
	else
	{
		CHECK( GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, NULL, &bufferSize) == 0 );
		CHECK( pPreparsedData = (PHIDP_PREPARSED_DATA)malloc(bufferSize) );
		CHECK( (int)GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &bufferSize) >= 0 );
		CHECK( HidP_GetCaps(pPreparsedData, &Caps) == HIDP_STATUS_SUCCESS );

		devName = name;
		std::transform(devName.begin(), devName.end(), devName.begin(), ::toupper);
	}
	
	for(it = mapVector.begin(); it != mapVector.end(); it++)
	{
		if(it->hidPath == devName)
		{
			mapping = it._Ptr;
			break;
		}
	}

	if(mapping == NULL)
	{
		Mappings m;// = new Mappings;
		ZeroMemory(&m, sizeof(Mappings));
		mapVector.push_back(m);
		mapping = &mapVector.back();
		mapping->devName = devName;
		mapping->hidPath = devName;
	}
	//TODO get real dev name, probably from registry (see lilypad)
	if(!mapping->devName.length()) mapping->devName = devName;

	if(devInfo.dwType == RIM_TYPEKEYBOARD && 
		(pRawInput->data.keyboard.Flags & RI_KEY_BREAK) != RI_KEY_BREAK)
	{
		if(pRawInput->data.keyboard.VKey == 0xff) return; //TODO
		if(pRawInput->data.keyboard.VKey == VK_ESCAPE) {
			resetState(hW);
			return;
		}
		if(btnCapturing < PAD_BUTTON_COUNT)
		{
			mapping->btnMap[btnCapturing] = PLY_SET_MAPPED(plyCapturing, pRawInput->data.keyboard.VKey);
			swprintf_s(buf, L"Captured KB button %d", pRawInput->data.keyboard.VKey);
			SendDlgItemMessageA(dgHwnd, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)buf);
			btnCapturing = PAD_BUTTON_COUNT;
		}
		else if(hatCapturing < PAD_HAT_COUNT)
		{
			for(int h=0; h < 4; h++)
			{
				if(hats7to4[h] == hatCapturing)
					mapping->hatMap[h] = PLY_SET_MAPPED(plyCapturing, pRawInput->data.keyboard.VKey);
			}
			swprintf_s(buf, L"Captured KB button %d", pRawInput->data.keyboard.VKey);
			SendDlgItemMessageA(dgHwnd, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)buf);
			hatCapturing = PAD_HAT_COUNT;
		}
		else if(axisCapturing < PAD_AXIS_COUNT)
		{
			mapping->axisMap[axisCapturing] = PLY_SET_MAPPED(plyCapturing, pRawInput->data.keyboard.VKey);
			swprintf_s(buf, L"Captured KB button %d", pRawInput->data.keyboard.VKey);
			SendDlgItemMessageA(dgHwnd, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)buf);
			axisCapturing = PAD_AXIS_COUNT;
		}
	}
	else //if(devInfo.dwType == RIM_TYPEHID)
	{
		//Capture buttons
		if(btnCapturing < PAD_BUTTON_COUNT || hatCapturing < PAD_HAT_COUNT)
		{
			// Button caps
			CHECK( pButtonCaps = (PHIDP_BUTTON_CAPS)malloc(sizeof(HIDP_BUTTON_CAPS) * Caps.NumberInputButtonCaps) );

			capsLength = Caps.NumberInputButtonCaps;
			CHECK( HidP_GetButtonCaps(HidP_Input, pButtonCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS )
			int numberOfButtons = pButtonCaps->Range.UsageMax - pButtonCaps->Range.UsageMin + 1;

			usageLength = numberOfButtons;
			CHECK(
				HidP_GetUsages(
					HidP_Input, pButtonCaps->UsagePage, 0, usage, &usageLength, pPreparsedData,
					(PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid
				) == HIDP_STATUS_SUCCESS );

			if(usageLength > 0)//Using first button only though
			{
				if(btnCapturing < PAD_BUTTON_COUNT)
				{
					mapping->btnMap[usage[0] - pButtonCaps->Range.UsageMin] = PLY_SET_MAPPED(plyCapturing, btnCapturing);
					btnCapturing = PAD_BUTTON_COUNT;
				}
				else if(hatCapturing < PAD_HAT_COUNT)
				{
					mapping->hatMap[usage[0] - pButtonCaps->Range.UsageMin] = PLY_SET_MAPPED(plyCapturing, hatCapturing);
					hatCapturing = PAD_HAT_COUNT;
				}
			}
		}
		else if(axisCapturing < PAD_AXIS_COUNT)
		{
			// Value caps
			CHECK( pValueCaps = (PHIDP_VALUE_CAPS)malloc(sizeof(HIDP_VALUE_CAPS) * Caps.NumberInputValueCaps) );
			capsLength = Caps.NumberInputValueCaps;
			CHECK( HidP_GetValueCaps(HidP_Input, pValueCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS )

			for(i = 0; i < Caps.NumberInputValueCaps && axisCapturing < PAD_AXIS_COUNT; i++)
			{
				CHECK(
					HidP_GetUsageValue(
						HidP_Input, pValueCaps[i].UsagePage, 0, pValueCaps[i].Range.UsageMin, &value, pPreparsedData,
						(PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid
					) == HIDP_STATUS_SUCCESS );

				uint32_t logical = pValueCaps[i].LogicalMax - pValueCaps[i].LogicalMin;

				switch(pValueCaps[i].Range.UsageMin)
				{
				case 0x30:	// X-axis
					CHECKDIFF(0);
					axisDiff[0] = value;
					break;

				case 0x31:	// Y-axis
					CHECKDIFF(1);
					axisDiff[1] = value;
					break;

				case 0x32: // Z-axis
					CHECKDIFF(2);
					axisDiff[2] = value;
					break;

				case 0x33: // Rotate-X
					CHECKDIFF(3);
					axisDiff[3] = value;
					break;

				case 0x34: // Rotate-Y
					CHECKDIFF(4);
					axisDiff[4] = value;
					break;

				case 0x35: // Rotate-Z
					CHECKDIFF(5);
					axisDiff[5] = value;
					break;

				case 0x39:	// Hat Switch
					if(value < 0x8) {
						mapping->axisMap[6] = PLY_SET_MAPPED(plyCapturing, axisCapturing);
						axisPass2 = false;
						fprintf(stderr, "Selected hat switch\n");
						swprintf_s(buf, L"Captured wheel hat switch"); 
						SendDlgItemMessageA(dgHwnd, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)buf);
						axisCapturing = PAD_AXIS_COUNT;
						goto Error;
					}
					break;
				}
			}

			axisPass2 = true;
		}
	}

Error:
	SAFE_FREE(pPreparsedData);
	SAFE_FREE(pButtonCaps);
	SAFE_FREE(pValueCaps);
}

//Also when switching player
void resetState(HWND hW)
{
	SendDlgItemMessage(hW, IDC_COMBO_FFB, CB_SETCURSEL, selectedJoy[plyCapturing], 0);
	SendDlgItemMessageW(hW, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)L"");

	SendDlgItemMessage(hW, IDC_COMBO_WHEEL_TYPE, CB_SETCURSEL, conf.WheelType[plyCapturing], 0);

	btnCapturing = PAD_BUTTON_COUNT;
	axisCapturing = PAD_AXIS_COUNT;
	hatCapturing = PAD_HAT_COUNT;
	ListView_DeleteAllItems(GetDlgItem(hW, IDC_LIST1));
	populateMappings(hW);
}

BOOL CALLBACK ConfigureRawDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	TCHAR buf[256];
	LVITEM lv;

	switch(uMsg) {
		case WM_INITDIALOG:
			if(!InitHid())
				return FALSE;
			dgHwnd = hW;
			//SendDlgItemMessage(hW, IDC_BUILD_DATE, WM_SETTEXT, 0, (LPARAM)__DATE__ " " __TIME__);
			
			RAWINPUTDEVICE rid[3];
			rid[0].usUsagePage = 0x01; 
			rid[0].usUsage = 0x05; 
			rid[0].dwFlags = RIDEV_INPUTSINK; // adds game pad
			rid[0].hwndTarget = hW;

			rid[1].usUsagePage = 0x01; 
			rid[1].usUsage = 0x04; 
			rid[1].dwFlags = RIDEV_INPUTSINK; // adds joystick
			rid[1].hwndTarget = hW;

			rid[2].usUsagePage = 0x01; 
			rid[2].usUsage = 0x06; 
			rid[2].dwFlags = RIDEV_INPUTSINK | RIDEV_NOLEGACY;   // adds HID keyboard and also ignores legacy keyboard messages
			rid[2].hwndTarget = hW;

			if (!RegisterRawInputDevices(rid, 3, sizeof(rid[0]))) {
				SendDlgItemMessageA(hW, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)"Could not register raw input devices.");
			}

			//LoadConfig();
			LoadMappings(&mapVector);
			//if (conf.Log) CheckDlgButton(hW, IDC_LOGGING, TRUE);
			//CheckDlgButton(hW, IDC_DFP_PASS, conf.DFPPass);
			populate(hW);
			resetState(hW);
			return TRUE;

		case WM_INPUT:
			if(axisCapturing == PAD_AXIS_COUNT &&
					btnCapturing == PAD_BUTTON_COUNT &&
					hatCapturing == PAD_HAT_COUNT)
				break;

			PRAWINPUT pRawInput;
			UINT      bufferSize;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));
			pRawInput = (PRAWINPUT)malloc(bufferSize);
			if(!pRawInput)
				break;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pRawInput, &bufferSize, sizeof(RAWINPUTHEADER));
			ParseRawInput(pRawInput, hW);
			free(pRawInput);

			if(axisCapturing == PAD_AXIS_COUNT && 
				btnCapturing == PAD_BUTTON_COUNT && 
				hatCapturing == PAD_HAT_COUNT)
			{
				resetState(hW);
			}
			return 0;

		case WM_KEYDOWN:
			if(LOWORD(lParam) == VK_ESCAPE)
			{
				resetState(hW);
				return TRUE;
			}
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
			case TCN_SELCHANGE:
				plyCapturing = SendDlgItemMessage(hW, IDC_TAB1, TCM_GETCURSEL, 0, 0);
				resetState(hW);
				break;
			}
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				switch (LOWORD(wParam))
				{
				case IDC_COMBO_WHEEL_TYPE:
					conf.WheelType[plyCapturing] = SendDlgItemMessage(hW, IDC_COMBO_WHEEL_TYPE, CB_GETCURSEL, 0, 0);
					break;
				case IDC_COMBO_FFB:
					selectedJoy[plyCapturing] = SendDlgItemMessage(hW, IDC_COMBO_FFB, CB_GETCURSEL, 0, 0);
					//player_joys[plyCapturing] = *(joysDev.begin() + selectedJoy[plyCapturing]);
					//resetState(hW);
					/*if(selectedJoy[plyCapturing] > 0 && selectedJoy[plyCapturing] == selectedJoy[1-plyCapturing])
					{
						selectedJoy[plyCapturing] = 0;
						resetState(hW);
						SendDlgItemMessage(hW, IDC_COMBO_FFB, CB_SETCURSEL, selectedJoy[plyCapturing], 0);
						//But would you want to?
						SendDlgItemMessageW(hW, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)L"Both players can't have the same controller."); //Actually, whatever, but config logics are limited ;P
					}*/
					break;
				case IDC_COMBO1:
					break;
				case IDC_COMBO2:
					break;
				default:
					return FALSE;
				}
				break;
			case BN_CLICKED:
				switch(LOWORD(wParam)) {
				case IDC_UNBIND:
					int selection;
					selection = ListView_GetSelectionMark(GetDlgItem(hW, IDC_LIST1));
					if(selection > -1)
					{
						ZeroMemory(&lv, sizeof(LVITEM));
						lv.iItem = ListView_GetSelectionMark(GetDlgItem(hW, IDC_LIST1));
						lv.mask = LVIF_PARAM;
						ListView_GetItem(GetDlgItem(hW, IDC_LIST1), &lv);
						int *map = (int*)lv.lParam;
						if(map)
						*map = PLY_UNSET_MAPPED(plyCapturing, *map);
						resetState(hW);
					}
					break;
				case IDC_BUTTON1://cross
				case IDC_BUTTON2://square
				case IDC_BUTTON3://circle
				case IDC_BUTTON4://triangle
				case IDC_BUTTON5://L1
				case IDC_BUTTON6://R1
				case IDC_BUTTON7://L2
				case IDC_BUTTON8://R2
				case IDC_BUTTON9://select
				case IDC_BUTTON10://start
				case IDC_BUTTON11://L3
				case IDC_BUTTON12://R3
					btnCapturing = (PS2Buttons) (LOWORD(wParam) - IDC_BUTTON1);
					MSG_PRESS_ESC(hW);
					return TRUE;
				case IDC_BUTTON17://x
				case IDC_BUTTON18://y
				case IDC_BUTTON19://z
				case IDC_BUTTON20://rz
				case IDC_BUTTON21://hat
					axisCapturing = (PS2Axis) (LOWORD(wParam) - IDC_BUTTON17);
					swprintf_s(buf, L"Capturing for axis %d, press ESC to cancel", axisCapturing);
					SendDlgItemMessageA(hW, IDC_STATIC_CAP, WM_SETTEXT, 0, (LPARAM)buf);
					return TRUE;
				case IDC_BUTTON13:
					hatCapturing = PAD_HAT_N;
					MSG_PRESS_ESC(hW);
					break;
				case IDC_BUTTON14:
					hatCapturing = PAD_HAT_W;
					MSG_PRESS_ESC(hW);
					break;
				case IDC_BUTTON15:
					hatCapturing = PAD_HAT_E;
					MSG_PRESS_ESC(hW);
					break;
				case IDC_BUTTON16:
					hatCapturing = PAD_HAT_S;
					MSG_PRESS_ESC(hW);
					break;
				case IDCANCEL:
					if(btnCapturing < PAD_BUTTON_COUNT || 
							axisCapturing < PAD_AXIS_COUNT || 
							hatCapturing < PAD_HAT_COUNT)
						return FALSE;
					EndDialog(hW, TRUE);
					return TRUE;
				case IDOK:
					//conf.DFPPass = IsDlgButtonChecked(hW, IDC_DFP_PASS);
					player_joys[0] = *(joysDev.begin() + selectedJoy[0]);
					player_joys[1] = *(joysDev.begin() + selectedJoy[1]);
					SaveMappings(&mapVector);
					EndDialog(hW, FALSE);
					return TRUE;
				}
			}
			
	}

	return FALSE;//DefWindowProc(hW, uMsg, wParam, lParam);
}