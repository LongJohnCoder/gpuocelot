// C standard library includes

// C++ standard library includes

// Ocelot includes
#include <ocelot/opencl/interface/Device.h>

opencl::Device::DeviceList opencl::Device::_deviceList = opencl::Device::DeviceList();

bool opencl::Device::_loaded = false;

cl_uint opencl::Device::_deviceCount = 0;

const bool opencl::Device::_hasPlatform(cl_platform_id platform) const {
	return ((Platform *)platform == _platform);
}

const bool opencl::Device::_isType(cl_device_type type) const {

	if((type & CL_DEVICE_TYPE_DEFAULT) == CL_DEVICE_TYPE_DEFAULT)
		type |= CL_DEVICE_TYPE_GPU;

	return ((type & _type) == _type);
}

const bool opencl::Device::_isValidType(const cl_device_type type) {
	return (type <= (CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU
		| CL_DEVICE_TYPE_ACCELERATOR | CL_DEVICE_TYPE_CUSTOM)) || (type == CL_DEVICE_TYPE_ALL);
}

opencl::Device::Device(executive::Device * d, cl_device_type type, 
	Platform * p, std::string & vendor, 
	Device * parentDevice, cl_device_partition_property * partitionProp,
	size_t partitionPropSize):
	Object(OBJTYPE_DEVICE),
	_exeDevice(d), _type(type), _vendorId(_deviceCount++), _vendor(vendor),
	_platform(p), _builtInKernels(""), _parentDevice(parentDevice),
	_partitionProp(partitionProp), _partitionPropSize(partitionPropSize) {

	_platform->retain();
}

opencl::Device::~Device() {

	_platform->release();

	delete _exeDevice;

	DeviceList::iterator it = std::find(_deviceList.begin(), _deviceList.end(), this);
	assert(it != _deviceList.end());

	_deviceList.erase(it);

}

void opencl::Device::release() {
	if(Object::release())
		delete this;
}


void opencl::Device::createDevices(Platform * platform, deviceT device, 
	unsigned int flags, int computeCapability, int workerThreadLimit ) {

	executive::DeviceVector d;
	cl_device_type type= CL_DEVICE_TYPE_DEFAULT;
	std::string vendor = "OCELOT";

	switch(device) {
		case DEVICE_NVIDIA_GPU:
			d = executive::Device::createDevices(ir::Instruction::SASS, flags,
				computeCapability);
			type = CL_DEVICE_TYPE_GPU;
			vendor = "NVIDIA";
			report(" - Added " << d.size() << " nvidia gpu devices." );
			break;

		case DEVICE_EMULATED:
			d = executive::Device::createDevices(ir::Instruction::Emulated, flags,
				computeCapability);
			type = CL_DEVICE_TYPE_GPU;
			vendor = "OCELOT";
			report(" - Added " << d.size() << " emulator devices." );
			break;

		case DEVICE_MULTICORE_CPU:
			d = executive::Device::createDevices(ir::Instruction::LLVM, flags,
				computeCapability);
			type = CL_DEVICE_TYPE_CPU;
			vendor = "OCELOT";
			report(" - Added " << d.size() << " llvm-cpu devices." );

			if (workerThreadLimit > 0) {
				for (executive::DeviceVector::iterator d_it = d.begin();
					d_it != d.end(); ++d_it) {
					(*d_it)->limitWorkerThreads(workerThreadLimit);
				}
			}
			break;

		case DEVICE_AMD_GPU:
			d =	executive::Device::createDevices(ir::Instruction::CAL, flags,
					computeCapability);
			type = CL_DEVICE_TYPE_GPU;
			vendor = "AMD";
			report(" - Added " << d.size() << " amd gpu devices." );
			break;

		case DEVICE_REMOTE:
			d =
				executive::Device::createDevices(ir::Instruction::Remote, flags,
					computeCapability);
			type = CL_DEVICE_TYPE_GPU;
			vendor = "OCELOT";
			report(" - Added " << d.size() << " remote devices." );
			break;
	}
	
	for(size_t i = 0; i < d.size(); i++)
		_deviceList.push_back(new Device(d[i], type, platform, vendor, NULL, NULL, 0));
}

