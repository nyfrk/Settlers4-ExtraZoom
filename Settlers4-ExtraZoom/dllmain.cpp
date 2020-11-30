#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <hlib.h>

using namespace hlib;
using namespace std;

#define PATTERN_FAR_LIMIT_HE  "69 45 08 20 4E 00 00 8B 51 4C 56 8B F2 2B F0 66 0F 6E C2 B8 00 00 0E 00"
#define PATTERN_FAR_LIMIT_GE "8D 0C 89 89 44 24 0C 8D 0C 89 8D 0C 89 8D 0C 89 C1 E1 05 2B C1 8B F8 81 FF 00 00 0E 00 89 7C 24 3C 7D 09 BF 00 00 0E 00 89 7C 24 3C"

#define PATTERN_SLIDER_HE "8D 90 00 00 0E 00 8B CA 89 53 4C"
#define PATTERN_SLIDER_GE "BE 00 00 0E 00 89 7D F0 2B F0"

#define PATTERN_SLIDER_BASE_HE "8B 46 4C 2D 00 00 0E 00 8B 0D"
#define PATTERN_SLIDER_BASE_NEAR_GE "81 EF 00 00 0E 00 89 5C 24 14 89 7C 24 10 89 44 24 18"
#define PATTERN_SLIDER_BASE_FAR_GE "8B 76 3C 33 DB 81 EE 00 00 0E 00"       

#define PATTERN_NEAR_LIMIT_HE "69 55 08 20 4E 00 00 8B 41 4C 56 BE 00 00 48 00"
#define PATTERN_NEAR_LIMIT_GE "8D 04 80 8B F9 8B 4F 3C 8D 04 80 89 4C 24 0C 8D 04 80 8D 34 80 C1 E6 05 03 F1 81 FE 00 00 48 00 76 05 BE 00 00 48 00"

#define PATTERN_SOUND_HE "8B 40 4C 2D 00 00 0E 00 66 0F 6E C0 F3 0F E6 C0 C1 E8 1F"
#define PATTERN_SOUND_GE "00 00 00 00 2D 00 00 0E 00 89 44 24 00 DF 6C 24 00 D8 0D"
#define PATTERN_UNKREF_HE "8B 47 4C 2D 00 00 0E 00"
#define PATTERN_UNKREF_GE "? ? ? 2D 00 00 0E 00 89 5C 24 14 89 44 24 10 89 5C 24 18"

static const DWORD OriginalFarLimit =   0xE0000u; // should not go lower than 0x20000
static const DWORD OriginalNearLimit = 0x480000u; // cannot go more than  0x8A0000
static const DWORD OriginalStepFactor = 20000u;

static const int 
	DefaultFarLimit = 4,
	DefaultNearLimit = 0,
	DefaultSteps = 150;

static DWORD NewFarLimit = OriginalFarLimit;
static DWORD NewNearLimit = OriginalNearLimit;
static DWORD NewStepFactor = OriginalStepFactor;
static BOOL RoundZoomLevel = 1;

static int clamp(int min, int max, int val) {
	if (val < min)
		return min;
	if (val > max)
		return max;
	return val;
}

static LPWSTR filenameOf(LPWSTR str) {
	if (str == NULL) return str;
	for (WCHAR* c = str; *c != 0; c++) {
		if (*c == L'/' || *c == L'\\') { str = c; ++str; }
	}
	return str;
}

static void __declspec(naked) __onSliderHookHE() {
	// make sure the least significant word is 0
	__asm {
		cmp RoundZoomLevel, 0
		jz lbl_NotRound
		xor ax, ax
		lbl_NotRound:
		mov edx, eax
		add edx, NewFarLimit
		ret
	}
}

static void __declspec(naked) __onFarStepHookHE() {
	// make sure the least significant word is 0
	__asm {
		mov eax, [ebp + 8]
		imul eax, NewStepFactor
		cmp RoundZoomLevel, 0
		jz lbl_NotRound
		xor ax, ax
		lbl_NotRound :
		ret
	}
}
static void __declspec(naked) __onNearStepHookHE() {
	// make sure the least significant word is 0
	__asm {
		mov edx, [ebp + 8]
		imul edx, NewStepFactor
		cmp RoundZoomLevel, 0
		jz lbl_NotRound
		xor dx, dx
		lbl_NotRound :
		ret
	}
}

static void __declspec(naked) __onSliderHookGE() {
	// make sure the least significant word is 0
	__asm {
		mov esi, NewFarLimit
		mov[ebp - 0x10], edi
		sub esi, eax
		cmp RoundZoomLevel, 0
		jz lbl_NotRound
		xor si, si
		lbl_NotRound :
		ret
	}
}

static void __declspec(naked) __onFarStepHookGE() {
	// make sure the least significant word is 0
	__asm {
		mov[esp + 0x0C +4], eax
		imul ecx, NewStepFactor
		cmp RoundZoomLevel, 0
		jz lbl_NotRound
		xor cx, cx
	lbl_NotRound:
		sub eax, ecx
		mov edi, eax
		cmp edi, NewFarLimit
		jnl lbl_noclamp
		mov edi, NewFarLimit
	lbl_noclamp:
		mov[esp + 0x3C +4], edi
		ret
	}
}

