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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "game-optimizer-wrapper.h"
#include "configs.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/utils/string_utils.h"
#include "../../../common/rtl/rattime.h"

#include <iostream>

// default solver: Halton MetaDE
constexpr const GUID Default_Solver_Guid = { 0x1274b08, 0xf721, 0x42bc, { 0xa5, 0x62, 0x5, 0x56, 0x71, 0x4c, 0x56, 0x85 } };

// default generation count; is scaled by degree of optimalization given by outside code
constexpr const size_t Default_Generation_Count = 10000;
// default population size; may differ later, when we have more elaborate models
constexpr const size_t Default_Population_Size = 86;

#undef min
#undef max

CGame_Optimizer_Wrapper::CGame_Optimizer_Wrapper(uint32_t stepping_ms, uint16_t degree_of_opt)
	: mStep_Size(scgms::One_Second* (static_cast<double>(stepping_ms) / 1000.0)), mDegree_Of_Optimize(degree_of_opt), mOpt_State(NGame_Optimize_State::None),
	mProgress{ solver::Null_Solver_Progress }
{
	//
}

void CGame_Optimizer_Wrapper::Optimizer_Thread_Fnc()
{
	scgms::SPersistent_Filter_Chain_Configuration configuration;
	refcnt::Swstr_list errors;

	mOpt_State = NGame_Optimize_State::Running;

	HRESULT rc = E_FAIL;
	if (configuration)
		rc = configuration->Load_From_Memory(mPrepared_Config.c_str(), mPrepared_Config.size(), errors.get());

	if (!Succeeded(rc))
	{
		mOpt_State = NGame_Optimize_State::Failed;
		return;
	}

	std::wstring optParamName = Widen_String(mOpt_Filter_Parameters_Name);

	// optimize parameters, block this until it's complete
	const wchar_t* param_to_optimize_name = optParamName.c_str();
	rc = scgms::Optimize_Parameters(configuration,
		&mOpt_Filter_Idx, &param_to_optimize_name, 1,
		nullptr,
		nullptr,
		Default_Solver_Guid,
		Default_Population_Size,
		Default_Generation_Count * mDegree_Of_Optimize / 100,
		nullptr,
		0,
		mProgress,
		errors
	);

	// optimized parameters extracting
	if (Succeeded(rc))
	{
		scgms::IFilter_Configuration_Link** begin, ** end;
		configuration->get(&begin, &end);

		auto link = begin + mOpt_Filter_Idx;

		scgms::IFilter_Parameter** pbegin, ** pend;
		(*link)->get(&pbegin, &pend);

		for (; pbegin != pend; pbegin++)
		{
			scgms::SFilter_Parameter sparam = refcnt::make_shared_reference_ext<scgms::SFilter_Parameter, scgms::IFilter_Parameter>(*pbegin, true);

			auto cname = sparam.configuration_name();

			if (std::wstring_view{ cname } == optParamName)
			{
				HRESULT hr = S_OK;
				mOptimized_Parameters = sparam.as_double_array(hr);
				break;
			}
		}
	}

	if (!Succeeded(rc))
	{
		mOpt_State = NGame_Optimize_State::Failed;
		return;
	}

	mOpt_State = NGame_Optimize_State::Success;
}

bool CGame_Optimizer_Wrapper::Load_Configuration(uint16_t config_class, uint16_t config_id, const std::string& log_file_input_path, const std::string& log_file_output_path)
{
	auto cfg_guid = Get_Config_Base_GUID(config_class, config_id);
	auto params_guid = Get_Config_Parameters_GUID(config_class, config_id);

	mPrepared_Config = Get_Config(cfg_guid, params_guid, mStep_Size, log_file_input_path, log_file_output_path, NConfig_Builder_Purpose::Optimalization,
		[&](size_t idx, NConfig_Meta meta, const std::string& val) {
			if (meta == NConfig_Meta::Param_Opt_Filter)
			{
				mOpt_Filter_Idx = idx;
				mOpt_Filter_Parameters_Name = val;
			}
		}
	);

	mPrepared_Config_Replay = Get_Config(cfg_guid, params_guid, mStep_Size, log_file_input_path, log_file_output_path, NConfig_Builder_Purpose::Replay,
		[&](size_t idx, NConfig_Meta meta, const std::string& val) {
			if (meta == NConfig_Meta::Param_Opt_Filter)
				mOpt_Filter_Replay_Idx = idx;
		}
	);

	return true;
}

