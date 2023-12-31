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

#include <string>
#include <cstdint>
#include <functional>
#include <scgms/rtl/guid.h>

enum class NConfig_Builder_Purpose
{
	Gameplay,
	Optimalization,
	Replay,
};

enum class NConfig_Meta
{
	None,
	Param_Opt_Filter,
};

extern const GUID& Get_Config_Base_GUID(uint32_t configClass, uint32_t configId);
extern const GUID& Get_Config_Parameters_GUID(uint32_t configClass, uint32_t configId);

extern std::string Get_Replay_Config(const std::string& logFilenameIn);
extern std::string Get_Config(const GUID& base_id, const GUID& parameters_id, double stepping, const std::string& logFilenameIn, const std::string& logFilenameOut, NConfig_Builder_Purpose purpose = NConfig_Builder_Purpose::Gameplay, std::function<void(size_t, NConfig_Meta, const std::string&)> metaCallback = {});
