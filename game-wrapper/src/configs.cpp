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

#include "configs.h"
#include <scgms/utils/string_utils.h>
#include <scgms/rtl/rattime.h>

#include <vector>
#include <sstream>
#include <map>
#include <iomanip>
#include <string>
#include <string_view>

namespace patients
{
	static const GUID patient_s2013_1 = { 0x7e685b57, 0x8ef2, 0x4ce5, { 0x93, 0x8b, 0x7b, 0xdf, 0x2a, 0x88, 0x6c, 0x89 } }; // {7E685B57-8EF2-4CE5-938B-7BDF2A886C89}
	const char* rsParams_s2013_1 = "0 0 0 20 20 0 0 0 0 0 0 0 20 10 30 10.2 0 1 0 0.2 0.05 0.4 0.005 0.005 100 0.01 0.005 0.001 0.01 0.05 0.001 0.05 0.0001 0.0001 0.002 0.05 1e-05 20 0.1 0.2 0.002 0.008 0.0001 0.0005 0.05 1 0.01 0.01 0.01 1 0 0 0 0 0 0 0 0 0 1 1 0 0 0 265.37 162.457 5.50433 0 100.25 100.25 3.20763 72.4342 141.154 265.37 102.32 138.56 100.25 0.08906 0.046122 0.003793 0.70391 0.21057 1.9152 0.054906 0.031319 253.52 0.087114 0.058138 0.027802 0.15446 0.225027 0.09001099999999999 0.23169 0.004637 0.00469 0.01208 0.9 0.0005 339 1 3.26673 0.0152 0.0766 0.0019 0.0078 1.23862 4.73141 0.05 0.05 0.05 10 0.95 0.12 0.4 0.3 0.08 0.02 0.05 30 0 15 15 500 500 500 500 500 50 300 200 300 200 500 500 500 250 300 200 0.8 1 0.5 2 2 10 2 0.5 500 0.6 0.2 0.2 0.9 1 1 1 0.05 0.02 0.5 3 0.01 1000 5 20 0.8 0.9 0.05 0.1 10 20 1 1 1 100 3 1 2 2 1 0.8 1 200 200 100 100";

	static const GUID patient_gct_1 = { 0x87d58e17, 0x3968, 0x45eb, { 0x90, 0xb9, 0x47, 0x17, 0xc3, 0xc8, 0x6, 0x9f } }; // {87D58E17-3968-45EB-90B9-4717C3C8069F}
	const char* rsParams_GCT_1 = "0 0 0 0 0 0 0 0 30 25 30 50 8 4 0.01 1.4 1.4 0.001 14 14 0.144 0.144 0.001 0.01 1e-05 0.0001 0 2000 500 2000 500 300 100 0.5 0.006944444444444445 0.003472222222222222 135 65 450 4e-12 16.2 2.6 0 54.8 30 25 80 240 8 5 0.01 8 3.4 0.07697 144 144 0.38519 0.38519 0.8 0.01 0.01 0.1 0 9000 2000 7425 2000 1800 300 0.98 0.03055555555555556 0.08263888888888889 500 500 500 500 500 500 200 200 60 60 80 1000 14 8 0.9 24 144 3 144 144 14.4 14.4 0.8 2 0.01 0.1 0.05 12000 12000 9000 9000 2500 1800 0.98 0.03472222222222222 0.08333333333333334";

	const std::map<GUID, std::string> mapping = {
		{ patient_s2013_1, rsParams_s2013_1 },

		{ patient_gct_1, rsParams_GCT_1 },
	};
}

namespace configs
{
	/**
	 * When specifying config, make sure it outputs the following signals: BG, IG, IOB and COB.
	 * Also keep in mind, that we want to specify model stepping and patient parameters
	 **/

	const char* rsPatient_Params_Placeholder = "{{PatientParameters}}";
	const char* rsPatient_Model_Stepping_Placeholder = "{{PatientStepping}}";
	const char* rsLog_File_Source_Placeholder = "{{LogFileSource}}";
	const char* rsLog_File_Target_Placeholder = "{{LogFileTarget}}";
	const char* rsFilter_Pos_Placeholder = "{{FilterIdx}}";

