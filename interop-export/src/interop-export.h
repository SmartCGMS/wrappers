#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/FilterLib.h"

#include <memory>
#include <thread>
#include <cstdint>

/*
 * Filter class for computing model parameters
 */
class CInterop_Export_Filter : public glucose::IAsynchronnous_Filter, public virtual glucose::IInterop_Export_Filter_Inspection, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::SFilter_Pipe mInput;
		// output pipe
		glucose::SFilter_Pipe mOutput;

		// registered listener
		glucose::TInterop_Listener mListener;

		// thread function
		void Run_Main();

	public:
		CInterop_Export_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
		virtual ~CInterop_Export_Filter();

		virtual HRESULT Run(glucose::IFilter_Configuration* configuration) override final;
		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;
		virtual HRESULT IfaceCalling Register_Listener(glucose::TInterop_Listener listener) override final;
};
