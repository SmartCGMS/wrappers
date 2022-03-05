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

#include <iostream>

#undef min
#undef max

CGame_Wrapper::CGame_Wrapper(uint32_t stepping_ms)
	: mStep_Size(scgms::One_Second* (static_cast<double>(stepping_ms) / 1000.0)), mConfig_GUID{ Invalid_GUID }, mCurrent_Time{ 0.0 }, mParameters_GUID{ Invalid_GUID },
	mSegment_Id{ 1 }
{
	//
}

CGame_Wrapper::~CGame_Wrapper()
{

}

bool CGame_Wrapper::Load_Configuration(uint16_t config_class, uint16_t config_id, const std::string& log_file_path)
{
	mIs_Replay = false;

	mConfig_GUID = Get_Config_Base_GUID(config_class, config_id);
	mParameters_GUID = Get_Config_Parameters_GUID(config_class, config_id);

	mConfig_Contents = Get_Config(mConfig_GUID, mParameters_GUID, mStep_Size, log_file_path, log_file_path, NConfig_Builder_Purpose::Gameplay);

	return !mConfig_Contents.empty();
}

bool CGame_Wrapper::Load_Replay_Configuration(const std::string& log_file_path)
{
	mIs_Replay = true;

	mConfig_Contents = Get_Replay_Config(log_file_path);

	return !mConfig_Contents.empty();
}

bool CGame_Wrapper::Execute_Configuration()
{
	mErrors = refcnt::Swstr_list{};
	scgms::SPersistent_Filter_Chain_Configuration configuration{};
	if (configuration->Load_From_Memory(mConfig_Contents.c_str(), mConfig_Contents.length(), mErrors.get()) == S_OK)
	{
		scgms::SFilter_Executor ex{ configuration, nullptr, nullptr, mErrors, this };

		mErrors.for_each([](const std::wstring& err) {
			std::wcerr << "Error: " << err << std::endl;
		});


		if (!ex) {
			return false;
		}

		mExecutor.reset(ex.get(), [](scgms::IFilter_Executor* obj_to_release) { if (obj_to_release != nullptr) obj_to_release->Release(); });
		ex.get()->AddRef();
	}

	if (!mExecutor)
		return false;

	mCurrent_Time = Unix_Time_To_Rat_Time(time(nullptr)); // at least preserve the initial timestamp (any further timestamps do not correspond to real-time)
	mSegment_Id = 1;

	// replays just gather all signals and terminate
	if (mIs_Replay)
		return true;

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Time_Segment_Start };

	evt.level() = 0.0;
	evt.device_time() = mCurrent_Time;
	evt.signal_id() = Invalid_GUID;
	evt.segment_id() = mSegment_Id;
	evt.device_id() = game_wrapper_id;

	auto rc = Succeeded(Inject_Event(std::move(evt)));
	if (!rc)
		return false;

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

	// on replay, store levels to be picked up by another thread
	if (mIs_Replay && evt.event_code() == scgms::NDevice_Event_Code::Level)
	{
		std::unique_lock<std::mutex> lck(mReplay_Step_Mtx);

		while (mPending_Signal && mExecutor)
			mReplay_Step_Cv.wait(lck);

		mPending_Signal = true;
		mPending_Replay_Time = evt.device_time();
		mPending_Replay_Level = evt.level();
		mPending_Replay_Id = evt.signal_id();

		mReplay_Step_Cv.notify_one();
	}

	if (mIs_Replay && evt.event_code() == scgms::NDevice_Event_Code::Shut_Down)
	{
		std::unique_lock<std::mutex> lck(mReplay_Step_Mtx);

		mReplay_Ended = true;
		mReplay_Step_Cv.notify_all();
	}

	// UDevice_Event destructor does this for us
	//event->Release();

	return S_OK;
}

bool CGame_Wrapper::Inject_Configuration_Info()
{
	std::unique_lock<std::mutex> lck(mExecution_Mtx);

	std::wstring infoContents = L"Config_ID=" + GUID_To_WString(mConfig_GUID) + L",Parameters_ID=" + GUID_To_WString(mParameters_GUID);

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Information };
	evt.device_id() = game_wrapper_id;
	evt.device_time() = mCurrent_Time;
	evt.segment_id() = mSegment_Id;
	evt.info.set(infoContents.c_str());

	return Succeeded(Inject_Event(std::move(evt)));
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