	const char* rsMeta_Filter_Marker = ";META:";
	const char  rsMeta_Delimiter = ',';
	const char  rsMeta_Value_Delimiter = ':';
	const char* rsMeta_Gameplay = "GAMEPLAY";
	const char* rsMeta_Optimalization = "OPTIMALIZATION";
	const char* rsMeta_Replay = "REPLAY";
	const char* rsMeta_All_Modes = "ALL";
	const char* rsMeta_Opt_Filter = "OPTFILTER";

	const char* rsFilter_Tag_Start = "[Filter_";

	const char* rsConfig_Replay_Only = R"CONFIG(
; CSV File Log Replay
[Filter_{{FilterIdx}}_{172EA814-9DF1-657C-1289-C71893F1D085}]
Log_File = {{LogFileSource}}
Emit_Shutdown = true
Filename_as_segment_id = false
Emit_All_Events_Before_Shutdown = true
)CONFIG";

	static const GUID config_s2013_1 = { 0xca90e215, 0x2d10, 0x4879, { 0xb2, 0x4f, 0x31, 0x37, 0xc3, 0x6e, 0x59, 0xe2 } }; // {CA90E215-2D10-4879-B24F-3137C36E59E2}
	const char* rsConfig_s2013_1 = R"CONFIG(

; CSV File Log Replay
;META:OPTIMALIZATION,REPLAY
[Filter_{{FilterIdx}}_{172EA814-9DF1-657C-1289-C71893F1D085}]
Log_File = {{LogFileSource}}
Emit_Shutdown = true
Filename_as_segment_id = false
Emit_All_Events_Before_Shutdown = false

; Signal mapping
;META:OPTIMALIZATION,REPLAY
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; BG
Signal_Src_Id = {F666F6C2-D7C0-43E8-8EE1-C8CAA8F860E5}
; Null (discards the events)
Signal_Dst_Id = {706E7FDB-8F22-486F-BFA5-6A56D3514209}

; Signal mapping
;META:OPTIMALIZATION,REPLAY
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; IG
Signal_Src_Id = {3034568D-F498-455B-AC6A-BCF301F69C9E}
; Null (discards the events)
Signal_Dst_Id = {706E7FDB-8F22-486F-BFA5-6A56D3514209}

; Signal mapping
;META:OPTIMALIZATION,REPLAY
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; IOB
Signal_Src_Id = {313A1C11-6BAC-46E2-8938-7353409F2FCD}
; Null (discards the events)
Signal_Dst_Id = {706E7FDB-8F22-486F-BFA5-6A56D3514209}


; Signal mapping
;META:OPTIMALIZATION,REPLAY
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; COB
Signal_Src_Id = {B74AA581-538C-4B30-B764-5BD0D97B0727}
; Null (discards the events)
Signal_Dst_Id = {706E7FDB-8F22-486F-BFA5-6A56D3514209}

; Signal mapping
;META:OPTIMALIZATION,REPLAY
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; rescue carbs
Signal_Src_Id = {F24920F7-3F7B-4000-B2D0-374F940E4898}
; Null (discards the events)
Signal_Dst_Id = {706E7FDB-8F22-486F-BFA5-6A56D3514209}


; Signal mapping
;META:OPTIMALIZATION,REPLAY
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; requested insulin basal rate
Signal_Src_Id = {B5897BBD-1E32-408A-A0D5-C5BFECF447D9}
; Null (discards the events)
Signal_Dst_Id = {706E7FDB-8F22-486F-BFA5-6A56D3514209}

; Signal mapping
;META:OPTIMALIZATION,REPLAY
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; requested insulin bolus
Signal_Src_Id = {09B16B4A-54C2-4C6A-948A-3DEF8533059B}
; Null (discards the events)
Signal_Dst_Id = {706E7FDB-8F22-486F-BFA5-6A56D3514209}

