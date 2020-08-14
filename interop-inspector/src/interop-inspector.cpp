#include "interop-inspector.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/utils/string_utils.h"

/*
 * IUnknown bridging functions
 */

extern "C" HRESULT IfaceCalling scgms_query_interface(scgms::IFilter* refPtr, GUID* iid, void** iobj)
{
	return refPtr->QueryInterface(iid, iobj);
}

extern "C" HRESULT IfaceCalling scgms_add_ref(scgms::IFilter* refPtr)
{
	return refPtr->AddRef();
}

extern "C" HRESULT IfaceCalling scgms_release(scgms::IFilter* refPtr)
{
	return refPtr->Release();
}

/*
 * Interop types factory functions
 */

template<class T>
HRESULT create_character_container(refcnt::IVector_Container<T>** str)
{
	if (!str)
		return E_FAIL;

	*str = refcnt::Create_Container<T>(nullptr, nullptr);

	return ((*str) != nullptr) ? S_OK : E_FAIL;
}

extern "C" HRESULT IfaceCalling scgms_create_str_container(refcnt::str_container **str)
{
	return create_character_container<char>(str);
}

extern "C" HRESULT IfaceCalling scgms_create_wstr_container(refcnt::wstr_container **str)
{
	return create_character_container<wchar_t>(str);
}

template<class T>
HRESULT extract_character_container(refcnt::IVector_Container<T>* str, T** target)
{
	T *begin, *end;
	if (str->get(&begin, &end) != S_OK)
		return E_FAIL;
	size_t length = std::distance(begin, end);

	T* tmpStr = new T[length + 1];

	for (size_t i = 0; i < length; i++)
		tmpStr[i] = begin[i];
	tmpStr[length] = 0;

	*target = tmpStr;

	return S_OK;
}

extern "C" HRESULT IfaceCalling scgms_extract_str_container(refcnt::str_container *str, char** target)
{
	return extract_character_container<char>(str, target);
}

extern "C" HRESULT IfaceCalling scgms_extract_wstr_container(refcnt::wstr_container *str, wchar_t** target)
{
	return extract_character_container<wchar_t>(str, target);
}

extern "C" HRESULT IfaceCalling scgms_convert_str_to_wstr(char* str, wchar_t** outstr)
{
	std::wstring wstr = Widen_Char(str);

	wchar_t* tmpStr = new wchar_t[wstr.size() + 1];
	size_t i;
	for (i = 0; i < wstr.size(); i++)
		tmpStr[i] = wstr[i];
	tmpStr[i] = L'\0';

	*outstr = tmpStr;

	return S_OK;
}

/*
 * Inspection callable bridge functions
 */

extern "C" HRESULT IfaceCalling scgms_drawing__new_data_available(scgms::IDrawing_Filter_Inspection* ref)
{
	return ref->New_Data_Available();
}

extern "C" HRESULT IfaceCalling scgms_drawing__draw(scgms::IDrawing_Filter_Inspection* ref, uint16_t type, uint16_t diagnosis, refcnt::str_container *svg, refcnt::IVector_Container<uint64_t> *segmentIds, refcnt::IVector_Container<GUID> *signalIds)
{
	return ref->Draw((scgms::TDrawing_Image_Type)type, (scgms::TDiagnosis)diagnosis, svg, segmentIds, signalIds);
}
