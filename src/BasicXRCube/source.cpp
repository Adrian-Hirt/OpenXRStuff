//###################################################################################################################
// Includes & Libraries
//###################################################################################################################
#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"Dxgi.lib") // for CreateDXGIFactory1
#pragma comment(lib,"D3dcompiler.lib") // To be able to compile the shaders

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11

// DirectX includes
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>

// OpenXR includes
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

// Other includes
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

struct vertex_t {
	float x, y, z; // Coordinates of the vertex
	float norm_x, norm_y, norm_z; // Normal Vector
};

typedef struct RGBA {
	FLOAT r;
	FLOAT g;
	FLOAT b;
	FLOAT a;
} RGBA;

struct const_buffer_t {
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 view_projection;
	DirectX::XMFLOAT4X4 rotation;
	DirectX::XMFLOAT4 light_vector;
	RGBA light_color;
	RGBA ambient_color;
};

//###################################################################################################################
// Function declarations
//###################################################################################################################

//------------------------------------------------------------------------------------------------------
// OpenXR Methods
//------------------------------------------------------------------------------------------------------
bool InitXr();
bool InitXrActions();
void PollOpenXrEvents(bool& running, bool& xr_running);
void PollOpenXrActions();
void RenderOpenXrFrame();
void RenderOpenXrLayer(XrTime predicted_time, std::vector<XrCompositionLayerProjectionView>& views, XrCompositionLayerProjection& layer_projection);

//------------------------------------------------------------------------------------------------------
// DirectX Methods
//------------------------------------------------------------------------------------------------------
bool InitD3DDevice(LUID& adapter_luid);
swapchain_data_t CreateSwapchainRenderTargets(XrSwapchainImageD3D11KHR& swapchain_image);
bool InitD3DPipeline();
bool InitD3DGraphics();
void ShutdownD3D();
void RenderD3DLayer(XrCompositionLayerProjectionView& view, swapchain_data_t& swapchain_data);
DirectX::XMMATRIX CreateViewProjectionMatrix(XrCompositionLayerProjectionView& view);

//------------------------------------------------------------------------------------------------------
// App Methods
//------------------------------------------------------------------------------------------------------
void UpdateSimulation(XrTime predicted_time);
void Draw(XrCompositionLayerProjectionView& view);


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
XrSessionState xr_session_state = XR_SESSION_STATE_UNKNOWN; // The session state
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
ID3D11VertexShader* d3d_vertex_shader;
ID3D11PixelShader* d3d_pixel_shader;
ID3D11InputLayout* d3d_input_layout;
ID3D11Buffer* d3d_const_buffer;
ID3D11Buffer* d3d_vertex_buffer;
ID3D11Buffer* d3d_index_buffer;

//------------------------------------------------------------------------------------------------------
// Constants to use
//------------------------------------------------------------------------------------------------------
const XrPosef xr_pose_identity = { {0, 0, 0, 1}, {0, 0, 0} }; // Struct consisting of a quaternion which describes the orientation, and a vector3f which describes the position

//------------------------------------------------------------------------------------------------------
// Pointer to a function that we need to load
//------------------------------------------------------------------------------------------------------
PFN_xrGetD3D11GraphicsRequirementsKHR ext_xrGetD3D11GraphicsRequirementsKHR;

