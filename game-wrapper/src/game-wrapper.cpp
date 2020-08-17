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

#include "game-wrapper.h"
#include "configs.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/utils/string_utils.h"
#include "../../../common/rtl/rattime.h"

CGame_Wrapper::CGame_Wrapper(uint32_t stepping_ms)
	: mStep_Size(scgms::One_Second * (static_cast<double>(stepping_ms)/1000.0))
{
	//
}

CGame_Wrapper::~CGame_Wrapper()
{

}

bool CGame_Wrapper::Load_Configuration(uint16_t config_class, uint16_t config_id, const std::string& log_file_path)
{
	mConfig_Contents = Get_Config(config_class, config_id, mStep_Size, log_file_path);

	return !mConfig_Contents.empty();
}

bool CGame_Wrapper::Execute_Configuration()
{
	mErrors = refcnt::Swstr_list{};
	scgms::SPersistent_Filter_Chain_Configuration configuration{};
	if (configuration->Load_From_Memory(mConfig_Contents.c_str(), mConfig_Contents.length(), mErrors.get()) == S_OK)
	{
		scgms::SFilter_Executor ex{ configuration, nullptr, nullptr, mErrors, this };
		mExecutor.reset(ex.get(), [](scgms::IFilter_Executor* obj_to_release) { if (obj_to_release != nullptr) obj_to_release->Release(); });
		ex.get()->AddRef();
	}

	mCurrent_Time = Unix_Time_To_Rat_Time(time(nullptr)); // at least preserve the initial timestamp (any further timestamps do not correspond to real-time)
	mSegment_Id = 1;

	return mExecutor.operator bool();
}

HRESULT IfaceCalling CGame_Wrapper::Configure(scgms::IFilter_Configuration* configuration, refcnt::wstr_list *error_description)
{
	return E_NOTIMPL;
}

HRESULT IfaceCalling CGame_Wrapper::Execute(scgms::IDevice_Event *event)
{
	scgms::UDevice_Event evt{ event };

	if (evt.event_code() == scgms::NDevice_Event_Code::Level)
	{
		if (evt.signal_id() == scgms::signal_BG)
			mState.bg = evt.level();
		else if (evt.signal_id() == scgms::signal_IG)
			mState.ig = evt.level();
		else if (evt.signal_id() == scgms::signal_IOB)
			mState.iob = evt.level();
		else if (evt.signal_id() == scgms::signal_COB)
			mState.cob = evt.level();
	}

	// UDevice_Event destructor does this for us
	//event->Release();

	return S_OK;
}

HRESULT CGame_Wrapper::Inject_Event(scgms::UDevice_Event &&event)
{
	if (!event)
		return E_INVALIDARG;

	scgms::IDevice_Event *raw_event = event.get();
	event.release();
	return mExecutor->Execute(raw_event);
}

bool CGame_Wrapper::Step(bool initial)
{
	std::unique_lock<std::mutex> lck(mExecution_Mtx);

	// do not advance simulation time on initial step
	if (!initial)
		mCurrent_Time += mStep_Size;

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.level() = 0.0;
	evt.device_time() = mCurrent_Time;
	evt.signal_id() = scgms::signal_Synchronization;
	evt.segment_id() = mSegment_Id;
	evt.device_id() = game_wrapper_id;

	return Succeeded(Inject_Event(std::move(evt)));
}

bool CGame_Wrapper::Inject_Bolus(double level)
{
	std::unique_lock<std::mutex> lck(mExecution_Mtx);

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.level() = level;
	evt.device_time() = mCurrent_Time;
	evt.signal_id() = scgms::signal_Requested_Insulin_Bolus;
	evt.segment_id() = mSegment_Id;
	evt.device_id() = game_wrapper_id;

	return Succeeded(Inject_Event(std::move(evt)));
}

bool CGame_Wrapper::Inject_Basal_Rate(double level)
{
	std::unique_lock<std::mutex> lck(mExecution_Mtx);

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.level() = level;
	evt.device_time() = mCurrent_Time;
	evt.signal_id() = scgms::signal_Requested_Insulin_Basal_Rate;
	evt.segment_id() = mSegment_Id;
	evt.device_id() = game_wrapper_id;

	return Succeeded(Inject_Event(std::move(evt)));
}

bool CGame_Wrapper::Inject_CHO(double level, bool rescue)
{
	std::unique_lock<std::mutex> lck(mExecution_Mtx);

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.level() = level;
	evt.device_time() = mCurrent_Time;
	evt.signal_id() = rescue ? scgms::signal_Carb_Rescue : scgms::signal_Carb_Intake;
	evt.segment_id() = mSegment_Id;
	evt.device_id() = game_wrapper_id;

	return Succeeded(Inject_Event(std::move(evt)));
}

void CGame_Wrapper::Terminate(const BOOL wait_for_shutdown)
{
	std::unique_lock<std::mutex> lck(mExecution_Mtx);

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Shut_Down };

	evt.device_time() = mCurrent_Time;
	evt.segment_id() = mSegment_Id;
	evt.device_id() = game_wrapper_id;

	Inject_Event(std::move(evt));

	mExecutor->Terminate(wait_for_shutdown);
}

const CPatient_Sensor_State& CGame_Wrapper::Get_State() const
{
	return mState;
}

extern "C" scgms_game_wrapper_t IfaceCalling scgms_game_create(uint16_t config_class, uint16_t config_id, uint32_t stepping_ms, const char* log_file_path)
{
	std::unique_ptr<CGame_Wrapper> wrapper = std::make_unique<CGame_Wrapper>(stepping_ms);

	if (!wrapper->Load_Configuration(config_class, config_id, log_file_path))
		return nullptr;

	if (!wrapper->Execute_Configuration())
		return nullptr;

	// make the first step, which initializes the model (and emits current state)
	wrapper->Step(true);

	auto res = wrapper.get();
	wrapper.release();
	return res;
}

extern "C" BOOL IfaceCalling scgms_game_step(scgms_game_wrapper_t wrapper_raw, double *boluses, uint32_t bolus_cnt, double *carbohydrates, uint8_t *cho_rescue_flag, uint32_t carbohydrates_cnt, double basal_insulin_setting, double* bg, double* ig, double* iob, double* cob)
{
	CGame_Wrapper* wrapper = dynamic_cast<CGame_Wrapper*>(wrapper_raw);
	if (!wrapper)
		return FALSE;

	uint32_t i;
	bool success = true;

	for (i = 0; i < bolus_cnt; i++)
		success &= wrapper->Inject_Bolus(boluses[i]);

	for (i = 0; i < carbohydrates_cnt; i++)
		success &= wrapper->Inject_CHO(carbohydrates[i], (cho_rescue_flag[i] != 0));

	if (std::isnan(basal_insulin_setting) && basal_insulin_setting > 0)
		success &= wrapper->Inject_Basal_Rate(basal_insulin_setting);

	if (!success)
		return FALSE;

	if (!wrapper->Step())
		return FALSE;

	auto state = wrapper->Get_State();
	if (bg)
		*bg = state.bg;
	if (ig)
		*ig = state.ig;
	if (iob)
		*iob = state.iob;
	if (cob)
		*cob = state.cob;

	return TRUE;
}

extern "C" BOOL IfaceCalling scgms_game_terminate(scgms_game_wrapper_t wrapper_raw)
{
	CGame_Wrapper* wrapper = dynamic_cast<CGame_Wrapper*>(wrapper_raw);
	if (!wrapper)
		return FALSE;

	wrapper->Terminate(TRUE);

	return TRUE;
}