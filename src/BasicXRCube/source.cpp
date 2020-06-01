//###################################################################################################################
// Includes & Libraries
//###################################################################################################################
#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"Dxgi.lib") // for CreateDXGIFactory1

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11

#include <d3d11.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <vector>


//###################################################################################################################
// Structs & Typedefs
//###################################################################################################################
struct swapchain_data_t {
	ID3D11DepthStencilView* depth_buffer;
	ID3D11RenderTargetView* back_buffer;
};

struct swapchain_t {
	XrSwapchain handle;
	int32_t width;
	int32_t height;
	std::vector<XrSwapchainImageD3D11KHR> swapchain_images;
	std::vector<swapchain_data_t> swapchain_data;
};


//###################################################################################################################
// Function declarations
//###################################################################################################################
bool InitXR();
bool InitD3DDevice(LUID& adapter_luid);
swapchain_data_t CreateSwapchainRenderTargets(XrSwapchainImageD3D11KHR& swapchain_image);


//###################################################################################################################
// Globals
//###################################################################################################################

//------------------------------------------------------------------------------------------------------
// Configs for the application
//------------------------------------------------------------------------------------------------------
const char* app_config_name = "BasicXRCube";
XrFormFactor app_config_form_factor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;	// We'll use a head mounted display
XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO; // And the HMD has two screens, one for each eye

//------------------------------------------------------------------------------------------------------
// OpenXR globals
//------------------------------------------------------------------------------------------------------
XrInstance xr_instance = {}; // The OpenXR instance
XrSession xr_session = {}; // Thw OpenXR session
XrSystemId xr_system_id = XR_NULL_SYSTEM_ID; // The OpenXR System, identified by its ID
XrEnvironmentBlendMode xr_blend_mode; // Blend Mode (transparent / opaque) to use for the application
XrSpace xr_app_space = {};

std::vector<XrView> xr_views;
std::vector<XrViewConfigurationView> xr_view_configurations;
std::vector<swapchain_t> xr_swapchains;

//------------------------------------------------------------------------------------------------------
// D3D globals
//------------------------------------------------------------------------------------------------------
ID3D11Device* d3d_device;
ID3D11DeviceContext* d3d_device_context;
DXGI_FORMAT d3d_swapchain_format = DXGI_FORMAT_R8G8B8A8_UNORM;

//------------------------------------------------------------------------------------------------------
// Constants to use
//------------------------------------------------------------------------------------------------------
const XrPosef xr_pose_identity = { {0, 0, 0, 1}, {0, 0, 0} }; // Struct consisting of a quaternion which describes the orientation, and a vector3f which describes the position

//------------------------------------------------------------------------------------------------------
// Pointer to a function that we need to load
//------------------------------------------------------------------------------------------------------
PFN_xrGetD3D11GraphicsRequirementsKHR ext_xrGetD3D11GraphicsRequirementsKHR;


//###################################################################################################################
// Main Function
//###################################################################################################################
int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {

	//------------------------------------------------------------------------------------------------------
	// Initialize OpenXR
	//------------------------------------------------------------------------------------------------------
	if (!InitXR()) {
		return -1;
	}

	return 0;
};