//------------------------------------------------------------------------------------------------------
// The data to draw
//------------------------------------------------------------------------------------------------------
vertex_t vertices[] = {
		{-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f},    // side 1
		{1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f},
		{-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f},
		{1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f},

		{-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f},    // side 2
		{-1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f},
		{1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f},
		{1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f},

		{-1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f},    // side 3
		{-1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f},
		{1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f},
		{1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f},

		{-1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f},    // side 4
		{1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f},
		{-1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f},
		{1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f},

		{1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f},    // side 5
		{1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f},
		{1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
		{1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},

		{-1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f},    // side 6
		{-1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f},
		{-1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f},
		{-1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f},
};

uint16_t indices[] = {
	2, 1, 0,    // side 1
	3, 1, 2,
	6, 5, 4,    // side 2
	7, 5, 6,
	10, 9, 8,    // side 3
	11, 9, 10,
	14, 13, 12,    // side 4
	15, 13, 14,
	18, 17, 16,    // side 5
	19, 17, 18,
	22, 21, 20,    // side 6
	23, 21, 22
};

DirectX::XMFLOAT3 cube_rotation_angles = { 0.0f, 0.0f, 0.0f };


//###################################################################################################################
// Main Function
//###################################################################################################################
int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {

	//------------------------------------------------------------------------------------------------------
	// Initialize OpenXR
	//------------------------------------------------------------------------------------------------------
	if (!InitXr()) {
		return -1;
	}

	//------------------------------------------------------------------------------------------------------
	// Initialize the OpenXR Actions
	//------------------------------------------------------------------------------------------------------
	if (!InitXrActions()) {
		return -1;
	}

	//------------------------------------------------------------------------------------------------------
	// Initialize the DirectX pipeline
	//------------------------------------------------------------------------------------------------------
	if (!InitD3DPipeline()) {
		return -1;
	}

	//------------------------------------------------------------------------------------------------------
	// Initialize the DirectX graphics
	//------------------------------------------------------------------------------------------------------
	if (!InitD3DGraphics()) {
		return -1;
	}

	//------------------------------------------------------------------------------------------------------
	// Main Loop
	//------------------------------------------------------------------------------------------------------
	bool loop_running = true;
	bool xr_running = false;

	while (loop_running) {
		// Poll the OpenXR events, and if OpenXR reports to still be running, keep going on
		PollOpenXrEvents(loop_running, xr_running);

		if (xr_running) {
			// TODO: If OpenXR is still running
			// 1) Poll actions
			PollOpenXrActions();

			// 2) Render frame. We'll also call the method that updates the simulation in
			//    that method, as we need to pass the predicted time (when the frame will
			//	  be rendered) to the simulation, such that we're able to use the time
			//	  to update the simulation accurately
			RenderOpenXrFrame();
		}

		// DEBUG; REMOVE LATER
		//loop_running = false;
	}

	//------------------------------------------------------------------------------------------------------
	// Shutdown D3D
	//------------------------------------------------------------------------------------------------------
	ShutdownD3D();


	//------------------------------------------------------------------------------------------------------
	// We're done
	//------------------------------------------------------------------------------------------------------
	return 0;
};


//###################################################################################################################
// OpenXR Methods
//###################################################################################################################
bool InitXr() {
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

		// Now call the xrEnumerateSwapchainImages function again, this time with the 2nd param set to the number
		// of swapchain images that got created by OpenXR. That way, we can store the swapchain images into our
		// swwapchain
		result = xrEnumerateSwapchainImages(swapchain_handle, swapchain_image_count, &swapchain_image_count, (XrSwapchainImageBaseHeader*)swapchain.swapchain_images.data());
		if (XR_FAILED(result)) {
			return false;
		}

		// For each swapchain image, call the function to create a render target using that swapchain image
		for (uint32_t i = 0; i < swapchain_image_count; i++) {
			swapchain.swapchain_data[i] = CreateSwapchainRenderTargets(swapchain.swapchain_images[i]);
		}

		// We're done creating that swapchain, we can now add it to the vector of our swapchains (as we have multiple,
		// as mentioned before one for each view
		xr_swapchains.push_back(swapchain);

	}

	return true;
}

bool InitXrActions() {
	// IMPLEMENT ME
	return true;
}

