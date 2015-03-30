/************************************************************************************
Filename    :   Win32_RoomTiny_AppRendered.cpp
Content     :   First-person view test application for Oculus Rift
Created     :   October 20th, 2014
Authors     :   Federico Mammano, from the original code written by Tom Heath
Copyright   :   Copyright 2012 Oculus, Inc. All Rights reserved.

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

// Additional structures needed for app-rendered
Scene      * pLatencyTestScene;
DataBuffer * MeshVBs[2] = { NULL, NULL };
DataBuffer * MeshIBs[2] = { NULL, NULL };
ShaderFill * DistortionShaderFill[2];

//-----------------------------------------------------------------------------------
void MakeNewDistortionMeshes(float overrideEyeRelief)
{
    for (int eye=0; eye<2; eye++)
    {
        if (MeshVBs[eye]) delete MeshVBs[eye];
        if (MeshIBs[eye]) delete MeshIBs[eye];

        ovrDistortionMesh meshData;
        ovrHmd_CreateDistortionMeshDebug(HMD, (ovrEyeType)eye, EyeRenderDesc[eye].Fov,
                                         ovrDistortionCap_Chromatic | ovrDistortionCap_TimeWarp,
                                         &meshData, overrideEyeRelief);
        MeshVBs[eye] = new DataBuffer(GL_ARRAY_BUFFER, meshData.pVertexData,
                                      sizeof(ovrDistortionVertex)*meshData.VertexCount);
        MeshIBs[eye] = new DataBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.pIndexData,
                                      sizeof(unsigned short)* meshData.IndexCount);
        ovrHmd_DestroyDistortionMesh(&meshData);
    }
}

//-----------------------------------------------------------------------------------------
void APP_RENDER_SetupGeometryAndShaders(void)
{
    const ShaderFill::VertexAttribDesc VertexDesc[] =
    {   {"Position",	2, GL_FLOAT, false, offsetof(ovrDistortionVertex, ScreenPosNDC)},
        {"inTWLF_V",	2, GL_FLOAT, false, offsetof(ovrDistortionVertex, TimeWarpFactor)},
        {"inTexCoord0", 2, GL_FLOAT, false, offsetof(ovrDistortionVertex, TanEyeAnglesR)},
		{"inTexCoord1", 2, GL_FLOAT, false, offsetof(ovrDistortionVertex, TanEyeAnglesG)},
		{"inTexCoord2", 2, GL_FLOAT, false, offsetof(ovrDistortionVertex, TanEyeAnglesB)}    };

	char* vShader =
		"#version 110																			\n"
		"uniform vec2 EyeToSourceUVScale;														\n"
		"uniform vec2 EyeToSourceUVOffset;														\n"
		"uniform mat4 EyeRotationStart;															\n"
		"uniform mat4 EyeRotationEnd;															\n"
		"attribute vec2 Position;																\n"
		"attribute vec2 inTWLF_V;																\n"		
		"attribute vec2 inTexCoord0;															\n"
		"attribute vec2 inTexCoord1;															\n"
		"attribute vec2 inTexCoord2;															\n"
		"varying vec4 oPosition;																\n"
		"varying vec2 oTexCoord0;																\n"
		"varying vec2 oTexCoord1;																\n"
		"varying vec2 oTexCoord2;																\n"
		"varying float oVignette;																\n"
		"vec2 TexCoord0 = vec2(inTexCoord0.x, -inTexCoord0.y);									\n"
		"vec2 TexCoord1 = vec2(inTexCoord1.x, -inTexCoord1.y);									\n"
		"vec2 TexCoord2 = vec2(inTexCoord2.x, -inTexCoord2.y);									\n"
		"float timewarpLerpFactor = inTWLF_V.x;													\n"
		"float Vignette = inTWLF_V.y;															\n"
		"vec2 TimewarpTexCoord( in vec2 TexCoord, in mat4 rotMat )								\n"
		"{																						\n"
		// Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic 
		// aberration and distortion). These are now "real world" vectors in direction (x,y,1) 
		// relative to the eye of the HMD.	Apply the 3x3 timewarp rotation to these vectors.
		"   vec3 transformed = vec3( ( rotMat * vec4( TexCoord.xy , 1.00000, 1.00000) ).xyz );	\n"
		// Project them back onto the Z=1 plane of the rendered images.
		"   vec2 flattened = (transformed.xy  / transformed.z );								\n"
		// Scale them into ([0,0.5],[0,1]) or ([0.5,0],[0,1]) UV lookup space (depending on eye)
		"   return ((EyeToSourceUVScale * flattened) + EyeToSourceUVOffset);					\n"
		"}																						\n"
		"mat4 mat4_lerp( in mat4 x, in mat4 y, in mat4 s )										\n"
		"{																						\n"
		"	return mat4(mix(x[0],y[0],s[0]), mix(x[1],y[1],s[1]), mix(x[2],y[2],s[2]), mix(x[3],y[3],s[3]));\n"
		"}																						\n"
		"void main()																			\n"
		"{																						\n"
		"   mat4 lerpedEyeRot = mat4_lerp( EyeRotationStart, EyeRotationEnd, mat4( timewarpLerpFactor));\n"
		"   oTexCoord0 = TimewarpTexCoord( TexCoord0, lerpedEyeRot);							\n"
		"   oTexCoord1 = TimewarpTexCoord( TexCoord1, lerpedEyeRot);							\n"
		"   oTexCoord2 = TimewarpTexCoord( TexCoord2, lerpedEyeRot);							\n"
		"   oPosition = vec4( Position.xy , 0.500000, 1.00000);									\n"
		"   oVignette = Vignette;																\n"
		"   gl_Position = oPosition;															\n"
		"}";

	char* pShader = 
		"#version 110																			\n"
		"uniform sampler2D Texture0;															\n"
		"varying vec4 oPosition;																\n"
		"varying vec2 oTexCoord0;																\n"
		"varying vec2 oTexCoord1;																\n"
		"varying vec2 oTexCoord2;																\n"
		"varying float oVignette;																\n"
		"void main()																			\n"
		"{																						\n"
		// 3 samples for fixing chromatic aberrations
		"   float R = texture2D(Texture0, oTexCoord0.xy).r;										\n"
		"   float G = texture2D(Texture0, oTexCoord1.xy).g;										\n"
		"   float B = texture2D(Texture0, oTexCoord2.xy).b;										\n"
		"   gl_FragColor = (oVignette*vec4(R,G,B,1));											\n"
		"}";

    DistortionShaderFill[0]= new ShaderFill(VertexDesc,5,vShader,pShader,pEyeRenderTexture[0],false);
    DistortionShaderFill[1]= new ShaderFill(VertexDesc,5,vShader,pShader,pEyeRenderTexture[1],false);

    // Create eye render descriptions
    for (int eye = 0; eye<2; eye++)
        EyeRenderDesc[eye] = ovrHmd_GetRenderDesc(HMD, (ovrEyeType)eye, HMD->DefaultEyeFov[eye]);

    MakeNewDistortionMeshes();

    // A model for the latency test colour in the corner
    pLatencyTestScene = new Scene();

    ExampleFeatures3(VertexDesc, 5, vShader, pShader);
}

//----------------------------------------------------------------------------------
void APP_RENDER_DistortAndPresent()
{
    bool waitForGPU = true;
    
    // Clear screen
	OGL.ClearAndSetRenderTarget(NULL, Recti(0,0,OGL.WinSize.w,OGL.WinSize.h));

    // Render latency-tester square
    unsigned char latencyColor[3];
    if (ovrHmd_GetLatencyTest2DrawColor(HMD, latencyColor))
    {
        float       col[] = { latencyColor[0] / 255.0f, latencyColor[1] / 255.0f,
                              latencyColor[2] / 255.0f, 1 };
        Matrix4f    view;
        ovrFovPort  fov = { 1, 1, 1, 1 };
        Matrix4f    proj = ovrMatrix4f_Projection(fov, 0.15f, 2, true);

		glUseProgram(pLatencyTestScene->Models[0]->Fill->Prog);
		GLint NewCol = glGetUniformLocation(pLatencyTestScene->Models[0]->Fill->Prog, "NewCol");
		if (NewCol >= 0) glUniform4fv(NewCol, 1, col);
        pLatencyTestScene->Render(view, proj.Transposed());
    }

    // Render distorted eye buffers
    for (int eye=0; eye<2; eye++)  
    {
        ShaderFill * useShaderfill     = DistortionShaderFill[eye];
        ovrPosef   * useEyePose        = &EyeRenderPose[eye];
        float      * useYaw            = &YawAtRender[eye];
        double       debugTimeAdjuster = 0.0;

        ExampleFeatures4(eye,&useShaderfill,&useEyePose,&useYaw,&debugTimeAdjuster,&waitForGPU);

        // Get and set shader constants
        ovrVector2f UVScaleOffset[2];
        ovrHmd_GetRenderScaleAndOffset(EyeRenderDesc[eye].Fov,
                                       pEyeRenderTexture[eye]->Size, EyeRenderViewport[eye], UVScaleOffset);

		glUseProgram(useShaderfill->Prog);
		GLint EyeToSourceUVScale  = glGetUniformLocation(useShaderfill->Prog, "EyeToSourceUVScale");
		GLint EyeToSourceUVOffset = glGetUniformLocation(useShaderfill->Prog, "EyeToSourceUVOffset");
		if (EyeToSourceUVScale >= 0)  glUniform2fv(EyeToSourceUVScale, 1, (float*)&UVScaleOffset[0]);
		if (EyeToSourceUVOffset >= 0) glUniform2fv(EyeToSourceUVOffset, 1, (float*)&UVScaleOffset[1]);

        ovrMatrix4f    timeWarpMatrices[2];
        Quatf extraYawSinceRender = Quatf(Vector3f(0, 1, 0), Yaw - *useYaw);
        ovrHmd_GetEyeTimewarpMatricesDebug(HMD, (ovrEyeType)eye, *useEyePose, timeWarpMatrices, debugTimeAdjuster);

        // Due to be absorbed by a future SDK update
        UtilFoldExtraYawIntoTimewarpMatrix((Matrix4f *)&timeWarpMatrices[0], useEyePose->Orientation, extraYawSinceRender);
        UtilFoldExtraYawIntoTimewarpMatrix((Matrix4f *)&timeWarpMatrices[1], useEyePose->Orientation, extraYawSinceRender);

        timeWarpMatrices[0] = ((Matrix4f)timeWarpMatrices[0]).Transposed();
        timeWarpMatrices[1] = ((Matrix4f)timeWarpMatrices[1]).Transposed();
		GLint EyeRotationStart = glGetUniformLocation(useShaderfill->Prog, "EyeRotationStart");
		GLint EyeRotationEnd   = glGetUniformLocation(useShaderfill->Prog, "EyeRotationEnd");
		if (EyeRotationStart >= 0) glUniformMatrix4fv(EyeRotationStart, 1, 0, &timeWarpMatrices[0].M[0][0]);
		if (EyeRotationEnd >= 0)   glUniformMatrix4fv(EyeRotationEnd, 1, 0, &timeWarpMatrices[1].M[0][0]);

        // Perform distortion
        OGL.Render(useShaderfill, MeshVBs[eye], MeshIBs[eye], sizeof(ovrDistortionVertex), (int)(MeshIBs[eye]->Size/sizeof(unsigned short)));
    }
	
	OGL.Present(true); // Vsync enabled

    // Only flush GPU for ExtendDesktop; not needed in Direct App Rendering with Oculus driver.
    if (HMD->HmdCaps & ovrHmdCap_ExtendDesktop)
    {
		glFlush();
        if (waitForGPU) 
            OGL.WaitUntilGpuIdle();
    }
    OGL.OutputFrameTime(ovr_GetTimeInSeconds());
    ovrHmd_EndFrameTiming(HMD);
}