//###################################################################################################################
// OpenXR Methods
//###################################################################################################################
bool InitXR() {
	XrResult result;

	//------------------------------------------------------------------------------------------------------
	// Setup the OpenXR instance. At the moment, we're only using the D3D11 extension
	//------------------------------------------------------------------------------------------------------
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

	//------------------------------------------------------------------------------------------------------
	// Setup the OpenXR system
	//------------------------------------------------------------------------------------------------------

	// This is rather easy, we just need to pass the correct form factor to the function call
	XrSystemGetInfo system_info = {};
	system_info.type = XR_TYPE_SYSTEM_GET_INFO;
	system_info.formFactor = app_config_form_factor;
	result = xrGetSystem(xr_instance, &system_info, &xr_system_id);
	if (XR_FAILED(result)) {
		return false;
	}

	//------------------------------------------------------------------------------------------------------
	// Setup the OpenXR session
	//------------------------------------------------------------------------------------------------------

	// Get the blend modes for the XR device. As the function call to xrEnumerateEnvironmentBlendModes
	// should return the blend modes in order of preference of the runtime, we can just pick the first one
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

	//------------------------------------------------------------------------------------------------------
	// Create a reference space
	//------------------------------------------------------------------------------------------------------

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

	//------------------------------------------------------------------------------------------------------
	// Setup the viewports / view configurations
	//------------------------------------------------------------------------------------------------------

	// Devices running OpenXR code can have multiple viewports (views we need to render). For a stereo
	// headset (XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO), this is usually 2, one for each eye. If it's
	// an AR application running on e.g. a phone, this would be 1, and for other constellations, such as
	// a cave-like system where on each wall an image is projected, this can even be 6 (or any other number).
	
	// As such, we first need to find out how many views we need to support. If we set the 4th param of
	// the xrEnumerateViewConfigurationViews function call to zero, it will retrieve the required
	// number of viewports and store it in the 5th param
	uint32_t viewport_count = 0;
	result = xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, app_config_view, 0, &viewport_count, NULL);
	if (XR_FAILED(result)) {
		return false;
	}

	// Now that we know how many views we need to render, resize the vector that contains the view
	// configurations (each XrViewConfigurationView specifies properties related to rendering an
	// individual view) and the vector containing the views (each XrView specifies the pose and
	// the fov of the view. Basically, a XrView is a view matrix in "traditional" rendering).
	xr_view_configurations.resize(viewport_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
	xr_views.resize(viewport_count, { XR_TYPE_VIEW });

	// Now we again call xrEnumerateViewConfigurationViews, this time we set the 4th param
	// to the number of our viewports, such that the method fills the xr_view_configurations
	// vector with the actual view configurations
	result = xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, app_config_view, viewport_count, &viewport_count, xr_view_configurations.data());
	if (XR_FAILED(result)) {
		return false;
	}

	//------------------------------------------------------------------------------------------------------
	// Setup the swapchains
	//------------------------------------------------------------------------------------------------------

	// For each viewport, we need to setup a swapchain. A swapchain consists of multiple buffers, where one
	// is used to draw the data to the screen, and another is used to render the deta from the simulation
	// to. With this approach, tearing (that might occur because the scene is updated while it's drawn
	// to the screen) should not occur

	for (uint32_t i = 0; i < viewport_count; i++) {
		// Get the current view configuration we're interested in
		XrViewConfigurationView& current_view_configuration = xr_view_configurations[i];

		// Create a create info struct to create the swapchain
		XrSwapchainCreateInfo swapchain_create_info = {};
		swapchain_create_info.type = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
		swapchain_create_info.arraySize = 1; // Number of array layers
		swapchain_create_info.mipCount = 1; // Only use one mipmap level, bigger numbers would only be useful for textures
		swapchain_create_info.faceCount = 1; // Number of faces to render, 1 should be used, other option would be 6 for cubemaps
		swapchain_create_info.format = d3d_swapchain_format; // Use the globally set swapchain format
		swapchain_create_info.width = current_view_configuration.recommendedImageRectWidth; // Just use the recommended width that the runtime gave us
		swapchain_create_info.height = current_view_configuration.recommendedImageRectHeight; // Just use the recommended height that the runtime gave us
		swapchain_create_info.sampleCount = current_view_configuration.recommendedSwapchainSampleCount; // Just use the recommended sample count that the runtime gave us
		swapchain_create_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

		// Create the OpenXR swapchain
		XrSwapchain swapchain_handle;
		result = xrCreateSwapchain(xr_session, &swapchain_create_info, &swapchain_handle);
		if (XR_FAILED(result)) {
			return false;
		}

		// OpenXR can create an arbitrary number of swapchain images (from which we'll create our buffers),
		// so we need to find out how many were created by the runtime
		// If we pass zero as the 2nd param, we request the number of swapchain images and store it
		// in the 3rd param
		uint32_t swapchain_image_count = 0;
		result = xrEnumerateSwapchainImages(swapchain_handle, 0, &swapchain_image_count, NULL);
		if (XR_FAILED(result)) {
			return false;
		}

		// Now we can create the swapchain
		swapchain_t swapchain = {};
		swapchain.width = swapchain_create_info.width;
		swapchain.height = swapchain_create_info.height;
		swapchain.handle = swapchain_handle;
		swapchain.swapchain_images.resize(swapchain_image_count, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
		swapchain.swapchain_data.resize(swapchain_image_count);

		// Now call the xrEnumerateSwapchainImages function again,
	}


	return true;
}