void PollOpenXrEvents(bool& loop_running, bool& xr_running) {
	XrResult result;

	// "Reset" this to true, if we need to exit the loop because of a
	// state change, we'll change this value later
	loop_running = true;

	// Struct to save the event data in.
	// We'll pass this to the xrPollEvent call, where the details of the
	// event will be stored in thhis struct.
	XrEventDataBuffer event_data_buffer = { XR_TYPE_EVENT_DATA_BUFFER };

	// Keep polling events as long as xrPollEvent returns events.
	// The call to xrPollEvent fetches the "next event" of the xr_instance,
	// and stores the event data inside the event data buffer we created earlier.
	// We use the XR_UNQUALIFIED_SUCCESS macro to see if the xrPollEvent call
	// was successful (i.e. was able to fetch an event)
	// If there are no more events left over, we can end.
	while (XR_UNQUALIFIED_SUCCESS(xrPollEvent(xr_instance, &event_data_buffer))) {
		// State of the session changed, maybe we need to do something
		if (event_data_buffer.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
			XrEventDataSessionStateChanged* state_change = (XrEventDataSessionStateChanged*)&event_data_buffer;

			// Set the global state of the xr session to the state we just got from the call
			// to xrPollEvent
			xr_session_state = state_change->state;

			switch (xr_session_state) {
				// Session is in the READY state, which means we can call xrBeginSession to
				// enter the SYNCHRONITED state (see OpenXR reference guide) at the Khronos
				// website: https://www.khronos.org/files/openxr-10-reference-guide.pdf
				// for more details about the state transitions
				case XR_SESSION_STATE_READY: {
					// We need to supply the call to xrBeginSession a valid XrSessionBeginInfo
					// struct. The only thing we need to set here is the primaryViewConfigurationType
					// field, which we configured previously. Usually, for head mounted devices
					// such as a VR headset or the Hololens, this is XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO
					// (i.e. two images will be rendered)
					XrSessionBeginInfo session_begin_info = {};
					session_begin_info.type = XR_TYPE_SESSION_BEGIN_INFO;
					session_begin_info.primaryViewConfigurationType = app_config_view;

					// Start the session. If the call was successful, set the xr_running flag
					// to true
					result = xrBeginSession(xr_session, &session_begin_info);
					if (XR_FAILED(result)) {
						MessageBox(NULL, "Couldn't begin the XR session.", "Error", MB_OK);
						return;
					}
					xr_running = true;
					break;
				}
				case XR_SESSION_STATE_STOPPING: {
					// Session is in the STOPPING state, where we need to call xrEndSession
					// to enter the IDLE state
					xr_running = false;
					result = xrEndSession(xr_session);
					if (XR_FAILED(result)) {
						MessageBox(NULL, "Couldn't end the XR session.", "Error", MB_OK);
						return;
					}
					break;
				}
				case XR_SESSION_STATE_EXITING: {
					// State after the user quit, here we need to break out from the loop
					// and quit the application
					loop_running = false;
					break;
				}
				case XR_SESSION_STATE_LOSS_PENDING: {
					// The runtime is losing the system or the device, need to break
					// out from the main loop and quit the application
					loop_running = false;
					break;
				}
			}
		}
		// Instance seems to be shutting down, need to break out from the main loop
		// and quit the aplication
		else if (event_data_buffer.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
			loop_running = false;
			return;
		}
		
		// "Reset" the event data buffer, such that it's ready for the next event
		// being fetched (if there is any to be fetched)
		event_data_buffer = { XR_TYPE_EVENT_DATA_BUFFER };
	}
}

void PollOpenXrActions() {
	// IMPLEMENT ME => First need to setup actions, but let's skip that for now
};