void opencl::Device::getDevices(cl_platform_id platform, cl_device_type type, cl_uint num_entries,
	cl_device_id * devices, cl_uint * num_devices) {

	if(!_isValidType(type))
		throw CL_INVALID_DEVICE_TYPE;

	cl_uint j = 0;
	if(devices != 0) {
		for(DeviceList::iterator d = _deviceList.begin(); d != _deviceList.end(); d++) {
			if((*d)->_isType(type) && (*d)->_hasPlatform(platform)) {
				if(j < num_entries)
					devices[j] = (cl_device_id)(*d);
				j++;
			}
		}
	}

	
	if(num_devices != 0)
		*num_devices = j;

	if(j==0)
		throw CL_DEVICE_NOT_FOUND;
}

void opencl::Device::limitWorkerThreadForAll(unsigned int limit) {
	for (DeviceList::iterator device = _deviceList.begin(); 
		device != _deviceList.end(); ++device) {
		(*device)->_exeDevice->select();
		(*device)->_exeDevice->limitWorkerThreads(limit);
		(*device)->_exeDevice->unselect();
	}
} 

void opencl::Device::unregisterModuleForAll(const std::string & name) {
	for (DeviceList::iterator device = _deviceList.begin(); 
		device != _deviceList.end(); ++device) {
		(*device)->_exeDevice->select();
		(*device)->_exeDevice->unload(name);
		(*device)->_exeDevice->unselect();
	}
}

void opencl::Device::setOptimizationLevelForAll(translator::Translator::OptimizationLevel l) {
	for (DeviceList::iterator device = _deviceList.begin(); 
		device != _deviceList.end(); ++device) {
		(*device)->_exeDevice->select();
		(*device)->_exeDevice->setOptimizationLevel(l);
		(*device)->_exeDevice->unselect();
	}
}