//###################################################################################################################
// D3D Methods
//###################################################################################################################

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

// This method takes a XrSwapchainImageD3D11KHR (which has a ID3D11Texture2D field, which normally
// needs to be created manually when using D3D11), and creates a render target (backbuffer) as well
// as a matching depth buffer
swapchain_data_t CreateSwapchainRenderTargets(XrSwapchainImageD3D11KHR& swapchain_image) {
	swapchain_data_t resulting_target = {};

	//----------------------------------------------------------------------------------
	// Create the backbuffer
	//----------------------------------------------------------------------------------

	// We need to pass some aditional data to the CreateRenderTargetView function, so we create
	// a Render target view desc and add the format of the swapchain, as well as the view dimension
	// to that description struct.
	// The swapchain image that OpenXR created uses a TYPELESS format, however, we need a "typed"
	// format. As such, we use the DXGI_FORMAT_R8G8B8A8_UNORM format, which is "A four-component, 32-bit
	// unsigned-normalized-integer format that supports 8 bits per channel including alpha" (according
	// to MSDN. That way, we can specify the colors with RGBA values between 0 and 255.
	D3D11_RENDER_TARGET_VIEW_DESC render_target_desc = {};
	render_target_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	render_target_desc.Format = d3d_swapchain_format;
	d3d_device->CreateRenderTargetView(swapchain_image.texture, &render_target_desc, &resulting_target.back_buffer);

	//----------------------------------------------------------------------------------
	// Create a matching depth buffer (z-buffer)
	//----------------------------------------------------------------------------------

	// As we can't directly use the .texture field of the swapchain image as for the backbuffer,
	// we first need to fetch information about the swapchain image (ID3D11Texture2D) that OpenXR
	// created and then we can use that to information to manually construct a texture object
	D3D11_TEXTURE2D_DESC image_desc = {};
	swapchain_image.texture->GetDesc(&image_desc);

	// Create the depth buffer description
	D3D11_TEXTURE2D_DESC depth_buffer_desc = {};
	depth_buffer_desc.SampleDesc.Count = 1; // Do not use supersampling for now
	depth_buffer_desc.MipLevels = 1; // Multiple mipmap levels are only useful for textures, for backbuffer only need one level
	depth_buffer_desc.Width = image_desc.Width;	// Use same width as the swapchain image that OpenXR created
	depth_buffer_desc.Height = image_desc.Height; // Use same height as the swapchain image that OpenXR created
	depth_buffer_desc.ArraySize = image_desc.ArraySize;
	depth_buffer_desc.Format = DXGI_FORMAT_R32_TYPELESS; // Use TYPELESS format, such that we have the same as for the image that OpenXR created
	depth_buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;

	// Create the depth buffer texture object
	ID3D11Texture2D* depth_buffer;
	d3d_device->CreateTexture2D(&depth_buffer_desc, NULL, &depth_buffer);

	// Then use that depth buffer texture object to finally create the depth buffer itself
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depth_stencil_view_desc.Format = DXGI_FORMAT_D32_FLOAT; // Data format for the depth buffer itself is float
	d3d_device->CreateDepthStencilView(depth_buffer, &depth_stencil_view_desc, &resulting_target.depth_buffer);

	// We don't need the ID3D11Texture2D object anymore. As it's a COM object, it should be freed by calling
	// Release() on it
	depth_buffer->Release();

	return resulting_target;
};