void RenderOpenXrFrame() {
	XrResult result;

	//------------------------------------------------------------------------------------------------------
	// Setup the frame 
	//------------------------------------------------------------------------------------------------------
	// The call to xrWait frame will fill in the frame_state struct, where the field we'll
	// be interested in is the predictedDisplayTime field.
	// That field stores a prediction when the next frame will be displayed. This can be used to
	// place objects, viewpoints, controllers etc. in the view
	XrFrameState frame_state = {};
	frame_state.type = XR_TYPE_FRAME_STATE;
	result = xrWaitFrame(xr_session, NULL, &frame_state);
	if (XR_FAILED(result)) {
		return;
	}

	//------------------------------------------------------------------------------------------------------
	// Begin the frame 
	//------------------------------------------------------------------------------------------------------
	result = xrBeginFrame(xr_session, NULL);
	if (XR_FAILED(result)) {
		return;
	}

	//------------------------------------------------------------------------------------------------------
	// Call to UpdateSimulation which will update the simulation for the predicted rendering time
	//------------------------------------------------------------------------------------------------------
	UpdateSimulation(frame_state.predictedDisplayTime);

	//------------------------------------------------------------------------------------------------------
	// Render the layer
	//------------------------------------------------------------------------------------------------------
	XrCompositionLayerBaseHeader* layer = nullptr;
	XrCompositionLayerProjection layer_projection = {};
	layer_projection.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
	std::vector<XrCompositionLayerProjectionView> views;

	// Check if the xrSession is in a state that we actually need to render. If the session isn't in the
	// VISIBLE or in the FOCUSED state, we don't need to render the layer (e.g. when the user of the
	// application takes off the vr headset while the application still is running. In that case,
	// we need to keep the application (and the simulation) running, but there is no point in rendering
	// anything.
	bool session_active = xr_session_state == XR_SESSION_STATE_VISIBLE || xr_session_state == XR_SESSION_STATE_FOCUSED;
	uint32_t layer_count = 0;

	if (session_active) {
		RenderOpenXrLayer(frame_state.predictedDisplayTime, views, layer_projection);
		layer = (XrCompositionLayerBaseHeader*)&layer_projection;
		layer_count = 1;
	}

	//------------------------------------------------------------------------------------------------------
	// Done rendeding the layer, send it to the display
	//------------------------------------------------------------------------------------------------------
	XrFrameEndInfo frame_end_info = {};
	frame_end_info.type = XR_TYPE_FRAME_END_INFO;
	frame_end_info.displayTime = frame_state.predictedDisplayTime;
	frame_end_info.environmentBlendMode = xr_blend_mode;
	frame_end_info.layerCount = layer_count;
	frame_end_info.layers = &layer;
	xrEndFrame(xr_session, &frame_end_info);
};