static void __declspec(naked) __onNearStepHookGE() {
	// make sure the least significant word is 0
	__asm {
		mov edi, ecx
		mov ecx, [edi + 0x3C]
		mov[esp + 0x0C +4], ecx

		imul eax, NewStepFactor
		cmp RoundZoomLevel, 0
		jz lbl_NotRound
		xor ax, ax
	lbl_NotRound :
		mov esi, eax
		add esi, ecx

		cmp esi, NewNearLimit
		jna lbl_noClamp
		mov esi, NewNearLimit
	lbl_noClamp:
		ret
	}
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	static CallPatch FarStepPatch;
	static Patch FarLimitPatch;
	static CallPatch SliderPatch;
	static Patch SliderBasePatch; // Near only for GE
	static Patch SliderBasePatchFarGE;
	static CallPatch NearStepPatch;
	static Patch NearLimitPatch;
	static Patch SoundPatch1,
		SoundPatch2; // Only for HE
	static Patch UnknRefPatch;

	static const HANDLE hProcess = GetCurrentProcess();

	static struct {	
		unsigned 
			isFarStepPatch : 1,
			isFarLimitPatch : 1,
			isSliderPatch : 1,
			isSliderBasePatch : 1,
			isSliderBasePatchFarGE : 1,
			isNearStepPatch : 1,
			isNearLimitPatch : 1,
			isSoundPatch1 : 1,
			isSoundPatch2 : 1,
			isUnkRefPatch : 1;
	} AppliedPatches = { 0 };

    switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: {
		const DWORD S4_Main = (DWORD)GetModuleHandleA(NULL);
		const bool isGE = GetModuleHandleA("GfxEngine.dll"); // if this module is present, we assume gold edition

		{
			WCHAR buf[MAX_PATH + 1];
			auto length = GetModuleFileNameW(hModule, buf, _countof(buf) - 1);
			buf[length] = '\0'; // xp compatibility

			if (length >= 0) { // we got a valid file name
				LPWSTR fn = filenameOf(buf);
				wcscpy_s(fn, _countof(buf) - (fn - buf), L"ExtraZoom.ini");
				auto farLimit = clamp(-30, 6, GetPrivateProfileIntW(L"ExtraZoom", L"FarLimit", DefaultFarLimit, buf));
				auto nearLimit = clamp(0, 33, GetPrivateProfileIntW(L"ExtraZoom", L"NearLimit", DefaultNearLimit, buf));
				auto steps = clamp(10, 1000, GetPrivateProfileIntW(L"ExtraZoom", L"Steps", DefaultSteps, buf));

				NewFarLimit = OriginalFarLimit - farLimit * 0x20000;
				NewNearLimit = OriginalNearLimit + nearLimit * 0x20000;
				NewStepFactor = OriginalStepFactor * steps / 100;

				RoundZoomLevel = clamp(0, 1, GetPrivateProfileIntW(L"ExtraZoom", L"RoundFix", NewFarLimit < OriginalFarLimit || NewStepFactor != OriginalStepFactor, buf));
			}
		}

		const DWORD FarLimitPattern = (DWORD)FindPattern(hProcess, S4_Main,
			StringPattern(isGE ? PATTERN_FAR_LIMIT_GE : PATTERN_FAR_LIMIT_HE));

		const DWORD SliderPattern = (DWORD)FindPattern(hProcess, S4_Main,
			StringPattern(isGE ? PATTERN_SLIDER_GE : PATTERN_SLIDER_HE));

		const DWORD SliderBasePattern = (DWORD)FindPattern(hProcess, S4_Main,
			StringPattern(isGE ? PATTERN_SLIDER_BASE_NEAR_GE : PATTERN_SLIDER_BASE_HE));

		const DWORD SliderBasePatternFarGE = isGE ? (DWORD)FindPattern(hProcess, S4_Main,
			StringPattern(PATTERN_SLIDER_BASE_FAR_GE)) : 1;

		const DWORD NearLimitPattern = (DWORD)FindPattern(hProcess, S4_Main,
			StringPattern(isGE ? PATTERN_NEAR_LIMIT_GE : PATTERN_NEAR_LIMIT_HE));

		const DWORD SoundPattern1 =	(DWORD)FindPattern(hProcess, S4_Main, 
			StringPattern(isGE ? PATTERN_SOUND_GE : PATTERN_SOUND_HE));

		const DWORD SoundPattern2 = isGE ? 1 : (DWORD)FindPattern(hProcess, SoundPattern1 + 1, StringPattern(PATTERN_SOUND_HE));

		const DWORD UnkRefPattern = (DWORD)FindPattern(hProcess, S4_Main,
			StringPattern(isGE ? PATTERN_UNKREF_GE : PATTERN_UNKREF_HE));


		if (!FarLimitPattern || !SliderPattern || !SliderBasePattern || !SliderBasePatternFarGE || !NearLimitPattern || !SoundPattern1 || !SoundPattern2  || !UnkRefPattern) {
			MessageBox(NULL, 
				"Sorry, your game version is not supported by the ExtraZoom plugin."
				"Please open a ticket here: https://github.com/nyfrk/Settlers4-ExtraZoom/issues",
				"S4 ExtraZoom", MB_ICONWARNING | MB_OK);
		}
		else {
			if (RoundZoomLevel || NewStepFactor != OriginalStepFactor || (isGE && NewFarLimit != OriginalFarLimit)) {
				AppliedPatches.isFarLimitPatch = true;
				if (isGE) 
					FarStepPatch = CallPatch(FarLimitPattern, (DWORD)__onFarStepHookGE, 39);
				else
					FarStepPatch = CallPatch(FarLimitPattern, (DWORD)__onFarStepHookHE, 2);
				FarStepPatch.patch(hProcess);
			}


			if (NewFarLimit != OriginalFarLimit && !isGE) {
				AppliedPatches.isFarLimitPatch = true;
				FarLimitPatch = Patch(FarLimitPattern + 20, NewFarLimit);
				FarLimitPatch.patch(hProcess);
			}

			if (RoundZoomLevel || NewFarLimit != OriginalFarLimit) {
				AppliedPatches.isSliderPatch = true;
				if (isGE) 
					SliderPatch = CallPatch(SliderPattern, (DWORD)__onSliderHookGE, 5);
				else
					SliderPatch = CallPatch(SliderPattern, (DWORD)__onSliderHookHE, 1);
				SliderPatch.patch(hProcess);
			}

			if (NewFarLimit != OriginalFarLimit) {
				AppliedPatches.isSliderBasePatch = true;
				if (isGE) {
					AppliedPatches.isSliderBasePatchFarGE = true;
					SliderBasePatch = Patch(SliderBasePattern + 2, NewFarLimit);
					SliderBasePatchFarGE = Patch(SliderBasePatternFarGE + 7, NewFarLimit);
					SliderBasePatchFarGE.patch(hProcess);
				}
				else
					SliderBasePatch = Patch(SliderBasePattern + 4, NewFarLimit);
				SliderBasePatch.patch(hProcess);
			}

			if (RoundZoomLevel || NewStepFactor != OriginalStepFactor || (isGE && NewNearLimit != OriginalNearLimit)) {
				AppliedPatches.isNearStepPatch = true;
				if (isGE)
					NearStepPatch = CallPatch(NearLimitPattern, (DWORD)__onNearStepHookGE, 34);
				else
					NearStepPatch = CallPatch(NearLimitPattern, (DWORD)__onNearStepHookHE, 2);
				NearStepPatch.patch(hProcess);
			}

			if (!isGE && NewNearLimit != OriginalNearLimit) {
				AppliedPatches.isNearLimitPatch = true;
				NearLimitPatch = Patch(NearLimitPattern + 12, NewNearLimit);
				NearLimitPatch.patch(hProcess);
			}

			if (NewFarLimit != OriginalFarLimit) {
				AppliedPatches.isSoundPatch1 = true;
				AppliedPatches.isUnkRefPatch = true;
				SoundPatch1 = Patch(SoundPattern1 + 4 + (int)isGE, NewFarLimit);
				SoundPatch1.patch(hProcess);
				UnknRefPatch = Patch(UnkRefPattern + 4, NewFarLimit);
				UnknRefPatch.patch(hProcess);
				if (!isGE) {
					AppliedPatches.isSoundPatch2 = true;
					SoundPatch2 = Patch(SoundPattern1 + 4, NewFarLimit);
					SoundPatch2.patch(hProcess);
				}
			}

		}
		break;
	}
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
		break;
    case DLL_PROCESS_DETACH:

		if (AppliedPatches.isFarStepPatch) FarStepPatch.unpatch(hProcess);
		if (AppliedPatches.isFarLimitPatch) FarLimitPatch.unpatch(hProcess);
		if (AppliedPatches.isSliderPatch) SliderPatch.unpatch(hProcess);
		if (AppliedPatches.isSliderBasePatch) SliderBasePatch.unpatch(hProcess);
		if (AppliedPatches.isSliderBasePatchFarGE) SliderBasePatchFarGE.unpatch(hProcess);
		if (AppliedPatches.isNearStepPatch) NearStepPatch.unpatch(hProcess);
		if (AppliedPatches.isNearLimitPatch) NearLimitPatch.unpatch(hProcess);
		if (AppliedPatches.isSoundPatch1) SoundPatch1.unpatch(hProcess);
		if (AppliedPatches.isSoundPatch2) SoundPatch2.unpatch(hProcess);
		if (AppliedPatches.isUnkRefPatch) UnknRefPatch.unpatch(hProcess);
        break;
    }
    return TRUE;
}