void opencl::Device::getInfo(cl_device_info param_name,
	size_t param_value_size,
	void * param_value,
	size_t * param_value_size_ret) {

	union infoUnion {
		cl_device_type cl_device_type_var;
		cl_uint cl_uint_var;
		size_t sizes[3];
		size_t size_t_var;
		cl_ulong cl_ulong_var;
		cl_bool cl_bool_var;
		cl_device_fp_config cl_device_fp_config_var;
		cl_device_mem_cache_type cl_device_mem_cache_type_var;
		cl_device_local_mem_type cl_device_local_mem_type_var;
		cl_device_exec_capabilities cl_device_exec_capabilities_var;
		cl_command_queue_properties cl_command_queue_properties_var;
		cl_platform_id cl_platform_id_var;
		cl_device_id cl_device_id_var;
		cl_device_partition_property cl_device_partition_property_var;
		cl_device_affinity_domain cl_device_affinity_domain_var;
	};

	infoUnion info;
	const void * ptr = &info;
	size_t infoLen = 0;
	std::string str;
	const executive::Device::Properties & prop = _exeDevice->properties();

#define ASSIGN_INFO(field, value) \
do { \
	info.field##_var = value; \
	infoLen = sizeof(field); \
} while(0)

#define ASSIGN_STRING(value) \
do { \
	str = value; \
	ptr = str.c_str(); \
	infoLen = str.length() + 1; \
}while(0)

	switch(param_name) {
		case CL_DEVICE_TYPE:
			ASSIGN_INFO(cl_device_type, _type);
			break;

		case CL_DEVICE_VENDOR_ID:
			ASSIGN_INFO(cl_uint, _vendorId);
			break;

		case CL_DEVICE_MAX_COMPUTE_UNITS:
			ASSIGN_INFO(cl_uint, (cl_uint)prop.multiprocessorCount * 48); //fermi	
			break;

		case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
			ASSIGN_INFO(cl_uint, 3);
			break;

		case CL_DEVICE_MAX_WORK_ITEM_SIZES:
			for(int i = 0; i < 3; i++)
				info.sizes[i] = (size_t)prop.maxThreadsDim[i];
			infoLen = sizeof(size_t[3]); 
			break;

		case CL_DEVICE_MAX_WORK_GROUP_SIZE:
			for(int i = 0; i < 3; i++)
				info.sizes[i] = (size_t)prop.maxGridSize[i];
			infoLen = sizeof(size_t[3]); 
			break;

		case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
			ASSIGN_INFO(cl_uint, 16);
			break;

		case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
			ASSIGN_INFO(cl_uint, 8);
			break;

		case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
			ASSIGN_INFO(cl_uint, 4);
			break;

		case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
			ASSIGN_INFO(cl_uint, 2);
			break;

		case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
			ASSIGN_INFO(cl_uint, 4);
			break;

		case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
			ASSIGN_INFO(cl_uint, 2);
			break;

		case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
			ASSIGN_INFO(cl_uint, 0); //cl_khr_fp16 extension not supported
			break;

		case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
			ASSIGN_INFO(cl_uint, 16);
			break;

		case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
			ASSIGN_INFO(cl_uint, 8);
			break;

		case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
			ASSIGN_INFO(cl_uint, 4);
			break;

		case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
			ASSIGN_INFO(cl_uint, 2);
			break;

		case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
			ASSIGN_INFO(cl_uint, 4);
			break;

		case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
			ASSIGN_INFO(cl_uint, 2);
			break;

		case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
			ASSIGN_INFO(cl_uint, 0); //cl_khr_fp16 extension not supported
			break;

		case CL_DEVICE_MAX_CLOCK_FREQUENCY:
			ASSIGN_INFO(cl_uint, (cl_uint)prop.clockRate);
			break;

		case CL_DEVICE_ADDRESS_BITS:
			ASSIGN_INFO(cl_uint, 64);
			break;

		case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
			ASSIGN_INFO(cl_ulong, (cl_ulong)prop.totalMemory);
			break;

		case CL_DEVICE_IMAGE_SUPPORT:
			ASSIGN_INFO(cl_bool, CL_TRUE);
			break;

		case CL_DEVICE_MAX_READ_IMAGE_ARGS:
			ASSIGN_INFO(cl_uint, 128);
			break;
			
		case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
			ASSIGN_INFO(cl_uint, 8);
			break;

		case CL_DEVICE_IMAGE2D_MAX_WIDTH:
			ASSIGN_INFO(size_t, 8192);
			break;

		case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
			ASSIGN_INFO(size_t, 8192);
			break;

		case CL_DEVICE_IMAGE3D_MAX_WIDTH:
			ASSIGN_INFO(size_t, 2048);
			break;

		case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
			ASSIGN_INFO(size_t, 2048);
			break;

		case CL_DEVICE_IMAGE3D_MAX_DEPTH:
			ASSIGN_INFO(size_t, 2048);
			break;

		case CL_DEVICE_IMAGE_MAX_BUFFER_SIZE:
			ASSIGN_INFO(size_t, 65536);
			break;

		case CL_DEVICE_IMAGE_MAX_ARRAY_SIZE:
			ASSIGN_INFO(size_t, 2048);
			break;

		case CL_DEVICE_MAX_SAMPLERS:
			ASSIGN_INFO(cl_uint, 16);
			break;

		case CL_DEVICE_MAX_PARAMETER_SIZE:
			ASSIGN_INFO(size_t, 1024);
			break;

		case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
			ASSIGN_INFO(cl_uint, 16*8/*sizeof(long16)*/);
			break;

		case CL_DEVICE_SINGLE_FP_CONFIG:
			ASSIGN_INFO(cl_device_fp_config, CL_FP_DENORM | CL_FP_INF_NAN
				| CL_FP_ROUND_TO_NEAREST | CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_INF
				| CL_FP_FMA | CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT | CL_FP_SOFT_FLOAT);
			break;

		case CL_DEVICE_DOUBLE_FP_CONFIG:
			ASSIGN_INFO(cl_device_fp_config, CL_FP_FMA | CL_FP_ROUND_TO_NEAREST
				| CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_INF | CL_FP_INF_NAN
				| CL_FP_DENORM);
			break;

		case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
			ASSIGN_INFO(cl_device_mem_cache_type, CL_READ_WRITE_CACHE);
			break;

		case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE: //Unknown
			ASSIGN_INFO(cl_uint, 0);
			break;

		case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE: //Unknown
			ASSIGN_INFO(cl_uint, 0);
			break;

		case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
			ASSIGN_INFO(cl_uint, (cl_uint)prop.totalConstantMemory);
			break;

		case CL_DEVICE_MAX_CONSTANT_ARGS:
			ASSIGN_INFO(cl_uint, (cl_uint)prop.totalConstantMemory/16);
			break;

		case CL_DEVICE_LOCAL_MEM_TYPE:
			ASSIGN_INFO(cl_device_local_mem_type, CL_GLOBAL);
			break;

		case CL_DEVICE_LOCAL_MEM_SIZE:
			ASSIGN_INFO(cl_ulong, (cl_uint)prop.totalMemory);
			break;

		case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
			ASSIGN_INFO(cl_bool, CL_FALSE);
			break;

		case CL_DEVICE_HOST_UNIFIED_MEMORY:
			ASSIGN_INFO(cl_bool, (cl_bool)prop.unifiedAddressing);
			break;

		case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
			ASSIGN_INFO(size_t, 1000);
			break;

		case CL_DEVICE_ENDIAN_LITTLE:
			ASSIGN_INFO(cl_bool, CL_TRUE);
			break;

		case CL_DEVICE_AVAILABLE:
			ASSIGN_INFO(cl_bool, CL_TRUE);
			break;

		case CL_DEVICE_COMPILER_AVAILABLE:
			ASSIGN_INFO(cl_bool, CL_TRUE);
			break;

		case CL_DEVICE_LINKER_AVAILABLE:
			ASSIGN_INFO(cl_bool, CL_TRUE);
			break;

		case CL_DEVICE_EXECUTION_CAPABILITIES:
			ASSIGN_INFO(cl_device_exec_capabilities, CL_EXEC_KERNEL 
				| CL_EXEC_NATIVE_KERNEL);
			break;

		case CL_DEVICE_QUEUE_PROPERTIES:
			ASSIGN_INFO(cl_command_queue_properties, CL_QUEUE_PROFILING_ENABLE);
			break;

		case CL_DEVICE_BUILT_IN_KERNELS:
			ptr = _builtInKernels.c_str();
			infoLen = _builtInKernels.length() + 1;
			break;

		case CL_DEVICE_PLATFORM:
			ASSIGN_INFO(cl_platform_id, (cl_platform_id)_platform);
			break;

		case CL_DEVICE_NAME:
			ASSIGN_STRING(prop.name);
			break;
			
		case CL_DEVICE_VENDOR:
			ASSIGN_STRING(_vendor);
			break;

		case CL_DRIVER_VERSION:
			ASSIGN_STRING("1.2");
			break;

		case CL_DEVICE_PROFILE:
			ASSIGN_STRING("FULL_PROFILE");
			break;

		case CL_DEVICE_VERSION:
			ASSIGN_STRING("OpenCL 1.2");
			break;

		case CL_DEVICE_OPENCL_C_VERSION:
			ASSIGN_STRING("OpenCL 1.2");
			break;

		case CL_DEVICE_EXTENSIONS:
			ASSIGN_STRING("cl_khr_global_int32_base_atomics \
cl_khr_global_int32_extended_atomics cl_khr_local_int32_base_atomics \
cl_khr_local_int32_extended_atomics cl_khr_byte_addressable_store \
cl_khr_fp64 cl_khr_gl_sharing cl_khr_gl_event");
			break;

		case CL_DEVICE_PRINTF_BUFFER_SIZE:
			ASSIGN_INFO(size_t, prop.printfFIFOSize);
			break;

		case CL_DEVICE_PREFERRED_INTEROP_USER_SYNC:
			ASSIGN_INFO(cl_bool, CL_TRUE);
			break;

		case CL_DEVICE_PARENT_DEVICE:
			ASSIGN_INFO(cl_device_id, (cl_device_id)_parentDevice);
			break;

		case CL_DEVICE_PARTITION_MAX_SUB_DEVICES:
			ASSIGN_INFO(cl_uint, 1); //allow partition, but only 1
			break;

		case CL_DEVICE_PARTITION_PROPERTIES:
			ASSIGN_INFO(cl_device_partition_property, CL_DEVICE_PARTITION_EQUALLY);
			break;

		case CL_DEVICE_PARTITION_AFFINITY_DOMAIN:
			ASSIGN_INFO(cl_device_affinity_domain, 0);
			break;

		case CL_DEVICE_PARTITION_TYPE:
			ptr = _partitionProp;
			infoLen = _partitionPropSize * sizeof(size_t);
			break;

		case CL_DEVICE_REFERENCE_COUNT:
			ASSIGN_INFO(cl_uint, (cl_uint)_references);
			break;

		default:
			assertM(false, "Device info unimplemented!\n");
			throw CL_UNIMPLEMENTED;
			break;
	}

	if(param_value && param_value_size < infoLen)
		throw CL_INVALID_VALUE;
	
	if(param_value != 0)
		std::memcpy(param_value, ptr, infoLen);

	if(param_value_size_ret !=0 )
		*param_value_size_ret = infoLen;

}