void RenderOpenXrLayer(XrTime predicted_time, std::vector<XrCompositionLayerProjectionView>& views, XrCompositionLayerProjection& layer_projection) {
	uint32_t view_count = 0;

	//------------------------------------------------------------------------------------------------------
	// Setup the views for the predicted rendering time
	//------------------------------------------------------------------------------------------------------
	// We got the predicted time from the OpenXR runtime at which it will render the next frame (i.e. the
	// frame we're preparing to render right now.
	// We can use this predicted time to call the method xrLocateViews, which will return the view and
	// projection info for a particular time.
	XrViewState view_state = {};
	view_state.type = XR_TYPE_VIEW_STATE;

	// Setup an info struct which we'll pass into the xrLocateView call with informations about the
	// predicted time, the type of view we have and the xr space we're in
	XrViewLocateInfo view_locate_info = {};
	view_locate_info.type = XR_TYPE_VIEW_LOCATE_INFO;
	view_locate_info.viewConfigurationType = app_config_view;
	view_locate_info.displayTime = predicted_time;
	view_locate_info.space = xr_app_space;

	// Call xrLocateViews, which will give us the number of views we have to render (stored in view_count), as
	// well as fill in the xr_views vector with the predicted views (which is basically a struct containing
	// the pose of the view, as well as the fov for that view. We'll use these two later to render with D3D11,
	// as we need to modify the objects and the view before rendering.
	xrLocateViews(xr_session, &view_locate_info, &view_state, (uint32_t)xr_views.size(), &view_count, xr_views.data());
	views.resize(view_count);

	//------------------------------------------------------------------------------------------------------
	// Render the layer for each view
	//------------------------------------------------------------------------------------------------------
	for (uint32_t i = 0; i < view_count; i++) {
		// First, we need to acquire a swapchain image, as we need a render target to render the data
		// to. As a reminder (from the CreateSwapchainRenderTargets method), a swapchain image
		// in the context of D3D11 is the buffer we want to render to.
		// As we don't pass a swapchain_image_id into the xrAcquireSwapchainImage call, the runtime decides
		// which swapchain image we'll get
		uint32_t swapchain_image_id;
		XrSwapchainImageAcquireInfo swapchain_acquire_info = {};
		swapchain_acquire_info.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
		xrAcquireSwapchainImage(xr_swapchains[i].handle, &swapchain_acquire_info, &swapchain_image_id);

		// We need to wait until the swapchain image is available for writing, as the compositor
		// could still be reading from it (writing while the compositor is still reading could
		// result in tearing or otherwise badly rendered frames).
		// For now, we set the timeout to infinite, but one could also set another timeout
		// by passing in an xrDuration
		XrSwapchainImageWaitInfo swapchain_wait_info = {};
		swapchain_wait_info.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
		swapchain_wait_info.timeout = XR_INFINITE_DURATION;
		xrWaitSwapchainImage(xr_swapchains[i].handle, &swapchain_wait_info);

		// Setup the info we need to render the layer for the current view. The XrCompositionLayerProjectionView
		// is a projection layer element, which has the pose of the current view (pose = location and orientation),
		// the fov of the current view, and the swapchain sub image, which holds the data for the composition
		// layer.
		// The subimage is of type XrSwapchainSubImage, which has a field to the swapchain to display and an
		// imageRect, which represents the valid portion of the image to use (in pixels)
		views[i] = {};
		views[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
		views[i].pose = xr_views[i].pose;
		views[i].fov = xr_views[i].fov;
		views[i].subImage.swapchain = xr_swapchains[i].handle;
		views[i].subImage.imageRect.offset = { 0, 0 };
		views[i].subImage.imageRect.extent = { xr_swapchains[i].width, xr_swapchains[i].height };

		// Call the RenderD3D method, which will call the Draw method which will eventually render the
		// content to the swapchain. With this call hierarchy, it should be possible to simply adapt the
		// Draw method if other content is to be rendered
		RenderD3DLayer(views[i], xr_swapchains[i].swapchain_data[swapchain_image_id]);

		// We're done rendering for the current view, so we can release the swapchain image (i.e. tell
		// the OpenXR runtime that we're done with this swapchain image.
		// We have to pass in a XrSwapchainImageReleaseInfo, but at the moment, this struct doesn't
		// do anything special.
		XrSwapchainImageReleaseInfo swapchain_release_info = {};
		swapchain_release_info.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
		xrReleaseSwapchainImage(xr_swapchains[i].handle, &swapchain_release_info);
	}

	//------------------------------------------------------------------------------------------------------
	// Set the rendered data to be displayed
	//------------------------------------------------------------------------------------------------------
	// Now thta we're done rendering all views, we can update the layer projection we got passed into
	// the method with the rendered views, such that we can display them.
	layer_projection.space = xr_app_space;
	layer_projection.viewCount = (uint32_t)views.size();
	layer_projection.views = views.data();
};

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

bool InitD3DPipeline() {
	HRESULT result;
	//----------------------------------------------------------------------------------
	// Compile the shaders and create the pixel & vertex shaders
	//----------------------------------------------------------------------------------
	ID3D10Blob* vert_shader_blob;
	ID3D10Blob* pixel_shader_blob;
	ID3D10Blob* errors;

	// Compile the vertex shader
	D3DCompileFromFile(L"shaders.shader", 0, 0, "VShader", "vs_5_0", D3D10_SHADER_OPTIMIZATION_LEVEL3, 0, &vert_shader_blob, &errors);
	if (errors) {
		MessageBox(NULL, "The vertex shader failed to compile.", "Error", MB_OK);
		return false;
	}

	// Compile the pixel shader
	D3DCompileFromFile(L"shaders.shader", 0, 0, "PShader", "ps_5_0", D3D10_SHADER_OPTIMIZATION_LEVEL3, 0, &pixel_shader_blob, &errors);
	if (errors) {
		MessageBox(NULL, "The pixel shader failed to compile.", "Error", MB_OK);
		return false;
	}

	// Encapsulate both shaders into shader objects
	result = d3d_device->CreateVertexShader(vert_shader_blob->GetBufferPointer(), vert_shader_blob->GetBufferSize(), NULL, &d3d_vertex_shader);
	if (FAILED(result)) {
		return false;
	}
	result = d3d_device->CreatePixelShader(pixel_shader_blob->GetBufferPointer(), pixel_shader_blob->GetBufferSize(), NULL, &d3d_pixel_shader);
	if (FAILED(result)) {
		return false;
	}

	// Set the shader objects
	d3d_device_context->VSSetShader(d3d_vertex_shader, 0, 0);
	d3d_device_context->PSSetShader(d3d_pixel_shader, 0, 0);

	//----------------------------------------------------------------------------------
	// Create the input layout. This describes to the GPU how the data is arranged
	//----------------------------------------------------------------------------------

	// For now, we'll only be using the position and the normal of the vertices
	D3D11_INPUT_ELEMENT_DESC input_desc[] = {
		{"SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
	result = d3d_device->CreateInputLayout(input_desc, _countof(input_desc), vert_shader_blob->GetBufferPointer(), vert_shader_blob->GetBufferSize(), &d3d_input_layout);
	if (FAILED(result)) {
		return false;
	}

	// Tell the GPU to use that input layout
	d3d_device_context->IASetInputLayout(d3d_input_layout);

	//----------------------------------------------------------------------------------
	// Create the constant buffer
	//----------------------------------------------------------------------------------
	D3D11_BUFFER_DESC const_buffer_desc;
	ZeroMemory(&const_buffer_desc, sizeof(const_buffer_desc));
	const_buffer_desc.ByteWidth = sizeof(const_buffer_t);
	const_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	result = d3d_device->CreateBuffer(&const_buffer_desc, NULL, &d3d_const_buffer);
	if (FAILED(result)) {
		return false;
	}

	// And now set the constant buffer
	d3d_device_context->VSSetConstantBuffers(0, 1, &d3d_const_buffer);

	return true;
};

bool InitD3DGraphics() {
	HRESULT result;

	//----------------------------------------------------------------------------------
	// Vertex buffer 
	//----------------------------------------------------------------------------------

	// Create the vertex buffer
	D3D11_BUFFER_DESC vert_buffer_desc;
	ZeroMemory(&vert_buffer_desc, sizeof(vert_buffer_desc));
	vert_buffer_desc.ByteWidth = sizeof(vertices);
	vert_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	// Create the buffer and copy the vertices into it as initial data
	D3D11_SUBRESOURCE_DATA vert_buff_data = { vertices };
	result = d3d_device->CreateBuffer(&vert_buffer_desc, &vert_buff_data, &d3d_vertex_buffer);
	if (FAILED(result)) {
		return false;
	}

	//----------------------------------------------------------------------------------
	// Index buffer 
	//----------------------------------------------------------------------------------
	D3D11_BUFFER_DESC index_buffer_desc;
	ZeroMemory(&index_buffer_desc, sizeof(index_buffer_desc));
	index_buffer_desc.ByteWidth = sizeof(indices);
	index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	// Create the buffer and copy the indices into it as initial data
	D3D11_SUBRESOURCE_DATA index_buffer_data = { indices };
	result = d3d_device->CreateBuffer(&index_buffer_desc, &index_buffer_data, &d3d_index_buffer);
	if (FAILED(result)) {
		return false;
	}

	return true;
};

void ShutdownD3D() {
	if (d3d_device_context) {
		d3d_device_context->Release();
	}

	if (d3d_device) {
		d3d_device->Release();
	}
}

void RenderD3DLayer(XrCompositionLayerProjectionView& view, swapchain_data_t& swapchain_data) {
	//----------------------------------------------------------------------------------
	// Setup viewport
	//----------------------------------------------------------------------------------
	// For D3D11 to render correctly, we need to create a D3D11_VIEWPORT struct and
	// set the top left XY coordinates of the viewport, as well as the width and height
	// of the viewport.
	// As this should match the size of the swapchain, we just use the size of the
	// subimage we set previously
	XrRect2Di& image_rect = view.subImage.imageRect;
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = (float)image_rect.offset.x;
	viewport.TopLeftY = (float)image_rect.offset.y;
	viewport.Width = (float)image_rect.extent.width;
	viewport.Height = (float)image_rect.extent.height;

	// Now we can set the viewport of the device context
	d3d_device_context->RSSetViewports(1, &viewport);

	//----------------------------------------------------------------------------------
	// Clear the Buffers
	//----------------------------------------------------------------------------------
	// It's usually nessecary to clear the backbuffer (as it usually still contains the
	// data from the previous frame). This is usually done by setting all the data
	// (pixels) to a single color.
	float clear_color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	d3d_device_context->ClearRenderTargetView(swapchain_data.back_buffer, clear_color);

	// Also clear the depth buffer, such that it's ready for rendering
	d3d_device_context->ClearDepthStencilView(swapchain_data.depth_buffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//----------------------------------------------------------------------------------
	// Set the render target
	//----------------------------------------------------------------------------------
	// Now we can set the target of all render operations to the backbuffer of the
	// swapchain we're using.
	// This will render all our content to that backbuffer.
	d3d_device_context->OMSetRenderTargets(1, &swapchain_data.back_buffer, swapchain_data.depth_buffer);

	Draw(view);
};

// Helper method that takes a XrCompositionLayerProjectionView and calculates the
// ViewProjection matrix from it, such that we can pass that matrix to the constant buffer
// and finally to the shader to correctly transform the objects.
DirectX::XMMATRIX CreateViewProjectionMatrix(XrCompositionLayerProjectionView& view) {
	//----------------------------------------------------------------------------------
	// Build projection matrix
	//----------------------------------------------------------------------------------
	// Define the far plane and the near plane. A value often used for the near
	// clipping in desktop applications is 1.0f, however that seems to be too high
	// for XR applications, as objects "relatively" close to the user already vanish
	// from the view, even when you would expect them not to
	const float near_clipping = 0.05f;
	const float far_clipping = 100.0f;

	// Construct the left, right, top and bottom values to pass into the
	// XMMatrixPerspectiveOffCenterRH call. These 4 values represent the x and y
	// coordinates respectively of the view frustum at the near plane.
	const float left = near_clipping * tanf(view.fov.angleLeft);
	const float right = near_clipping * tanf(view.fov.angleRight);
	const float top = near_clipping * tanf(view.fov.angleUp);
	const float bottom = near_clipping * tanf(view.fov.angleDown);

	// Construct the projection matrix from the values we just calculated
	DirectX::XMMATRIX projection_matrix = DirectX::XMMatrixPerspectiveOffCenterRH(left, right, bottom, top, near_clipping, far_clipping);

	//----------------------------------------------------------------------------------
	// Build view matrix
	//----------------------------------------------------------------------------------
	// Load the orientation quaternion into a vector, such that we can use it to construct
	// an affine transformation
	DirectX::XMVECTOR view_rotation = DirectX::XMLoadFloat4((DirectX::XMFLOAT4*) & view.pose.orientation);

	// Also load the position vector into a 3-component vector
	DirectX::GXMVECTOR view_translation = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*) & view.pose.position);

	// Build an affine transformation, represented as a matrix, of the view transformation
	// The first argument is the scaling factor. As we don't want to scale anything right now,
	// we pass in a vector with all ones.
	// The second argument is the point identifying the origin of the rotation, which we set
	// to (0, 0, 0), such that we rotate about the origin
	// As third argument we pass the rotation we loaded before, and as fourth argument
	// the translation we loaded before. 
	DirectX::XMMATRIX view_transformation_matrix = DirectX::XMMatrixAffineTransformation(DirectX::g_XMOne, DirectX::g_XMZero, view_rotation, view_translation);

	// We then need to inverse the matrix. The first argument could be a pointer to a
	// XMVECTOR which would store the determinant. As we don't care about the
	// determinant, we just pass in NULL
	DirectX::XMMATRIX view_matrix = DirectX::XMMatrixInverse(NULL, view_transformation_matrix);

	// Finally, return the Transpose of the product of the view matrix and the projection
	// matrix
	return DirectX::XMMatrixTranspose(view_matrix * projection_matrix);
}

//###################################################################################################################
// App Methods
//###################################################################################################################
void UpdateSimulation(XrTime predicted_time) {
	cube_rotation_angles.x += 0.02f;
	cube_rotation_angles.y += 0.04f;
}

void Draw(XrCompositionLayerProjectionView& view) {
	//----------------------------------------------------------------------------------
	// Setup
	//----------------------------------------------------------------------------------
	// Use the helper method to create the view-projection matrix
	DirectX::XMMATRIX view_projection_matrix = CreateViewProjectionMatrix(view);

	// Create the transform buffer struct, which stores the data we want to pass
	// to the shaders
	const_buffer_t transform_buffer;

	// Store the view-projection matrix in the transform buffer struct
	DirectX::XMStoreFloat4x4(&transform_buffer.view_projection, view_projection_matrix);

	//----------------------------------------------------------------------------------
	// Set buffers and primitive topology
	//----------------------------------------------------------------------------------
	UINT stride = sizeof(vertex_t);
	UINT offset = 0;
	// Set vertex buffer to use
	d3d_device_context->IASetVertexBuffers(0, 1, &d3d_vertex_buffer, &stride, &offset);

	// We'll also need to set the index buffer to be able to draw the triangles.
	// As the type of an index is uint16_t, we'll use the DXGI_FORMAT_R16_UINT
	// format
	d3d_device_context->IASetIndexBuffer(d3d_index_buffer, DXGI_FORMAT_R16_UINT, 0);

	// And finally we'll tell the renderer that we want to render a trianglelist
	d3d_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//----------------------------------------------------------------------------------
	// Setup lighting
	//----------------------------------------------------------------------------------
	transform_buffer.light_vector = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);
	transform_buffer.light_color = { 0.5f, 0.5f, 0.5f, 1.0f };
	transform_buffer.ambient_color = { 0.2f, 0.2f, 0.2f, 1.0f };

	//----------------------------------------------------------------------------------
	// Draw a single cube
	//----------------------------------------------------------------------------------
	// Load the rotation angles (roll, pitch, yaw) into a XMVECTOR
	DirectX::XMVECTOR rotation_angles = DirectX::XMLoadFloat3(&cube_rotation_angles);

	// Get the rotation we'll apply to the cube. Here, we need a quaternion
	DirectX::XMVECTOR model_rotation = DirectX::XMQuaternionRotationRollPitchYawFromVector(rotation_angles);

	// Get the translation which we'll apply to the cube. We'll also use the xr_pose_identity,
	// which means the cube is positioned at (0, 0, 0), i.e. the origin.
	DirectX::GXMVECTOR model_translation = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*) &xr_pose_identity.position);

	// Set the scaling factor of the cube. We'll scale it down by a factor of 10, i.e.
	// we'll multiply the unit-vector by 0.1 in the affine translation
	float scaling_factor = 0.1f;

	// Setup the affine translation which we'll use for our cube
	DirectX::XMMATRIX model_matrix = DirectX::XMMatrixAffineTransformation(DirectX::g_XMOne * scaling_factor, DirectX::g_XMZero, model_rotation, model_translation);

	// Store the model matrix for our cube in the constant buffer for the shader
	DirectX::XMStoreFloat4x4(&transform_buffer.world, DirectX::XMMatrixTranspose(model_matrix));

	// We also need to store the rotation matrix (of the cube), as we need it to correctly
	// light up the cube. Here, we need a vector instead of a quaternion
	DirectX::XMMATRIX object_rotation = DirectX::XMMatrixRotationRollPitchYawFromVector(rotation_angles);
	DirectX::XMStoreFloat4x4(&transform_buffer.rotation, object_rotation);

	// Send the constant buffer to the GPU, such that the shader can use it
	d3d_device_context->UpdateSubresource(d3d_const_buffer, 0, NULL, &transform_buffer, 0, 0);

	// And now we tell the GPU to draw our vertices
	d3d_device_context->DrawIndexed((UINT)_countof(indices), 0, 0);
}