; Signal mapping
;META:OPTIMALIZATION,REPLAY
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; delivered insulin basal rate
Signal_Src_Id = {BF88A8CB-1290-4477-A2CF-BDD06DF628AB}
; Null (discards the events)
Signal_Dst_Id = {706E7FDB-8F22-486F-BFA5-6A56D3514209}

; Signal mapping
;META:OPTIMALIZATION,REPLAY
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; delivered insulin bolus
Signal_Src_Id = {22D87566-AF1B-4CC7-8D11-C5E04E1E9C8A}
; Null (discards the events)
Signal_Dst_Id = {706E7FDB-8F22-486F-BFA5-6A56D3514209}


; Signal generator
[Filter_{{FilterIdx}}_{9EEB3451-2A9D-49C1-BA37-2EC0B00E5E6D}]
Model = {B387A874-8D1E-460B-A5EC-BA36AB7516DE}
Feedback_Name = fb1
Synchronize_To_Signal = true
Synchronization_Signal = {FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF}
Time_Segment_Id = 1
Stepping = {{PatientStepping}}
Maximum_Time = 00:00:00
Shutdown_After_Last = false
Echo_Default_Parameters_As_Event = false
Individualize_Segment_Specific_Parameters = false
Parameters = {{PatientParameters}}

; Signal mapping
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; S2013 IG
Signal_Src_Id = {55B07D3D-0D99-47D0-8A3B-3E543C25E5B1}
; IG
Signal_Dst_Id = {3034568D-F498-455B-AC6A-BCF301F69C9E}


; Signal mapping
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; S2013 BG
Signal_Src_Id = {1EEE155A-9150-4958-8AFD-3161B73CF9FC}
; BG
Signal_Dst_Id = {F666F6C2-D7C0-43E8-8EE1-C8CAA8F860E5}


; Signal mapping
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; S2013 delivered insulin
Signal_Src_Id = {AA402CE3-BA4A-457B-AA19-1B908B9B53C4}
; delivered insulin
Signal_Dst_Id = {EE655943-06BF-4F9D-B27D-AACB3943FB91}

; Calculated signal
[Filter_{{FilterIdx}}_{14A25F4C-E1B1-85C4-1274-9A0D11E09813}]
; IOB
Model = {D3D57CB4-48DA-40E2-9E53-BB1E848A6395}
; IOB exponential
Signal = {238D2353-6D37-402C-AF39-6C5552A77E1F}

; 00:00:00
Prediction_Window = 00:00:00
Solve_Parameters = false
Solver = {00000000-0000-0000-0000-000000000000}
Model_Bounds = 0.02083333333333334 0 0.05208333333333334 0.125 1 0
Solve_On_Level_Count = 0
Solve_On_Calibration = false
Solve_On_Time_Segment_End = false
Solve_Using_All_Segments = false
Metric = {00000000-0000-0000-0000-000000000000}
Levels_Required = 0
Measured_Levels = false
Relative_Error = false
Squared_Diff = false
Prefer_More_Levels = false
Metric_Threshold = 0


; Calculated signal
[Filter_{{FilterIdx}}_{14A25F4C-E1B1-85C4-1274-9A0D11E09813}]
; COB
Model = {E63C23E4-7932-4C47-9CEA-A7A67F751723}
; COB bilinear
Signal = {E29A9D38-551E-4F3F-A91D-1F14D93467E3}

Prediction_Window = 00:00:00
Solve_Parameters = false
Solver = {00000000-0000-0000-0000-000000000000}
Model_Bounds = 0.02083333333333334 0 0.05208333333333334 0.125 1 0
Solve_On_Level_Count = 0
Solve_On_Calibration = false
Solve_On_Time_Segment_End = false
Solve_Using_All_Segments = false
Metric = {00000000-0000-0000-0000-000000000000}
Levels_Required = 0
Measured_Levels = false
Relative_Error = false
Squared_Diff = false
Prefer_More_Levels = false
Metric_Threshold = 0


