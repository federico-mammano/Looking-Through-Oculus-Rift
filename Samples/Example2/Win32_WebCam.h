/************************************************************************************
Filename    :   Win32_WebCam.h
Content     :   Webcam class for Oculus Rift test application
Created     :   December 20th, 2014
Authors     :   Federico Mammano

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*************************************************************************************/

#ifndef WEBCAM_H_
#define WEBCAM_H_

#define WEBCAM_NB						2		// Number of webcams: 1 for cyclops/non-stereo mode or 2 for stereo mode
#define WEBCAM_0_DEVICE_NUMBER			0		// The device number for webcam 0 (eg.: Left Eye) among connected ones. If you have 2 webcams and they are inverted, swith the number with WEBCAM_1_DEVICE_NUMBER!
#define WEBCAM_0_VERT_ORIENTATION		true	// Is webcam 0 (eg.: Left Eye) vertically positioned?
#define WEBCAM_0_HMD_FOV_RATIO			1.0f	// The ratio: (Web Cam Diagonal Field of View) / (Oculus Rift Eye Field of View) for webcam 0 (eg.: Left Eye)
#define WEBCAM_1_DEVICE_NUMBER			1		// The device number for webcam 1 (eg.: Right Eye) among connected ones. If you have 2 webcams and they are inverted, swith the number with WEBCAM_0_DEVICE_NUMBER!
#define WEBCAM_1_VERT_ORIENTATION		true	// Is webcam 1 (eg.: Right Eye) vertically positioned?
#define WEBCAM_1_HMD_FOV_RATIO			1.0f	// The ratio: (Web Cam Diagonal Field of View) / (Oculus Rift Eye Field of View) for webcam 1 (eg.: Right Eye)

#include "opencv2/highgui/highgui.hpp"	// Include OpenCV
#include <opencv2/imgproc/imgproc.hpp>
#include "Kernel/OVR_Log.h"
#include "Kernel/OVR_Threads.h"
#include "Hand.h"

#if RENDER_OPENGL																							// Buffer Object
bool bIsBOSupported						= false;
#else
bool bIsBOSupported						= true;
#endif
float fHMDEyeAspectRatio				= 0.888889f;
bool bLookThrough						= true;

// Simple Quad
Model::Vertex SimpleQuadVertices[]		= { { Vector3f(-1.0f,  -1.0f, 0.0f), Model::Color(255, 255, 255, 200), 0.0f, 0.0f },
											{ Vector3f( 1.0f,  -1.0f, 0.0f), Model::Color(255, 255, 255, 200), 1.0f, 0.0f },
											{ Vector3f( 1.0f,	1.0f, 0.0f), Model::Color(255, 255, 255, 200), 1.0f, 1.0f },
											{ Vector3f(-1.0f,	1.0f, 0.0f), Model::Color(255, 255, 255, 200), 0.0f, 1.0f } };
uint16_t SimpleQuadIndices[]			= { 0, 1, 2, 3, 0, 2 };

