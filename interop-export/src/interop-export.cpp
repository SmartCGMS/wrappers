#include "interop-export.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/rattime.h"

#include <chrono>

CInterop_Export_Filter::CInterop_Export_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput(inpipe), mOutput(outpipe), mListener(nullptr)
{
	//
}

CInterop_Export_Filter::~CInterop_Export_Filter()
{
	//
}

HRESULT IfaceCalling CInterop_Export_Filter::QueryInterface(const GUID*  riid, void ** ppvObj)
{
	if (Internal_Query_Interface<glucose::IInterop_Export_Filter_Inspection>(glucose::Interop_Export_Filter_Inspection, *riid, ppvObj))
		return S_OK;

	return E_INVALIDARG;
}

void CInterop_Export_Filter::Run_Main()
{
	for (; glucose::UDevice_Event evt = mInput.Receive(); )
	{
		// if there was a listener registered, pass the event
		if (mListener)
		{
			mListener(
				static_cast<uint8_t>(evt.event_code),
				reinterpret_cast<uint8_t*>(&evt.device_id),
				reinterpret_cast<uint8_t*>(&evt.signal_id),
				static_cast<int64_t>(Rat_Time_To_Unix_Time(evt.device_time)),
				static_cast<int32_t>(evt.level * glucose::Native_Interop_Float_Multiplier),
				evt.parameters.get(),
				evt.info.get());
		}

		// and pass it further
		if (!mOutput.Send(evt))
			break;
	}
}

HRESULT CInterop_Export_Filter::Run(glucose::IFilter_Configuration* configuration)
{
	// no configuration needs to be done

	Run_Main();

	return S_OK;
}

HRESULT IfaceCalling CInterop_Export_Filter::Register_Listener(glucose::TInterop_Listener listener)
{
	mListener = listener;

	return S_OK;
}
