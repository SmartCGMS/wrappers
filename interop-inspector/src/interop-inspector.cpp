/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "interop-inspector.h"

#include <scgms/rtl/referencedImpl.h>
#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/SolverLib.h>
#include <scgms/utils/string_utils.h>
#include <scgms/utils/math_utils.h>
#include <scgms/utils/DebugHelper.h>

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

DLL_EXPORT HRESULT IfaceCalling scgms_create_str_container(refcnt::str_container **str)
{
	return create_character_container<char>(str);
}

DLL_EXPORT HRESULT IfaceCalling scgms_create_wstr_container(refcnt::wstr_container **str)
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

DLL_EXPORT HRESULT IfaceCalling scgms_extract_str_container(refcnt::str_container *str, char** target)
{
	return extract_character_container<char>(str, target);
}

DLL_EXPORT HRESULT IfaceCalling scgms_extract_wstr_container(refcnt::wstr_container *str, wchar_t** target)
{
	return extract_character_container<wchar_t>(str, target);
}

DLL_EXPORT HRESULT IfaceCalling scgms_convert_str_to_wstr(char* str, wchar_t** outstr)
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
 * SCGMS additions to simple iface
 */

 // default solver: Halton MetaDE
constexpr const GUID Default_Solver_Guid = { 0x1b21b62f, 0x7c6c, 0x4027,{ 0x89, 0xbc, 0x68, 0x7d, 0x8b, 0xd3, 0x2b, 0x3c } };	// {1B21B62F-7C6C-4027-89BC-687D8BD32B3C}

DLL_EXPORT HRESULT IfaceCalling scgms_optimizer__create_progress_instance(solver::TSolver_Progress** progress)
{
	if (!progress)
		return E_FAIL;

	*progress = new solver::TSolver_Progress;
	**progress = solver::Null_Solver_Progress;

	return S_OK;
}

DLL_EXPORT HRESULT IfaceCalling scgms_optimizer__dump_progress(solver::TSolver_Progress* progress, double* pctDone, double* bestMetric)
{
	if (!progress)
		return E_FAIL;

	*bestMetric = Is_Any_NaN(progress->best_metric[0]) ? 0.0 : progress->best_metric[0];
	*pctDone = 0;
	
	if (progress->max_progress > 0)
		*pctDone = static_cast<double>(progress->current_progress) / static_cast<double>(progress->max_progress);

	return S_OK;
}

DLL_EXPORT HRESULT IfaceCalling scgms_optimizer__optimize_parameters(const char* config, uint32_t optimizeIdx, const char* optimizeParamName, uint32_t optGenCount, uint32_t optPopulationSize, solver::TSolver_Progress* progress, char** target)
{
	scgms::SPersistent_Filter_Chain_Configuration configuration;

	refcnt::Swstr_list errors;

	std::string configStr(config);
	std::wstring optParamName = Widen_String(optimizeParamName);

	HRESULT rc = E_FAIL;
	if (configuration)
		rc = configuration->Load_From_Memory(configStr.c_str(), configStr.size(), errors.get());

	if (!Succeeded(rc))
		return rc;

	size_t filterIdx = static_cast<size_t>(optimizeIdx);
	size_t populationSize = static_cast<size_t>(optPopulationSize);
	size_t generationCount = static_cast<size_t>(optGenCount);

	solver::TSolver_Progress& progressRef = *progress;

	const wchar_t* param_to_optimize_name = optParamName.c_str();
	rc = scgms::Optimize_Parameters(configuration,
		&filterIdx, &param_to_optimize_name, 1,
		nullptr,
		nullptr,
		Default_Solver_Guid,
		populationSize,
		generationCount,
		nullptr,
		0,
		progressRef,
		errors
	);

	// optimized parameters extracting
	if (Succeeded(rc))
	{
		scgms::IFilter_Configuration_Link** begin, ** end;
		configuration->get(&begin, &end);

		auto link = begin + filterIdx;

		scgms::IFilter_Parameter** pbegin, ** pend;
		(*link)->get(&pbegin, &pend);

		for (; pbegin != pend; pbegin++)
		{
			scgms::SFilter_Parameter sparam = refcnt::make_shared_reference_ext<scgms::SFilter_Parameter, scgms::IFilter_Parameter>(*pbegin, true);

			auto cname = sparam.configuration_name();

			if (std::wstring_view{ cname } == optParamName)
			{
				HRESULT hr = S_OK;

				std::wstring str = sparam.as_wstring(hr, true);
				std::string paramsStrRaw = Narrow_WString(str);

				*target = new char[paramsStrRaw.size() + 1];
				std::fill(*target, (*target) + paramsStrRaw.size() + 1, '\0');

				for (size_t i = 0; i < paramsStrRaw.size(); i++)
					(*target)[i] = paramsStrRaw[i];

				break;
			}
		}
	}

	return rc;
}

/*
 * Inspection callable bridge functions
 */

DLL_EXPORT HRESULT IfaceCalling scgms_drawing__new_data_available(scgms::IDrawing_Filter_Inspection* ref)
{
	return ref->New_Data_Available();
}

DLL_EXPORT HRESULT IfaceCalling scgms_drawing__draw(scgms::IDrawing_Filter_Inspection* ref, uint16_t type, uint16_t diagnosis, refcnt::str_container *svg, refcnt::IVector_Container<uint64_t> *segmentIds, refcnt::IVector_Container<GUID> *signalIds)
{
	return ref->Draw((scgms::TDrawing_Image_Type)type, (scgms::TDiagnosis)diagnosis, svg, segmentIds, signalIds);
}

DLL_EXPORT HRESULT IfaceCalling scgms_error_metric__promise(scgms::ISignal_Error_Inspection* ref, const uint64_t segment_id, bool all_segments, double* const metric_value, BOOL defer_to_dtor)
{
	return ref->Promise_Metric(all_segments ? scgms::All_Segments_Id : segment_id, metric_value, defer_to_dtor);
}
