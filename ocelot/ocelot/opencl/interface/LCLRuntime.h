#ifndef OCELOT_LCL_RUNTIME_H_INCLUDED
#define OCELOT_LCL_RUNTIME_H_INCLUDED

//C++ libs
#include <list>
#include <algorithm>

//Ocelot libs
#include <ocelot/opencl/interface/lcl_runtime.h>
#include <ocelot/opencl/interface/VirtualBuffer.h>


struct _lcl_vbuf : public lcl::VirtualBuffer {
};

namespace lcl {

	class LCLRuntime {

	public: 
		typedef VirtualBuffer::VirtualBufferList VirtualBufferList;

		typedef enum {
			DEVICE_READ = 0,
			DEVICE_WRITE,
			HOST_READ,
			HOST_WRITE
		}AccessType;

	public:

		//! \brief singleton accessors */
		static LCLRuntime *instance;
		
		//! \brief get available implementation */
		static LCLRuntime *get();

		LCLRuntime();
		~LCLRuntime();

		bool isValidVirtualBuffer(VirtualBuffer * vBuf);

		bool isInEvaluation();

		bool isEvaluated();

		size_t increaseAccessSize(size_t size, AccessType type);

	public:
		virtual lcl_vbuf lclCreateVirtualBuffer(lcl_context context,
							size_t size,
							lcl_int * errcode_ret);
		virtual lcl_int lclEnqueueReadVirtualBuffer(lcl_command_queue command_queue,
							lcl_vbuf virtual_buffer,
							lcl_bool blocking_write,
							size_t offset,
							size_t size,
							void * ptr,
							lcl_uint num_events_in_wait_list,
							const lcl_event * event_wait_list,
							lcl_event * event);
		virtual lcl_int lclEnqueueWriteVirtualBuffer(lcl_command_queue command_queue,
							lcl_vbuf virtual_buffer,
							lcl_bool blocking_write,
							size_t offset,
							size_t cb,
							const void * ptr,
							lcl_uint num_events_in_wait_list,
							const lcl_event * event_wait_list,
							lcl_event * event);
		virtual lcl_int lclSetKernelArg(lcl_kernel kernel,
					lcl_uint arg_index,
					size_t arg_size,
					const void * arg_value);
		virtual lcl_int lclEnqueueNDRangeKernel(lcl_command_queue command_queue,
					lcl_kernel kernel,
					lcl_uint work_dim,
					const size_t * global_work_offset,
					const size_t * global_work_size,
					const size_t * local_work_size,
					lcl_uint num_events_in_wait_list,
					const lcl_event * event_wait_list,
					lcl_event * event);
		virtual lcl_int lclFinish(lcl_command_queue command_queue);
		virtual lcl_int lclReleaseVirtualBuffer(lcl_vbuf virtual_buffer);
		virtual lcl_int lclEvaluateStart();
		virtual lcl_int lclEvaluateEnd();

	private:
		VirtualBufferList _vBuffers;

		bool _inEvaluation;
		bool _isEvaluated;

		size_t _deviceReadSize;
		size_t _deviceWriteSize;
		size_t _hostReadSize;
		size_t _hostWriteSize;

	};
}

#endif