; Signal mapping
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; IOB exponential
Signal_Src_Id = {238D2353-6D37-402C-AF39-6C5552A77E1F}
; IOB
Signal_Dst_Id = {313A1C11-6BAC-46E2-8938-7353409F2FCD}


; Signal mapping
[Filter_{{FilterIdx}}_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; COB bilinear
Signal_Src_Id = {E29A9D38-551E-4F3F-A91D-1F14D93467E3}
; COB
Signal_Dst_Id = {B74AA581-538C-4B30-B764-5BD0D97B0727}


; Signal generator
;META:OPTIMALIZATION,REPLAY,OPTFILTER:Parameters
[Filter_{{FilterIdx}}_{9EEB3451-2A9D-49C1-BA37-2EC0B00E5E6D}]
; Icarus AI rates_pack_boluses
Model = {A682F21E-9F52-41E6-9052-23CE44C75335}
Feedback_Name = 
Synchronize_To_Signal = true
; All signals
Synchronization_Signal = {3034568D-F498-455B-AC6A-BCF301F69C9E}
Time_Segment_Id = 0
; 00:00:00
Stepping = 0
; 00:00:00
Maximum_Time = 0
Shutdown_After_Last = false
Echo_Default_Parameters_As_Event = false
Parameters = 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1.5 0.253968 10 0.5079359999999999 10 0.7619039999999999 10 1.015872 10 1.26984 10 1.523808 10 1.777776 10 2.031744 10 5 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100
Parameters = 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1.5 3 0.02083333333333334 0 10 0.2608506944444445 10 0.521701388888889 10 0.7825520833333335 10 1.043402777777778 10 1.304253472222222 10 1.565104166666667 10 1.825954861111111 10 0 10 0.0521701388888889 10 0.1043402777777778 10 0.1565104166666667 10 0.2086805555555556 10 0.2608506944444445 10 0.3130208333333334 10 0.3651909722222223 10 0.4173611111111112 10 0.4695312500000001 10 0.521701388888889 10 0.5738715277777778 10 0.6260416666666668 10 0.6782118055555557 10 0.7303819444444446 10 0.7825520833333335 10 0.8347222222222224 10 0.8868923611111112 10 0.9390625000000001 10 0.991232638888889 10 1.043402777777778 10 1.095572916666667 10 1.147743055555556 10 1.199913194444445 10 1.252083333333334 10 1.304253472222223 10 1.356423611111111 10 1.40859375 10 1.460763888888889 10 1.512934027777778 10 1.565104166666667 10 1.617274305555556 10 1.669444444444445 10 1.721614583333334 10 1.773784722222222 10 1.825954861111111 10 1.878125 10 1.930295138888889 10 1.982465277777778 10 2.034635416666667 10 10 5 0.08333333333333333 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100 2.086805555555556 100



; Signal feedback
;META:OPTIMALIZATION,REPLAY
[Filter_{{FilterIdx}}_{5D29EA43-4FAC-4141-A03F-733B10296727}]
; requested insulin basal rate
Signal_Src_Id = {B5897BBD-1E32-408A-A0D5-C5BFECF447D9}
Remove_from_source = false
Feedback_Name = fb1


; Signal feedback
;META:OPTIMALIZATION,REPLAY
[Filter_{{FilterIdx}}_{5D29EA43-4FAC-4141-A03F-733B10296727}]
; requested insulin bolus
Signal_Src_Id = {09B16B4A-54C2-4C6A-948A-3DEF8533059B}
Remove_from_source = false
Feedback_Name = fb1


; Calculated signal
[Filter_{{FilterIdx}}_{14A25F4C-E1B1-85C4-1274-9A0D11E09813}]
Model = {637465FB-FB6F-4A05-BB13-AB2A59B9774B}
Signal = {0FC616DD-07BF-49EA-BF2C-FCA139429F05}
Prediction_Window = 00:00:00
Solve_Parameters = false
Solver = {00000000-0000-0000-0000-000000000000}
Model_Bounds = 0 6.66 100
Solve_On_Level_Count = 0
Solve_On_Calibration = false
Solve_On_Time_Segment_End = false
Solve_Using_All_Segments = false
Metric = {00000000-0000-0000-0000-000000000000}
Levels_Required = 0
Measured_Levels = false
Relative_Error = false
Squared_Diff = false
Prefer_More_Levels = false
Metric_Threshold = 0

; Signal error
[Filter_{{FilterIdx}}_{690FBC95-84CA-4627-B47C-9955EA817A4F}]
Description = Treatment_efficiency
Reference_Signal = {3034568D-F498-455B-AC6A-BCF301F69C9E}
Error_Signal = {0FC616DD-07BF-49EA-BF2C-FCA139429F05}
; average error
Metric = {D272A84D-50FF-46CE-977E-C8E368C3706A}
Levels_Required = 100
Relative_Error = false
Squared_Diff = false
Prefer_More_Levels = false
Metric_Threshold = 0
Emit_Metric_As_Signal = false
Emit_Last_Value_Only = false
Output_CSV_file = $([[maybe_unused]])

; Log
;META:GAMEPLAY,REPLAY
[Filter_{{FilterIdx}}_{C0E942B9-3928-4B81-9B43-A347668200BA}]
Log_File = {{LogFileTarget}}
Log_Segments_Individually = false
)CONFIG";

	static const GUID config_gct_1 = { 0x5e9ea84a, 0x3d39, 0x4f27, { 0xb3, 0x2a, 0x28, 0xea, 0xf7, 0x11, 0xea, 0xc7 } };// {5E9EA84A-3D39-4F27-B32A-28EAF711EAC7}
	const char* rsConfig_gct_1 = R"CONFIG(
