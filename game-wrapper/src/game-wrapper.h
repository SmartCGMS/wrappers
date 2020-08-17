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

#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/scgmsLib.h"
#include "../../../common/iface/referencedIface.h"

#include <cstdint>
#include <cmath>
#include <limits>
#include <mutex>

struct CPatient_Sensor_State
{
	double bg = std::numeric_limits<double>::quiet_NaN();
	double ig = std::numeric_limits<double>::quiet_NaN();
	double iob = std::numeric_limits<double>::quiet_NaN();
	double cob = std::numeric_limits<double>::quiet_NaN();
};

constexpr const GUID game_wrapper_id = { 0xb01f968d, 0x5fb9, 0x426c, { 0x9d, 0x42, 0x67, 0x18, 0xaf, 0xd8, 0xaa, 0xc1 } };	// {B01F968D-5FB9-426C-9D42-6718AFD8AAC1}

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Game wrapper to split back-end logic from front-end
 */
class CGame_Wrapper : public virtual scgms::IFilter, public virtual refcnt::CNotReferenced
{
	private:
		// executor associated with this instance
		scgms::SFilter_Executor mExecutor;
		// error list, reused among runs
		refcnt::Swstr_list mErrors;
		// loaded config contents
		std::string mConfig_Contents;
		// current rat time
		double mCurrent_Time;
		// size of a single simulation step
		double mStep_Size;
		// used segment ID
		uint64_t mSegment_Id;

		// mutex for locking sections operating with execution pointer
		std::mutex mExecution_Mtx;

		// current patient state
		CPatient_Sensor_State mState;

	protected:
		// inject given event to current execution
		HRESULT Inject_Event(scgms::UDevice_Event &&event);

	public:
		CGame_Wrapper(uint32_t stepping_ms);
		virtual ~CGame_Wrapper();

		bool Load_Configuration(uint16_t config_class, uint16_t config_id, const std::string& log_file_path);
		bool Execute_Configuration();
		bool Step(bool initial = false);
		void Terminate(const BOOL wait_for_shutdown);

		bool Inject_Bolus(double level);
		bool Inject_Basal_Rate(double level);
		bool Inject_CHO(double level, bool rescue);

		const CPatient_Sensor_State& Get_State() const;

		// scgms::IFilter iface
		virtual HRESULT IfaceCalling Configure(scgms::IFilter_Configuration* configuration, refcnt::wstr_list *error_description);
		virtual HRESULT IfaceCalling Execute(scgms::IDevice_Event *event);
};

#pragma warning( pop )

// a type for interop-exportable pointer to CGame_Wrapper instance; the pointer should never be dereferenced in outer code as the CGame_Wrapper class is not designed to be interoperable
using scgms_game_wrapper_t = CGame_Wrapper*;

/*
 * scgms_game_create
 *
 * Creates game wrapper instance with given parameters.
 *
 * Parameters:
 *		config_class - category of configs to be used; this is more like an attept to split difficulties and patient types
 *		config_id - config identifier within selected config class
 *		stepping_ms - model stepping in milliseconds - subsequent scgms_game_step calls would step the model by this exact amount of milliseconds
 *		log_file_path - where to put the log file; empty or nullptr to indicate the intent to discard the log
 *
 * Return values:
 *		<valid scgms_game_wrapper_t> - success
 *		nullptr - failure
 */
extern "C" scgms_game_wrapper_t IfaceCalling scgms_game_create(uint16_t config_class, uint16_t config_id, uint32_t stepping_ms, const char* log_file_path);

/*
 * scgms_game_step
 *
 * Performs a single step within the simulation, injects all necessary events according to given parameters
 *
 * Parameters:
 *		wrapper - pointer to a game wrapper instance obtained from scgms_game_create call
 *		boluses - an array of boluses to be dosed [U]
 *		bolus_cnt - length of the bolus array
 *		carbohydrates - an array of carbohydrates to be dosed [g]
 *		cho_rescue_flags - an array of flags indicating the "rescue" CHO type (FALSE (zero) for regular CHO, TRUE (non-zero) for rescue CHO)
 *		carbohydrates_cnt - length of carbohydrates and cho_rescue_flags array
 *		basal_insulin_setting - setting of basal insulin rate - quiet_NaN indicating "do not change", zero for turning the virtual pump off, otherwise accepts positive values only
 *		bg - output variable for blood glucose reading [mmol/L]
 *		ig - output variable for interstitial glucose reading [mmol/L]
 *		iob - output variable for current model insulin on board [U]
 *		cob - output variable for current model carbohydrates on board [g]
 *
 * Return values:
 *		TRUE (non-zero) - success
 *		FALSE (zero) - failure - parameters are invalid or the attempt to step the model has failed
 */
extern "C" BOOL IfaceCalling scgms_game_step(scgms_game_wrapper_t wrapper, double *boluses, uint32_t bolus_cnt, double *carbohydrates, uint8_t *cho_rescue_flags, uint32_t carbohydrates_cnt, double basal_insulin_setting, double* bg, double* ig, double* iob, double* cob);

/*
 * scgms_game_terminate
 *
 * Terminates the running game, invalidates the obtained scgms_game_wrapper_t object. May block due to yet unprocessed events.
 *
 * Parameters:
 *		wrapper - pointer to a game wrapper instance obtained from scgms_game_create call
 *
 * Return values:
 *		TRUE (non-zero) - success
 *		FALSE (zero) - failure
 */
extern "C" BOOL IfaceCalling scgms_game_terminate(scgms_game_wrapper_t wrapper);