const char * opencl::Device::name() {
	return _exeDevice->properties().name;
} 

void opencl::Device::load(ir::Module * module) {

	if(module->loaded())
		return;

	module->loadNow();

	_exeDevice->select();
	_exeDevice->load(module);
	_exeDevice->unselect();
}

void opencl::Device::unload(const std::string & name) {

	_exeDevice->select();
	_exeDevice->unload(name);
	_exeDevice->unselect();
}

void opencl::Device::launch(const std::string& module,
                const std::string& kernel, const ir::Dim3& grid,
                const ir::Dim3& block, size_t sharedMemory,
                const void* argumentBlock, size_t argumentBlockSize,
                const trace::TraceGeneratorVector& traceGenerators,
                const ir::ExternalFunctionSet* externals) {

	_exeDevice->select();
	_exeDevice->launch(module, kernel, grid, block, sharedMemory,
		argumentBlock, argumentBlockSize, traceGenerators,
		externals);
	_exeDevice->unselect();
}

void opencl::Device::setOptimizationLevel(translator::Translator::OptimizationLevel level) {

	_exeDevice->select();
	_exeDevice->setOptimizationLevel(level);
	_exeDevice->unselect();

}

void * opencl::Device::allocate(size_t size) {

	void * ptr;

	_exeDevice->select();
	ptr = _exeDevice->allocate(size)->pointer();
	_exeDevice->unselect();

	return ptr;
}

bool opencl::Device::read(const void * src, void * host, size_t offset, size_t size) {
	

	if(!_exeDevice->checkMemoryAccess((char *)src + offset, size))
		return false;

	_exeDevice->select();
	_exeDevice->getMemoryAllocation(src)->copy(host, offset, size);
	_exeDevice->unselect();

	return true;
}

bool opencl::Device::write(void * dest, const void * host, size_t offset, size_t size) {


	if(!_exeDevice->checkMemoryAccess((char *)dest + offset, size)) {
		return false;
	}

	_exeDevice->select();
	_exeDevice->getMemoryAllocation(dest)->copy(offset, host, size);
	_exeDevice->unselect();

	return true;
}