// Fading Edge Quad
float fBandWidth						= 0.1f;																// Fading Edge Percentage Width (eg.: 0.1 = 10% width)
float fBandHeight						= 0.1f;																// Fading Edge Percentage Height
Model::Vertex FadingEdgeQuadVertices[]	= { // Bottom Left
											{ Vector3f(-1.0f, -1.0f, 0.0f),							Model::Color(255, 255, 255, 0),		0.0f, 0.0f },
											{ Vector3f(-1.0f+fBandWidth, -1.0f, 0.0f),				Model::Color(255, 255, 255, 0),		0.5f*fBandWidth, 0.0f },
											{ Vector3f(-1.0f+fBandWidth, -1.0f+fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	0.5f*fBandWidth, 0.5f*fBandHeight },
											{ Vector3f(-1.0f, -1.0f+fBandHeight, 0.0f),				Model::Color(255, 255, 255, 0),		0.0f, 0.5f*fBandHeight },
											// Bottom Center
											{ Vector3f(-1.0f+fBandWidth, -1.0f+fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	0.5f*fBandWidth, 0.5f*fBandHeight },
											{ Vector3f(-1.0f+fBandWidth, -1.0f, 0.0f),				Model::Color(255, 255, 255, 0),		0.5f*fBandWidth, 0.0f },
											{ Vector3f(1.0f-fBandWidth, -1.0f, 0.0f),				Model::Color(255, 255, 255, 0),		1.0f-0.5f*fBandWidth, 0.0f },
											{ Vector3f(1.0f-fBandWidth, -1.0f+fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	1.0f-0.5f*fBandWidth, 0.5f*fBandHeight },
											// Bottom Right
											{ Vector3f(1.0f-fBandWidth, -1.0f+fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	1.0f-0.5f*fBandWidth, 0.5f*fBandHeight },
											{ Vector3f(1.0f-fBandWidth, -1.0f, 0.0f),				Model::Color(255, 255, 255, 0),		1.0f-0.5f*fBandWidth, 0.0f },
											{ Vector3f(1.0f, -1.0f, 0.0f),							Model::Color(255, 255, 255, 0),		1.0f, 0.0f },
											{ Vector3f(1.0f, -1.0f+fBandHeight, 0.0f),				Model::Color(255, 255, 255, 0),		1.0f, 0.5f*fBandHeight },
											// Center Left
											{ Vector3f(-1.0f, 1.0f-fBandHeight, 0.0f),				Model::Color(255, 255, 255, 0),		0.0f, 1.0f-0.5f*fBandHeight },
											{ Vector3f(-1.0f, -1.0f+fBandHeight, 0.0f),				Model::Color(255, 255, 255, 0),		0.0f, 0.5f*fBandHeight },
											{ Vector3f(-1.0f+fBandWidth, -1.0f+fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	0.5f*fBandWidth, 0.5f*fBandHeight },
											{ Vector3f(-1.0f+fBandWidth, 1.0f-fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	0.5f*fBandWidth, 1.0f-0.5f*fBandHeight },
											// Center
											{ Vector3f(-1.0f+fBandWidth, 1.0f-fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	0.5f*fBandWidth, 1.0f-0.5f*fBandHeight },
											{ Vector3f(-1.0f+fBandWidth, -1.0f+fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	0.5f*fBandWidth, 0.5f*fBandHeight },
											{ Vector3f(1.0f-fBandWidth, -1.0f+fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	1.0f-0.5f*fBandWidth, 0.5f*fBandHeight },
											{ Vector3f(1.0f-fBandWidth, 1.0f-fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	1.0f-0.5f*fBandWidth, 1.0f-0.5f*fBandHeight },
											// Center Right
											{ Vector3f(1.0f-fBandWidth, 1.0f-fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	1.0f-0.5f*fBandWidth, 1.0f-0.5f*fBandHeight },
											{ Vector3f(1.0f-fBandWidth, -1.0f+fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	1.0f-0.5f*fBandWidth, 0.5f*fBandHeight },
											{ Vector3f(1.0f, -1.0f+fBandHeight, 0.0f),				Model::Color(255, 255, 255, 0),		1.0f, 0.5f*fBandHeight },
											{ Vector3f(1.0f, 1.0f-fBandHeight, 0.0f),				Model::Color(255, 255, 255, 0),		1.0f, 1.0f-0.5f*fBandHeight },
											// Top Left
											{ Vector3f(-1.0f, 1.0f, 0.0f),							Model::Color(255, 255, 255, 0),		0.0f, 1.0f },
											{ Vector3f(-1.0f, 1.0f-fBandHeight, 0.0f),				Model::Color(255, 255, 255, 0),		0.0f, 1.0f-0.5f*fBandHeight },
											{ Vector3f(-1.0f+fBandWidth, 1.0f-fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	0.5f*fBandWidth, 1.0f-0.5f*fBandHeight },
											{ Vector3f(-1.0f+fBandWidth, 1.0f, 0.0f),				Model::Color(255, 255, 255, 0),		0.5f*fBandWidth, 1.0f },
											// Top Center
											{ Vector3f(-1.0f+fBandWidth, 1.0f, 0.0f),				Model::Color(255, 255, 255, 0),		0.5f*fBandWidth, 1.0f },
											{ Vector3f(-1.0f+fBandWidth, 1.0f-fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	0.5f*fBandWidth, 1.0f-0.5f*fBandHeight },
											{ Vector3f(1.0f-fBandWidth, 1.0f-fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	1.0f-0.5f*fBandWidth, 1.0f-0.5f*fBandHeight },
											{ Vector3f(1.0f-fBandWidth, 1.0f, 0.0f),				Model::Color(255, 255, 255, 0),		1.0f-0.5f*fBandWidth, 1.0f },
											// Top Right
											{ Vector3f(1.0f-fBandWidth, 1.0f-fBandHeight, 0.0f),	Model::Color(255, 255, 255, 255),	1.0f-0.5f*fBandWidth, 1.0f-0.5f*fBandHeight },
											{ Vector3f(1.0f, 1.0f-fBandHeight, 0.0f),				Model::Color(255, 255, 255, 0),		1.0f, 1.0f-0.5f*fBandHeight },
											{ Vector3f(1.0f, 1.0f, 0.0f),							Model::Color(255, 255, 255, 0),		1.0f, 1.0f },
											{ Vector3f(1.0f-fBandWidth, 1.0f, 0.0f),				Model::Color(255, 255, 255, 0),		1.0f-0.5f*fBandWidth, 1.0f } 
										  };
uint16_t FadingEdgeQuadIndices[]		= { 0,  1,  2,  3,  0,  2,								
											4,  5,  6,  7,  4,  6,
											8,  9,  10, 11, 8,  10,
											12, 13, 14, 15, 12, 14,
											16, 17, 18, 19, 16, 18,
											20, 21, 22, 23, 20, 22,
											24, 25, 26, 27, 24, 26,
											28, 29, 30, 31, 28, 30,
											32, 33, 34, 35, 32, 34 };
// Shader
#if RENDER_OPENGL
ShaderFill::VertexAttribDesc QuadVertDesc[] = { {"Position", 3, GL_FLOAT,		  false, offsetof(Model::Vertex, Pos)},
											    {"Color",    4, GL_UNSIGNED_BYTE, true,  offsetof(Model::Vertex, C)},
												{"TexCoord", 2, GL_FLOAT,		  false, offsetof(Model::Vertex, U)} };
char* VertexShaderSrc					= "#version 110\n"
										  "uniform mat4 Proj, View;\n"
										  "attribute vec4 Position, Color;\n"
										  "attribute vec2 TexCoord;\n"
										  "varying vec4 oColor;\n"
										  "varying vec2 oTexCoord;\n"
										  "void main() { gl_Position = Proj * (View * Position); oTexCoord = TexCoord; oColor = Color; }";
char* PixelShaderSrc					= "#version 110\n"
										  "uniform sampler2D Texture0;\n"
										  "varying vec4 oColor;\n"
										  "varying vec2 oTexCoord;\n"
										  "void main() { gl_FragColor = oColor * texture2D(Texture0, oTexCoord); }\n";
#else
D3D11_INPUT_ELEMENT_DESC QuadVertDesc[]	= {	{"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Model::Vertex, Pos),   D3D11_INPUT_PER_VERTEX_DATA, 0},
											{"Color",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, offsetof(Model::Vertex, C),     D3D11_INPUT_PER_VERTEX_DATA, 0},
											{"TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Model::Vertex, U),     D3D11_INPUT_PER_VERTEX_DATA, 0} };
char* VertexShaderSrc					= "float4x4 Proj, View;"
										  "void main(in  float4 Position  : POSITION,    in  float4 Color : COLOR0, in  float2 TexCoord  : TEXCOORD0,"
										  "          out float4 oPosition : SV_Position, out float4 oColor: COLOR0, out float2 oTexCoord : TEXCOORD0)"
										  "{ oPosition = mul(Proj, mul(View, Position)); oTexCoord = TexCoord; oColor = Color; }";
char* PixelShaderSrc					= "Texture2D Texture   : register(t0); SamplerState Linear : register(s0);"
										  "float4 main(in float4 Position : SV_Position, in float4 Color: COLOR0, in float2 TexCoord : TEXCOORD0) : SV_Target"
										  "{ return Color * Texture.Sample(Linear, TexCoord); }";
#endif 


// ==================================================================================//
//  WebCamDevice Class
// ==================================================================================//
class WebCamDevice
{
private:

	cv::Mat						Frame;
    cv::VideoCapture			Video;  
	OVR::Thread					CaptureFrameThread;
	OVR::Mutex				   *pMutex;
	int							iStatus;
	bool						bHasCapturedFrame;
	bool						bIsVertOriented;
	ImageBuffer				   *pImageBuffer;
	ShaderFill				   *pShaderFill;
	Model					   *pQuadModel;
	Model					   *pFadingEdgeQuadModel;
#if RENDER_OPENGL
	int							iCurrentBOIndex;
	GLuint						uiBOIDs[2];
#else
	Ptr<ID3D11BlendState>		BlendState;
	unsigned char			   *pucRGBAData;																			// Buffer to convert BGR WebCam Frame to RGBA format
#endif

public:

	int							iWidth, iHeight, iBufferSize, iColorChannels;
	float						fAspectRatio;
	float						fWebCamHMD_DiagonalFOVRatio;															// The ratio: (Web Cam Diagonal Field of View) / (Oculus Rift Eye Field of View)
																														// For a fullscreen feeling take this parameter to 1.0f 
																														// For a more realistic feeling (depending on your webcam lens) espress this ratio
public:

	// Constructor
	WebCamDevice() : iStatus(OVR::Thread::NotRunning), pMutex(NULL), bHasCapturedFrame(false), 
					 pImageBuffer(NULL), pShaderFill(NULL), pQuadModel(NULL), pFadingEdgeQuadModel(NULL)
	{
	#if RENDER_OPENGL
		iColorChannels	= 3;																							// 3 for BGR format
		iCurrentBOIndex	= 0; 
	#else
		iColorChannels	= 4;																							// 4 for RGBA format (R8G8B8A8)
		pucRGBAData		= NULL;
	#endif
	}

    // Destructor
    ~WebCamDevice()
	{
	#if RENDER_OPENGL
		if(bIsBOSupported) { glDeleteBuffers(2, uiBOIDs); }
	#endif
	}

	int Initialize(int iDeviceNum, bool bVOriented=false, float fDiagonalFOVRatio=1.0f) 
	{
		Video.open(iDeviceNum);																							// Open the WebCam number iDevice
		if(!Video.isOpened()) 
		{ 
			char msg[100];
			sprintf_s(msg, 100, "Cannot open the video of WebCam number %d", iDeviceNum);
			MessageBoxA(NULL,msg,"", MB_OK); 
			return(0); 
		}
		iWidth			= (int)Video.get(CV_CAP_PROP_FRAME_WIDTH);														// Get the Width of frames of the WebCam Video
		iHeight			= (int)Video.get(CV_CAP_PROP_FRAME_HEIGHT);														// Get the Height of frames of the WebCam Video
		iBufferSize		= iWidth*iHeight*iColorChannels;
		fAspectRatio	= (float)iHeight/(float)iWidth;
		bIsVertOriented	= bVOriented;																					// Change landscape webcam frame to portrait in order to better exploit the resolutions
		fWebCamHMD_DiagonalFOVRatio = fDiagonalFOVRatio; 

		pImageBuffer = new ImageBuffer(false, false, Sizei(iWidth, iHeight));
	#if RENDER_OPENGL
		if(bIsBOSupported)																								// Use 2 Pixel Buffer Objects to optimize uploading pipeline: 1 from WebCam to PBO, and 1 from PBO to Texture Object.
		{																												
			glGenBuffers(2, uiBOIDs);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, uiBOIDs[0]);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, iBufferSize, 0, GL_STREAM_DRAW);										// Note: glBufferData with NULL pointer reserves only memory space.
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, uiBOIDs[1]);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, iBufferSize, 0, GL_STREAM_DRAW);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		}
		glBindTexture(GL_TEXTURE_2D, pImageBuffer->TexId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, iWidth, iHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);
	#else
		D3D11_TEXTURE2D_DESC dsDesc;
		dsDesc.Width				= iWidth;
		dsDesc.Height				= iHeight;
		dsDesc.MipLevels			= 1;
		dsDesc.ArraySize			= 1;
		dsDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
		dsDesc.SampleDesc.Count		= 1;
		dsDesc.SampleDesc.Quality	= 0;
		dsDesc.Usage				= (bIsBOSupported ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT);
		dsDesc.CPUAccessFlags		= (bIsBOSupported ? D3D11_CPU_ACCESS_WRITE : 0);
		dsDesc.MiscFlags			= 0;
		dsDesc.BindFlags			= D3D11_BIND_SHADER_RESOURCE;
		WND.Device->CreateTexture2D(&dsDesc, NULL, &pImageBuffer->Tex);
		WND.Device->CreateShaderResourceView(pImageBuffer->Tex, NULL, &pImageBuffer->TexSv);

		D3D11_BLEND_DESC bm;
		memset(&bm, 0, sizeof(bm));
		bm.RenderTarget[0].BlendEnable = true;
		bm.RenderTarget[0].BlendOp     = bm.RenderTarget[0].BlendOpAlpha    = D3D11_BLEND_OP_ADD;
		bm.RenderTarget[0].SrcBlend    = bm.RenderTarget[0].SrcBlendAlpha   = D3D11_BLEND_SRC_ALPHA;
		bm.RenderTarget[0].DestBlend   = bm.RenderTarget[0].DestBlendAlpha  = D3D11_BLEND_INV_SRC_ALPHA;
		bm.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		WND.Device->CreateBlendState(&bm, &BlendState.GetRawRef());
	#endif

		pShaderFill = new ShaderFill(QuadVertDesc, 3, VertexShaderSrc, PixelShaderSrc, pImageBuffer);


		// Simple Quad Model
        pQuadModel = new Model(Vector3f(0,0,0), pShaderFill);
		
		int iNbVertices = sizeof(SimpleQuadVertices)/sizeof(SimpleQuadVertices[0]);
		for(int i=0; i<iNbVertices; i++) { pQuadModel->AddVertex(SimpleQuadVertices[i]); }
		
		int iNbIndices = sizeof(SimpleQuadIndices)/sizeof(SimpleQuadIndices[0]);
		for(int i=0; i<iNbIndices; i++) { pQuadModel->AddIndex(SimpleQuadIndices[i]); }
        
		pQuadModel->AllocateBuffers();
		

		// Fading Edge Quad Model
		pFadingEdgeQuadModel = new Model(Vector3f(0,0,0), pShaderFill);
		
		iNbVertices = sizeof(FadingEdgeQuadVertices)/sizeof(FadingEdgeQuadVertices[0]);
		for(int i=0; i<iNbVertices; i++) { pFadingEdgeQuadModel->AddVertex(FadingEdgeQuadVertices[i]); }

		iNbIndices = sizeof(FadingEdgeQuadIndices)/sizeof(FadingEdgeQuadIndices[0]);
		for(int i=0; i<iNbIndices; i++) { pFadingEdgeQuadModel->AddIndex(FadingEdgeQuadIndices[i]); }
		
		pFadingEdgeQuadModel->AllocateBuffers();


		// Create a new thread to capture the webcam frames
		CaptureFrameThread = OVR::Thread((OVR::Thread::ThreadFn)&WebCamDevice::CaptureFrameThreadFn, this);
		iStatus			   = OVR::Thread::Running;
		CaptureFrameThread.Start(OVR::Thread::Running);
		pMutex = new OVR::Mutex();

		return(1);
	}

	void StopCapture() 
	{
		if(iStatus == OVR::Thread::NotRunning) { return; } 

		iStatus = OVR::Thread::NotRunning;
		CaptureFrameThread.Join();
		Video.release();
	#if !RENDER_OPENGL
		if (pucRGBAData) { OVR_FREE(pucRGBAData); pucRGBAData = NULL; }
	#endif
		if(pMutex) { delete pMutex; }
	}

	void ProcessHand(cv::Mat &InFrame) 
	{
		Hand hand;
		cv::Mat SkinMask(iHeight, iWidth, CV_8UC1);
		hand.BGR2SkinMask(InFrame, SkinMask);
		if (hand.ExtractContourAndHull(SkinMask))
		{
			hand.IdentifyProperties();
			hand.Draw(InFrame);
			DWORD dwGesture = hand.GestureDetection();
			if(WEBCAM_0_VERT_ORIENTATION)
			{
				if(dwGesture & HAND_GESTURE_SWIPE_LEFT) { bLookThrough = true; }										// Switching from VR world to LookingThrough mode
				else if(dwGesture & HAND_GESTURE_SWIPE_RIGHT) { bLookThrough = false; }									// Switching from LookingThrough mode to the VR world
			}
			else
			{
				if(dwGesture & HAND_GESTURE_SWIPE_DOWN) { bLookThrough = true; }										// Switching from VR world to LookingThrough mode
				else if(dwGesture & HAND_GESTURE_SWIPE_UP) { bLookThrough = false; }									// Switching from LookingThrough mode to the VR world
			}
		}
	}

	void SetFrame(const cv::Mat &InFrame) 
	{
		OVR::Mutex::Locker Locker(pMutex);
		Frame = InFrame;
		bHasCapturedFrame = true;
	}

	bool GetFrame(cv::Mat &OutFrame)
	{
		if (!bHasCapturedFrame) { return false; }
		OVR::Mutex::Locker Locker(pMutex);
		OutFrame = Frame;
		bHasCapturedFrame = false;
		return true;
	}

	void Update()
	{
		if (!bHasCapturedFrame || !pImageBuffer) { return; }
		OVR::Mutex::Locker Locker(pMutex);

	#if RENDER_OPENGL
		if(bIsBOSupported)
		{
			iCurrentBOIndex		= (iCurrentBOIndex + 1) % 2;															// iCurrentBOIndex is used to copy pixels from a Pixel Buffer Object to a Texture Object. Increment current index first then get the next index
			int iNextBOIndex	= (iCurrentBOIndex + 1) % 2;															// iNextBOIndex is used to update pixels in a Pixel Buffer Object and it is used for next frame

			// Read from the PBO (current)
			glBindTexture(GL_TEXTURE_2D, pImageBuffer->TexId);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, uiBOIDs[iCurrentBOIndex]);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, iWidth, iHeight, GL_BGR, GL_UNSIGNED_BYTE, 0);						// Copy pixels from Pixel Buffer Object to Texture Object. Use offset instead of pointer.

			// Write into the PBO (next)
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, uiBOIDs[iNextBOIndex]);			
			glBufferData(GL_PIXEL_UNPACK_BUFFER, iBufferSize, 0, GL_STREAM_DRAW);										// Map the Buffer Object into client's memory
			GLubyte *pubBuffer = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);							// Note: glMapBuffer() causes sync issue. If GPU is working with this buffer, glMapBuffer() will wait for GPU to finish its job. To avoid waiting (stall), you can call first glBufferData() with NULL pointer before glMapBuffer().
			if(pubBuffer)																								// If you do that, the previous data in Pixel Buffer Object will be discarded and glMapBuffer() returns a new allocated pointer immediately even if GPU is still working with the previous data.
			{
				memcpy((void *)pubBuffer, (const void*)Frame.data, iBufferSize);										// Update data directly on the mapped buffer
				glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);																	// Release pointer to mapping buffer
			}
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, pImageBuffer->TexId);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, iWidth, iHeight, GL_BGR, GL_UNSIGNED_BYTE, (GLvoid*)Frame.data);
		}
	#else
		if(bIsBOSupported)
		{
			D3D11_MAPPED_SUBRESOURCE map;
			WND.Context->Map(pImageBuffer->Tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);   
			memcpy((void *)map.pData, (const void*)Frame.data, iBufferSize);
			WND.Context->Unmap(pImageBuffer->Tex, 0);
		}
		else { WND.Context->UpdateSubresource(pImageBuffer->Tex, 0, NULL, (const void*)Frame.data, iWidth*iColorChannels, iHeight*iColorChannels); }
	#endif
	}

	void DrawBoard(Matrix4f view, Matrix4f proj, float fXBoardGap=0)
	{
		if(!pQuadModel) { return; }

		Matrix4f modelmat = Matrix4f::Translation(-1.5f+2.5f*fXBoardGap, 2.0f, 3.0f) * 
							Matrix4f::Scaling(1.0f, (bIsVertOriented ? 1.0f/fAspectRatio : fAspectRatio), 1.0f) * 
							Matrix4f::RotationZ(bIsVertOriented ? MATH_FLOAT_PIOVER2 : MATH_FLOAT_PI);
		Matrix4f mat = (view * modelmat).Transposed();

	#if RENDER_OPENGL
		glUseProgram(pQuadModel->Fill->Prog);
		GLint ViewLoc = glGetUniformLocation(pQuadModel->Fill->Prog, "View");
		GLint ProjLoc = glGetUniformLocation(pQuadModel->Fill->Prog, "Proj");
		if (ViewLoc >= 0) glUniformMatrix4fv(ViewLoc, 1, 0, &mat.M[0][0]);
		if (ProjLoc >= 0) glUniformMatrix4fv(ProjLoc, 1, 0, &proj.M[0][0]);
		WND.Render(pQuadModel->Fill, pQuadModel->VertexBuffer, pQuadModel->IndexBuffer, sizeof(Model::Vertex), pQuadModel->numIndices);
	#else
		pQuadModel->Fill->VShader->SetUniform("View",16,(float *) &mat);
		pQuadModel->Fill->VShader->SetUniform("Proj",16,(float *) &proj);
		WND.Context->OMSetBlendState(BlendState, NULL, 0xffffffff);
		WND.Render(pQuadModel->Fill, pQuadModel->VertexBuffer, pQuadModel->IndexBuffer, sizeof(Model::Vertex), pQuadModel->numIndices);
		WND.Context->OMSetBlendState(NULL, NULL, 0xffffffff);
	#endif
	}

	void DrawLookThrough()
	{
		if(!pFadingEdgeQuadModel) { return; }

		Matrix4f mat  = Matrix4f();
		Matrix4f proj = Matrix4f(); // OrthoProjection;
		proj.M[0][0]  = fWebCamHMD_DiagonalFOVRatio;
		proj.M[1][1]  = -fWebCamHMD_DiagonalFOVRatio*(bIsVertOriented ? 1.0f/fAspectRatio : fAspectRatio)*fHMDEyeAspectRatio;
		if(bIsVertOriented) { proj *= Matrix4f::RotationZ(-MATH_FLOAT_PIOVER2); }
	#if RENDER_OPENGL
		glUseProgram(pFadingEdgeQuadModel->Fill->Prog);
		GLint ViewLoc = glGetUniformLocation(pFadingEdgeQuadModel->Fill->Prog, "View");
		GLint ProjLoc = glGetUniformLocation(pFadingEdgeQuadModel->Fill->Prog, "Proj");
		if (ViewLoc >= 0) glUniformMatrix4fv(ViewLoc, 1, 0, &mat.M[0][0]);
		if (ProjLoc >= 0) glUniformMatrix4fv(ProjLoc, 1, 0, &proj.M[0][0]);
		WND.Render(pFadingEdgeQuadModel->Fill, pFadingEdgeQuadModel->VertexBuffer, pFadingEdgeQuadModel->IndexBuffer, 
				   sizeof(Model::Vertex), pFadingEdgeQuadModel->numIndices);
	#else
		pFadingEdgeQuadModel->Fill->VShader->SetUniform("View",16,(float *) &mat);
		pFadingEdgeQuadModel->Fill->VShader->SetUniform("Proj",16,(float *) &proj);
		WND.Context->OMSetBlendState(BlendState, NULL, 0xffffffff);
		WND.Render(pFadingEdgeQuadModel->Fill, pFadingEdgeQuadModel->VertexBuffer, pFadingEdgeQuadModel->IndexBuffer, 
				   sizeof(Model::Vertex), pFadingEdgeQuadModel->numIndices);
		WND.Context->OMSetBlendState(NULL, NULL, 0xffffffff);
	#endif
	}

	static int CaptureFrameThreadFn(Thread *pthread, void* h)
	{
		OVR_UNUSED(pthread);
		WebCamDevice *pDevice = (WebCamDevice *)h;
		cv::Mat TmpBGRFrame;

		while (pDevice->iStatus == OVR::Thread::Running)
		{
			bool bSuccess = pDevice->Video.read(TmpBGRFrame);															// Capture a new frame from WebCam's video
			if (bSuccess) 
			{
				pDevice->ProcessHand(TmpBGRFrame);																		// Analyze the new frame, detect hand and gestures
			#if RENDER_OPENGL
				pDevice->SetFrame(TmpBGRFrame);
			#else
				if(pDevice->pucRGBAData == NULL) { pDevice->pucRGBAData = (unsigned char*)OVR_ALLOC(pDevice->iBufferSize); }
				cv::Mat TmpRGBAFrame(TmpBGRFrame.size(), CV_8UC4, pDevice->pucRGBAData);
				cv::cvtColor(TmpBGRFrame, TmpRGBAFrame, CV_BGR2RGBA, pDevice->iColorChannels);
				pDevice->SetFrame(TmpRGBAFrame);
			#endif
			}
			else { OVR_DEBUG_LOG(("Cannot read a frame from video file.")); }
		}
		return(1);
	}
};

// ==================================================================================//
//  WebCamManager Class
// ==================================================================================//
class WebCamManager
{
public:

	WebCamDevice WebCams[WEBCAM_NB];

public:

	// Constructors
	WebCamManager(ovrHmd HMD)
	{ 
		fHMDEyeAspectRatio	= 0.5f*(float)HMD->Resolution.w/(float)HMD->Resolution.h;
	#if RENDER_OPENGL
		bIsBOSupported = (glGenBuffers && glBindBuffer && glBufferData && glBufferSubData && 
						  glMapBuffer && glUnmapBuffer && glDeleteBuffers && glGetBufferParameteriv);
	#endif
	#if WEBCAM_NB		
		WebCams[0].Initialize(WEBCAM_0_DEVICE_NUMBER, WEBCAM_0_VERT_ORIENTATION, WEBCAM_0_HMD_FOV_RATIO);
	#endif
	#if WEBCAM_NB == 2
		WebCams[1].Initialize(WEBCAM_1_DEVICE_NUMBER, WEBCAM_1_VERT_ORIENTATION, WEBCAM_1_HMD_FOV_RATIO);
	#endif
	}

	void Update()
	{
	#if WEBCAM_NB
		WebCams[0].Update();
	#endif
	#if WEBCAM_NB == 2
		WebCams[1].Update();
	#endif
	}

	void DrawBoard(Matrix4f view, Matrix4f proj)
	{
	#if WEBCAM_NB == 1
		WebCams[0].DrawBoard(view, proj, 1.0f);
		WebCams[0].DrawBoard(view, proj);
	#else if WEBCAM_NB == 2
		WebCams[0].DrawBoard(view, proj, 1.0f);
		WebCams[1].DrawBoard(view, proj);
	#endif
	}

	void DrawLookThrough (int eye) 
	{
	#if WEBCAM_NB == 1
		eye = 0;
	#endif
		WebCams[eye].DrawLookThrough(); 
	}

	void StopCapture() 
	{
	#if WEBCAM_NB
		WebCams[0].StopCapture();
	#endif
	#if WEBCAM_NB == 2
		WebCams[1].StopCapture();
	#endif
	}
};

#endif // WEBCAM_H_