; Signal generator
[Filter_001_{9EEB3451-2A9D-49C1-BA37-2EC0B00E5E6D}]
; GCT model
Model = {C91E7DEB-0285-4FA0-831E-94F0F1A0962E}
Feedback_Name = fb1
Synchronize_To_Signal = true
Synchronization_Signal = {FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF}
Time_Segment_Id = 1
Stepping = {{PatientStepping}}
Maximum_Time = 00:00:00
Shutdown_After_Last = false
Echo_Default_Parameters_As_Event = true
Parameters = {{PatientParameters}}


; Signal mapping
[Filter_002_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; GCT model - BG
Signal_Src_Id = {B45606DE-3F1B-4EC4-BB83-2EE99EC83139}
; Blood glucose
Signal_Dst_Id = {F666F6C2-D7C0-43E8-8EE1-C8CAA8F860E5}


; Signal mapping
[Filter_003_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; GCT model - IG
Signal_Src_Id = {C4D0FA39-120A-49B1-8677-D4E9CD5D59F9}
; Interstitial glucose
Signal_Dst_Id = {3034568D-F498-455B-AC6A-BCF301F69C9E}


; Signal mapping
[Filter_004_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; GCT model - COB
Signal_Src_Id = {B23E1DF5-D291-4015-A03D-A92EC3F13C16}
; COB
Signal_Dst_Id = {B74AA581-538C-4B30-B764-5BD0D97B0727}


; Signal mapping
[Filter_005_{8FAB525C-5E86-AB81-12CB-D95B1588530A}]
; GCT model - IOB
Signal_Src_Id = {FE49FE8E-F819-4323-B84D-7F0901E4A271}
; IOB
Signal_Dst_Id = {313A1C11-6BAC-46E2-8938-7353409F2FCD}


; Log
[Filter_006_{C0E942B9-3928-4B81-9B43-A347668200BA}]
Log_File = {{LogFileTarget}}
)CONFIG";

	const std::map<GUID, std::string> mapping = {
		{ config_s2013_1, rsConfig_s2013_1 },
		{ config_gct_1, rsConfig_gct_1 },
	};

}

bool Match_Replace_And_Advance(const char** itr, std::ostringstream& oss, const char* needle, const std::string& replaceWith)
{
	if (strncmp(*itr, needle, strlen(needle)) == 0)
	{
		oss << replaceWith;
		*itr += strlen(needle);

		return true;
	}

	return false;
}

bool Match_And_Advance(const char** itr, const char* needle)
{
	if (strncmp(*itr, needle, strlen(needle)) == 0)
	{
		*itr += strlen(needle);

		return true;
	}

	return false;
}

bool Match(const char** itr, const char* needle)
{
	return (strncmp(*itr, needle, strlen(needle)) == 0);
}

inline void Discard_Rest_Of_Line(const char** itr)
{
	// read until the end of line (or end of input)
	while (**itr != '\0' && **itr != '\r' && **itr != '\n')
		(*itr)++;

	// read while there are just new lines
	while (**itr == '\r' || **itr == '\n')
		(*itr)++;
}

std::string Read_Rest_Of_Line(const char** itr)
{
	const char* begin = *itr;
	// read until the end of line (or end of input)
	while (**itr != '\0' && **itr != '\r' && **itr != '\n')
		(*itr)++;

	const char* end = *itr;

	// read while there are just new lines
	while (**itr == '\r' || **itr == '\n')
		(*itr)++;

	return std::string{ begin, end };
}

std::map<std::string, std::string> Parse_Meta_String(const std::string& str)
{
	std::map<std::string, std::string> res;

	std::istringstream iss(str);
	std::string line;
	while (std::getline(iss, line, configs::rsMeta_Delimiter))
	{
		auto delimpos = line.find(configs::rsMeta_Value_Delimiter);
		if (delimpos == std::string::npos)
			res[line] = "";
		else
			res[line.substr(0, delimpos)] = line.substr(delimpos + 1);
	}

	return res;
}

const GUID& Get_Config_Base_GUID(uint32_t configClass, uint32_t configId)
{
	switch (configClass)
	{
		case 1:		// Ikaros - S2013, easy
		case 2:		// Ikaros - S2013, medium
		case 3:		// Ikaros - S2013, hard
			return configs::config_s2013_1;

		case 4:		// Ikaros - GCT, easy
		case 5:		// Ikaros - GCT, medium
		case 6:		// Ikaros - GCT, hard
			return configs::config_gct_1;
	}

	return Invalid_GUID;
}

const GUID& Get_Config_Parameters_GUID(uint32_t configClass, uint32_t configId)
{
	switch (configClass)
	{
		case 1:		// Ikaros - S2013, easy
		{
			switch (configId)
			{
				case 1:
					return patients::patient_s2013_1;
			}
			break;
		}
		case 2:		// Ikaros - S2013, medium
		{
			// TODO
			break;
		}
		case 3:		// Ikaros - S2013, hard
		{
			// TODO
			break;
		}

		case 4:		// Ikaros - GCT, easy
		{
			switch (configId)
			{
				case 1:
					return patients::patient_gct_1;
			}
			break;
		}
		case 5:		// Ikaros - GCT, medium
		{
			// TODO
			break;
		}
		case 6:		// Ikaros - GCT, hard
		{
			// TODO
			break;
		}
	}

	return Invalid_GUID;
}

static inline void Build_Filter_Idx_Str(size_t idx, std::string& target)
{
	std::ostringstream oss;
	oss << std::setw(3) << std::setfill('0') << idx;

	target = oss.str();
}

enum class NDiscard_State
{
	No_Discard,
	Follow_Up,
	Discard,
};

static std::string Build_Config_From_Template(const char* citr, const std::string& patientParams, const double stepping, const std::string& logFilenameIn, const std::string& logFilenameOut, NConfig_Builder_Purpose purpose, std::function<void(size_t, NConfig_Meta, const std::string&)> metaCallback = {})
{
	std::ostringstream oss;

	const std::string patientStepping = Narrow_WString(Rat_Time_To_Default_WStr(stepping));

	size_t curFilterIdx = 1;
	std::string curFilterIdxStr;

	Build_Filter_Idx_Str(curFilterIdx, curFilterIdxStr);

	bool freshNewLine = true;
	NDiscard_State discardState = NDiscard_State::No_Discard;

	auto metaStrToEnum = [](const std::string& str) {

		if (str == configs::rsMeta_Opt_Filter)
			return NConfig_Meta::Param_Opt_Filter;

		return NConfig_Meta::None;
	};

	while (*citr != '\0')
	{
		if (freshNewLine)
		{
			// is a comment (may be meta comment); either way, remove the comment entirely
			if (*citr == ';')
			{
				// meta marker
				if (Match_And_Advance(&citr, configs::rsMeta_Filter_Marker))
				{
					auto metastr = Read_Rest_Of_Line(&citr);
					auto metas = Parse_Meta_String(metastr);

					if (metas.find(configs::rsMeta_All_Modes) != metas.end())
						discardState = NDiscard_State::No_Discard;
					else if ((metas.find(configs::rsMeta_Gameplay) == metas.end() && purpose == NConfig_Builder_Purpose::Gameplay)
						|| (metas.find(configs::rsMeta_Optimalization) == metas.end() && purpose == NConfig_Builder_Purpose::Optimalization))
						discardState = NDiscard_State::Follow_Up;
					else
						discardState = NDiscard_State::No_Discard;

					if (metaCallback)
					{
						for (auto& m : metas)
						{
							auto en = metaStrToEnum(m.first);
							if (en != NConfig_Meta::None)
								metaCallback(curFilterIdx - 1, en, m.second);
						}
					}
				}
				else
					Discard_Rest_Of_Line(&citr);

				continue;
			}
			else if (Match(&citr, configs::rsFilter_Tag_Start))
			{
				if (discardState == NDiscard_State::Follow_Up)
					discardState = NDiscard_State::Discard;
				else
					discardState = NDiscard_State::No_Discard;
			}
			else
				freshNewLine = false;
		}

		if (discardState == NDiscard_State::No_Discard)
		{
			// placeholder begin markers
			if (*citr == '{' && *(citr + 1) == '{')
			{
				if (Match_Replace_And_Advance(&citr, oss, configs::rsPatient_Params_Placeholder, patientParams))
					continue;

				if (Match_Replace_And_Advance(&citr, oss, configs::rsLog_File_Target_Placeholder, logFilenameOut))
					continue;

				if (Match_Replace_And_Advance(&citr, oss, configs::rsLog_File_Source_Placeholder, logFilenameIn))
					continue;

				if (Match_Replace_And_Advance(&citr, oss, configs::rsPatient_Model_Stepping_Placeholder, patientStepping))
					continue;

				// replace filter idx placeholder with newly evaluated index
				if (Match_Replace_And_Advance(&citr, oss, configs::rsFilter_Pos_Placeholder, curFilterIdxStr))
				{
					curFilterIdx++;
					Build_Filter_Idx_Str(curFilterIdx, curFilterIdxStr);
					continue;
				}
			}

			oss << *citr;
		}

		if (*citr == '\r' || *citr == '\n')
			freshNewLine = true;
		else
			freshNewLine = false;

		citr++;
	}

	return oss.str();
}

std::string Get_Replay_Config(const std::string& logFilenameIn)
{
	return Build_Config_From_Template(configs::rsConfig_Replay_Only, "", 0.0, logFilenameIn, logFilenameIn, NConfig_Builder_Purpose::Replay);
}

std::string Get_Config(const GUID& base_id, const GUID& parameters_id, double stepping, const std::string& logFilenameIn, const std::string& logFilenameOut, NConfig_Builder_Purpose purpose, std::function<void(size_t, NConfig_Meta, const std::string&)> metaCallback)
{
	auto conf_itr = configs::mapping.find(base_id);
	if (conf_itr == configs::mapping.end())
		return "";

	auto param_itr = patients::mapping.find(parameters_id);
	if (param_itr == patients::mapping.end())
		return "";

	const std::string& patientParams = param_itr->second;

	const char* citr = conf_itr->second.c_str();

	return Build_Config_From_Template(citr, patientParams, stepping, logFilenameIn, logFilenameOut, purpose, metaCallback);
}