bool CGame_Optimizer_Wrapper::Start()
{
	if (mOpt_Thread)
		return false;

	mOpt_State = NGame_Optimize_State::Running;

	mProgress.cancelled = FALSE;
	mProgress.max_progress = 100;
	mProgress.current_progress = 0;
	mProgress.best_metric = solver::Nan_Fitness;

	mOpt_Thread = std::make_unique<std::thread>(&CGame_Optimizer_Wrapper::Optimizer_Thread_Fnc, this);

	return true;
}

NGame_Optimize_State CGame_Optimizer_Wrapper::Get_Progress(double& pct)
{
	if (mOpt_State == NGame_Optimize_State::Success)
		pct = 1.0;
	else
		pct = static_cast<double>(mProgress.current_progress) / static_cast<double>(mProgress.max_progress);

	return mOpt_State;
}

bool CGame_Optimizer_Wrapper::Replay()
{
	scgms::SPersistent_Filter_Chain_Configuration configuration;
	refcnt::Swstr_list errors;

	HRESULT rc = E_FAIL;
	if (configuration)
		rc = configuration->Load_From_Memory(mPrepared_Config_Replay.c_str(), mPrepared_Config_Replay.size(), errors.get());

	if (!Succeeded(rc))
	{
		mOpt_State = NGame_Optimize_State::Failed;
		return false;
	}

	std::wstring optParamName = Widen_String(mOpt_Filter_Parameters_Name);

	// set optimized parameters to filter in replay config
	{
		scgms::IFilter_Configuration_Link** begin, **end;
		configuration->get(&begin, &end);

		auto link = begin + mOpt_Filter_Replay_Idx;

		scgms::IFilter_Parameter** pbegin, ** pend;
		(*link)->get(&pbegin, &pend);

		for (; pbegin != pend; pbegin++)
		{
			scgms::SFilter_Parameter sparam = refcnt::make_shared_reference_ext<scgms::SFilter_Parameter, scgms::IFilter_Parameter>(*pbegin, true);

			auto cname = sparam.configuration_name();

			if (std::wstring_view{ cname } == optParamName)
			{
				sparam.set_double_array(mOptimized_Parameters);
				break;
			}
		}
	}

	// launch the replay
	scgms::SFilter_Executor ex{ configuration, nullptr, nullptr, errors };

	if (!ex)
		return false;

	// wait for shutdown; we just want to store results to log file
	ex->Terminate(TRUE);

	return true;
}

extern "C" scgms_game_optimizer_wrapper_t IfaceCalling scgms_game_optimize(uint16_t config_class, uint16_t config_id, uint32_t stepping_ms, const char* log_file_input_path, const char* log_file_output_path, uint16_t degree_of_opt)
{
	std::unique_ptr<CGame_Optimizer_Wrapper> wrapper = std::make_unique<CGame_Optimizer_Wrapper>(stepping_ms, degree_of_opt);

	if (!wrapper->Load_Configuration(config_class, config_id, log_file_input_path, log_file_output_path))
		return nullptr;

	if (!wrapper->Start())
		return nullptr;

	auto res = wrapper.get();
	wrapper.release();
	return res;
}

extern "C" BOOL IfaceCalling scgms_game_get_optimize_status(scgms_game_optimizer_wrapper_t wrapper_raw, NGame_Optimize_State * state, double* progress_pct)
{
	CGame_Optimizer_Wrapper* wrapper = dynamic_cast<CGame_Optimizer_Wrapper*>(wrapper_raw);
	if (!wrapper)
		return FALSE;

	*state = wrapper->Get_Progress(*progress_pct);

	return TRUE;
}

extern "C" BOOL IfaceCalling scgms_game_optimizer_terminate(scgms_game_optimizer_wrapper_t wrapper_raw)
{
	CGame_Optimizer_Wrapper* wrapper = dynamic_cast<CGame_Optimizer_Wrapper*>(wrapper_raw);
	if (!wrapper)
		return FALSE;

	return wrapper->Replay() ? TRUE : FALSE;
}
