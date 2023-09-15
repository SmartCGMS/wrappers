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

#pragma once

#include <scgms/iface/FilterIface.h>
#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/scgmsLib.h>
#include <scgms/iface/referencedIface.h>
#include <scgms/iface/SolverIface.h>
#include <scgms/rtl/UILib.h>
#include <scgms/rtl/SolverLib.h>

#include <cstdint>
#include <cmath>
#include <limits>
#include <mutex>
#include <thread>

// enumeration of optimalization states
enum class NGame_Optimize_State : size_t
{
	None		= 0,	// optimalization hasn't started yet; this should never be exported through library interface, as this state gets immediatelly replaced by either Running of Failed state
	Running		= 1,	// in progress
	Success		= 2,	// finished successfully
	Failed		= 3,	// failed, unable to find parameters

	count
};

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Game optimizer wrapper to split back-end logic (how to properly handle optimalization) from front-end
 */
class CGame_Optimizer_Wrapper : public refcnt::CNotReferenced
{
	private:
		// default step size, should be the same as in the original game run
		double mStep_Size = 0;

		// 0 - 100 (in percents of recommended pop size / generation count
		uint16_t mDegree_Of_Optimize = 20;

		// config prepared for optimalization
		std::string mPrepared_Config;
		// config prepared for optimalization replay (should log the outputs)
		std::string mPrepared_Config_Replay;

		// vector of optimized parameters
		std::vector<double> mOptimized_Parameters;

		// thread for optimalization
		std::unique_ptr<std::thread> mOpt_Thread;

		// stored solver progress
		solver::TSolver_Progress mProgress;

		// optimalization progress state
		NGame_Optimize_State mOpt_State;

		// identifier of optimized index in optimalization config
		size_t mOpt_Filter_Idx = 0;
		// identifier of optimized index in replay config
		size_t mOpt_Filter_Replay_Idx = 0;
		// name of parameter set in configuration
		std::string mOpt_Filter_Parameters_Name = "";

	protected:
		// thread for optimizer
		void Optimizer_Thread_Fnc();

	public:
		CGame_Optimizer_Wrapper(uint32_t stepping_ms, uint16_t degree_of_opt);

		// loads configuration based on given parameters - loads game log from input path, stores optimized gameplay to output path
		bool Load_Configuration(uint16_t config_class, uint16_t config_id, const std::string& log_file_input_path, const std::string& log_file_output_path);

		// starts the optimalization
		bool Start();

		// retrieves progress from internal container
		NGame_Optimize_State Get_Progress(double& pct);

		// replays the optimized config; this assumes the optimalization process was successfull
		bool Replay();

		// cancels the optimalization at the closest cancel point
		bool Request_Cancel();
};

#pragma warning( pop )

// a type for interop-exportable pointer to CGame_Optimizer_Wrapper instance; the pointer should never be dereferenced in outer code as the CGame_Wrapper class is not designed to be interoperable
using scgms_game_optimizer_wrapper_t = CGame_Optimizer_Wrapper*;

/*
 * scgms_game_optimize
 *
 * Optimizes the parameters of given configuration based on given logfile
 *
 * Parameters:
 *		config_class - class of config to be used for optimalization
 *		config_id - identifier of config within given class
 *		stepping_ms - stepping of whole model in milliseconds
 *		log_file_input_path - path to input log (to be replayed in order to optimize)
 *		log_file_output_path - path to output (where the optimized gameplay should be stored)
 *		degree_of_opt - degree of optimalization (generations count); the higher value, the longer it takes, but the better the result should be
 *
 * Return values:
 *		<a valid scgms_game_optimizer_wrapper_t pointer> - success
 *		nullptr - failure
 */
extern "C" scgms_game_optimizer_wrapper_t IfaceCalling scgms_game_optimize(uint16_t config_class, uint16_t config_id, uint32_t stepping_ms,
	const char* log_file_input_path, const char* log_file_output_path, uint16_t degree_of_opt);

/*
 * scgms_game_get_optimize_status
 *
 * Retrieves the current state of running optimalization
 *
 * Parameters:
 *		wrapper - pointer to a game optimizer wrapper instance obtained from scgms_game_optimize call
 *		state - output variable for current optimizer state enum value
 *		progress_pct - output variable for current progress (0 - 1)
 *
 * Return values:
 *		TRUE (non-zero) - success, state retrieved successfully
 *		FALSE (zero) - failure
 */
extern "C" BOOL IfaceCalling scgms_game_get_optimize_status(scgms_game_optimizer_wrapper_t wrapper, NGame_Optimize_State* state, double* progress_pct);

/*
 * scgms_game_cancel_optimize
 * 
 * Cancels the optimalization at the closest cancel point (e.g.; in between generations of a genetic algorithm, ...)
 * Optionally waits for the optimalization to be cancelled before returning to caller.
 * 
 * Parameters:
 *		wrapper - pointer to a game optimizer wrapper instance obtained from scgms_game_optimize call
 *		wait - TRUE (non-zero) waits for optimalization to be terminated, FALSE (zero) requests the termination and returns immediatelly
 * 
 * Return values:
 *		TRUE (non-zero) - success, termination has been requested (if wait is TRUE, the optimalization has also ended)
 *		FALSE (zero) - no optimalization is running
 */
extern "C" BOOL IfaceCalling scgms_game_cancel_optimize(scgms_game_optimizer_wrapper_t wrapper, BOOL wait);

/*
 * scgms_game_optimizer_terminate
 *
 * Terminates the optimalization; replays the optimized state and stores outputs to the log file given in initial optimize call
 *
 * Parameters:
 *		wrapper - pointer to a game optimizer wrapper instance obtained from scgms_game_optimize call
 *
 * Return values:
 *		TRUE (non-zero) - success, optimalization successfully terminated and log file stored
 *		FALSE (zero) - failure
 */
extern "C" BOOL IfaceCalling scgms_game_optimizer_terminate(scgms_game_optimizer_wrapper_t wrapper);