bool CGame_Wrapper::Replay_Step(GUID& id, double& level, double& time)
{
	if (!mIs_Replay || !mExecutor || mReplay_Ended)
		return false;

	// lock scope
	{
		std::unique_lock<std::mutex> lck(mReplay_Step_Mtx);

		while (!mPending_Signal && mExecutor && !mReplay_Ended)
			mReplay_Step_Cv.wait(lck);

		if (!mPending_Signal)
			return false;

		mPending_Signal = false;
		time = mPending_Replay_Time;
		level = mPending_Replay_Level;
		id = mPending_Replay_Id;

		mReplay_Step_Cv.notify_one();
	}

	return true;
}

bool CGame_Wrapper::Inject_Level(GUID* signal_id, double level, double relative_step_time)
{
	std::unique_lock<std::mutex> lck(mExecution_Mtx);

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	// ensure non-negative time; negative times may result in rejection by the discrete model
	relative_step_time = std::max(0.0, relative_step_time);

	evt.level() = level;
	evt.device_time() = mCurrent_Time + mStep_Size * relative_step_time;
	evt.signal_id() = *signal_id;
	evt.segment_id() = mSegment_Id;
	evt.device_id() = game_wrapper_id;

	return Succeeded(Inject_Event(std::move(evt)));
}

void CGame_Wrapper::Terminate(const BOOL wait_for_shutdown)
{
	//Inject_Configuration_Info();
	if (!mExecutor)
		return;

	bool rc = true;

	std::unique_lock<std::mutex> lck(mExecution_Mtx);

	if (!mIs_Replay)
	{

		scgms::UDevice_Event evt_stop{ scgms::NDevice_Event_Code::Time_Segment_Stop };

		evt_stop.level() = 0.0;
		evt_stop.device_time() = mCurrent_Time;
		evt_stop.signal_id() = Invalid_GUID;
		evt_stop.segment_id() = mSegment_Id;
		evt_stop.device_id() = game_wrapper_id;

		rc = Succeeded(Inject_Event(std::move(evt_stop)));
	}

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Shut_Down };

	evt.device_time() = mCurrent_Time;
	evt.segment_id() = mSegment_Id;
	evt.device_id() = game_wrapper_id;

	rc &= Succeeded(Inject_Event(std::move(evt)));

	mExecutor->Terminate(wait_for_shutdown);

	mExecutor.reset();
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

extern "C" scgms_game_wrapper_t IfaceCalling scgms_game_replay_create(const char* log_file_path)
{
	std::unique_ptr<CGame_Wrapper> wrapper = std::make_unique<CGame_Wrapper>(0);

	if (!wrapper->Load_Replay_Configuration(log_file_path))
		return nullptr;

	if (!wrapper->Execute_Configuration())
		return nullptr;

	auto res = wrapper.get();
	wrapper.release();
	return res;
}

extern "C" BOOL IfaceCalling scgms_game_step(scgms_game_wrapper_t wrapper_raw, GUID* input_signal_ids, double* input_signal_levels, double* input_signal_times, uint32_t input_signal_count, double* bg, double* ig, double* iob, double* cob)
{
	CGame_Wrapper* wrapper = dynamic_cast<CGame_Wrapper*>(wrapper_raw);
	if (!wrapper)
		return FALSE;

	// sort inputs by time, so the model gets stepped correctly
	std::vector<size_t> input_indices(input_signal_count);
	if (input_signal_count > 0) {
		std::generate(input_indices.begin(), input_indices.end(), [n = 0]() mutable {
			return n++;
		});

		std::sort(input_indices.begin(), input_indices.end(), [&input_signal_times](size_t a, size_t b) {
			return input_signal_times[a] < input_signal_times[b];
		});
	}

	for (uint32_t i = 0; i < input_signal_count; i++)
	{
		if (!wrapper->Inject_Level(&input_signal_ids[input_indices[i]], input_signal_levels[input_indices[i]], input_signal_times[input_indices[i]]))
			return FALSE;
	}

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

extern "C" BOOL IfaceCalling scgms_game_replay_step(scgms_game_wrapper_t wrapper_raw, GUID * signal_id, double* level, double* time)
{
	CGame_Wrapper* wrapper = dynamic_cast<CGame_Wrapper*>(wrapper_raw);
	if (!wrapper)
		return FALSE;

	return wrapper->Replay_Step(*signal_id, *level, *time) ? TRUE : FALSE;
}

extern "C" BOOL IfaceCalling scgms_game_get_additional_state(scgms_game_wrapper_t wrapper, GUID * requested_signal_ids, double* output_signal_levels, size_t signal_count)
{
	// TODO

	return FALSE;
}

extern "C" BOOL IfaceCalling scgms_game_terminate(scgms_game_wrapper_t wrapper_raw)
{
	CGame_Wrapper* wrapper = dynamic_cast<CGame_Wrapper*>(wrapper_raw);
	if (!wrapper)
		return FALSE;

	wrapper->Terminate(TRUE);

	return TRUE;
}
