//--------------------------------------------------------------------------------------------------------------
// Includes & Libraries
//--------------------------------------------------------------------------------------------------------------
#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"Dxgi.lib") // for CreateDXGIFactory1

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11

#include <d3d11.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>



//--------------------------------------------------------------------------------------------------------------
// Structs & Typedefs
//--------------------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------------------
// Function declarations
//--------------------------------------------------------------------------------------------------------------
bool InitXR();
bool InitD3DDevice(LUID& adapter_luid);

//--------------------------------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------------------------------

// Configs for the application
const char* app_config_name = "BasicXRCube";
XrFormFactor app_config_form_factor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;	// We'll use a head mounted display
XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO; // And the HMD has two screens, one for each eye

// OpenXR globals
XrInstance xr_instance = {};
XrSession xr_session = {};
XrSystemId xr_system_id = XR_NULL_SYSTEM_ID;
XrEnvironmentBlendMode xr_blend_mode;
XrSpace xr_app_space = {};

// D3D globals
ID3D11Device* d3d_device;
ID3D11DeviceContext* d3d_device_context;

PFN_xrGetD3D11GraphicsRequirementsKHR ext_xrGetD3D11GraphicsRequirementsKHR;

// Constants to use
const XrPosef xr_pose_identity = { {0, 0, 0, 1}, {0, 0, 0} }; // Struct consisting of a quaternion which describes the orientation, and a vector3f which describes the position

//--------------------------------------------------------------------------------------------------------------
// Main Function
//--------------------------------------------------------------------------------------------------------------
int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
	bool openXRinitialized = InitXR();

	if (!openXRinitialized) {
		return -1;
	}

	return 0;
};

//--------------------------------------------------------------------------------------------------------------
// OpenXR Methods
//--------------------------------------------------------------------------------------------------------------
bool InitXR() {
	XrResult result;

	// Setup the OpenXR instance. At the moment, we're only using the D3D11 extension
	const char* enabled_extensions[] = { XR_KHR_D3D11_ENABLE_EXTENSION_NAME };
	XrInstanceCreateInfo create_info = {};
	create_info.type = XR_TYPE_INSTANCE_CREATE_INFO;
	create_info.enabledExtensionCount = 1;
	create_info.enabledExtensionNames = enabled_extensions;
	create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	strcpy_s(create_info.applicationInfo.applicationName, 128, app_config_name); // Copy the application name from the global

	result = xrCreateInstance(&create_info, &xr_instance);
	if (XR_FAILED(result)) {
		return false;
	}

	// Next, we need to setup the OpenXR system. This is rather easy, we just need to pass the correct form factor to the function call
	XrSystemGetInfo system_info = {};
	system_info.type = XR_TYPE_SYSTEM_GET_INFO;
	system_info.formFactor = app_config_form_factor;
	result = xrGetSystem(xr_instance, &system_info, &xr_system_id);
	if (XR_FAILED(result)) {
		return false;
	}

	// Get the blend modes for the XR device. As the function call to xrEnumerateEnvironmentBlendModes should
	// return the blend modes in order of preference of the runtime, we can just pick the first one
	uint32_t blend_count = 0; // Throwaway variable, but we need to pass in a pointer to a uint32_t, or the function call fails
	result = xrEnumerateEnvironmentBlendModes(xr_instance, xr_system_id, app_config_view, 1, &blend_count, &xr_blend_mode);
	if (XR_FAILED(result)) {
		return false;
	}

	// Get the address of the "xrGetD3D11GraphicsRequirementsKHR" function and store it, such that we can
	// call the function. As of now, it seems it's not yet possible to directly call the function
	xrGetInstanceProcAddr(xr_instance, "xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&ext_xrGetD3D11GraphicsRequirementsKHR));

	XrGraphicsRequirementsD3D11KHR graphics_requirements = {};
	graphics_requirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR;
	// Call the function which retrieves the D3D11 feature level and graphic device requirements for an instance and system
	// The graphics_requirements param is a XrGraphicsRequirementsD3D11KHR struct, where
	// the field "adapterLuid" identifies the graphics device to be used
	ext_xrGetD3D11GraphicsRequirementsKHR(xr_instance, xr_system_id, &graphics_requirements);

	// This LUID is then passed to the InitD3DDevice function, which creates a D3D11 device with the
	// graphics adapter that matches the LUID
	bool d3d_device_initialized = InitD3DDevice(graphics_requirements.adapterLuid);
	if(!d3d_device_initialized) {
		return false;
	}

	// Create a OpenXR session
	// First we need to create a binding for the D3D11 device we just created before
	XrGraphicsBindingD3D11KHR graphics_binding = {};
	graphics_binding.type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR;
	graphics_binding.device = d3d_device;

	// Then we can use this graphics binding to create a create info struct to pass to the create session function
	XrSessionCreateInfo session_create_info = {};
	session_create_info.type = XR_TYPE_SESSION_CREATE_INFO;
	session_create_info.next = &graphics_binding;
	session_create_info.systemId = xr_system_id;

	// Now we're ready to create the xr session
	result = xrCreateSession(xr_instance, &session_create_info, &xr_session);
	if (XR_FAILED(result)) {
		return false;
	}

	// Next, we need to choose a reference space to display the content. We'll use local, as the hololens doesn't
	// have a stage (which is usually restricted to the guardian system of the device, and more used by devices
	// such as the oculus rift or the valve index)
	XrReferenceSpaceCreateInfo reference_space_create_info = {};
	reference_space_create_info.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
	reference_space_create_info.poseInReferenceSpace = xr_pose_identity;
	reference_space_create_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;

	// Create the reference space
	result = xrCreateReferenceSpace(xr_session, &reference_space_create_info, &xr_app_space);
	if (XR_FAILED(result)) {
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------------------------
// D3D Methods
//--------------------------------------------------------------------------------------------------------------

// Turns the LUID that we get from the "xrGetD3D11GraphicsRequirementsKHR" function in a specific adapter
// and creates a D3D11 device using that adapter
bool InitD3DDevice(LUID& adapter_luid) {
	IDXGIAdapter1* adapter = nullptr;
	IDXGIFactory1* dxgi_factory;
	DXGI_ADAPTER_DESC1 adapter_desc;

	// Create a DXGI factory to be used for finding the correct adapter
	CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&dxgi_factory));

	// Loop over all adapters that the factory can return
	int current_adapter_id = 0;
	while (dxgi_factory->EnumAdapters1(current_adapter_id++, &adapter) == S_OK) {
		adapter->GetDesc1(&adapter_desc);

		// If the luid of the current selected adapter matches the one we're looking for,
		// we can break from the loop and use that adapter.
		// Else release the adapter and keep on searching
		if (memcmp(&adapter_desc.AdapterLuid, &adapter_luid, sizeof(&adapter_luid)) == 0) {
			break;
		}
		else {
			adapter->Release();
			adapter = nullptr;
		}
	}

	// Release the factory as it's not used anymore after this point
	dxgi_factory->Release();

	// We want to use DirectX feature level 11.0
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	// If we didn't find an adapter that we can use, return false, as we can't create
	// a D3D11 device without an adapter
	if (adapter == nullptr) {
		return false;
	}
	
	// Create the D3D11 device. We don't create the swapchains here, because we'll create them later
	HRESULT result = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, 0, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, &d3d_device, nullptr, &d3d_device_context);

	if (FAILED(result)) {
		return false;
	}

	adapter->Release();
	return true;
};
