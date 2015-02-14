/************************************************************************************
Filename    :   Win32_OGLAppUtil.h
Content     :   OGL and Application/Window setup functionality for RoomTiny
Created     :   October 20th, 2014
Author      :   Federico Mammano, from the original code written by Tom Heath
Copyright   :   Copyright 2014 Oculus, Inc. All Rights reserved.
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
 
#include "Kernel/OVR_Math.h"
#include <CAPI/GL/CAPI_GLE.h>
#include <CAPI/GL/CAPI_GL_Util.h>
#include <dwmapi.h>
using namespace OVR;

#pragma warning(disable : 4995) // The compiler encountered a function that was marked with pragma deprecated.

//---------------------------------------------------------------------
struct OpenGL
{
    HWND                     Window;
    bool                     Key[256];
    Sizei                    WinSize;

    bool InitWindowAndDevice(HINSTANCE hinst, Recti vp,  bool windowed);
	void ClearAndSetRenderTarget(struct ImageBuffer * imagebuffer, Recti vp);
    void Render(struct ShaderFill* fill, struct DataBuffer* vertices, DataBuffer* indices,UINT stride, int count);

    bool IsAnyKeyPressed() const
    {
        for (unsigned i = 0; i < (sizeof(Key) / sizeof(Key[0])); i++)        
            if (Key[i]) return true;        
        return false;
    }

    void SetMaxFrameLatency(int value) { OVR_UNUSED(value); }

	void Present(bool useVsync)
	{
		BOOL success;
		int swapInterval = (useVsync) ? 1 : 0;
		if (wglGetSwapIntervalEXT() != swapInterval)
			wglSwapIntervalEXT(swapInterval);

		HDC dc = GetDC(Window);
		success = SwapBuffers(dc);
		ReleaseDC(Window, dc);

		OVR_ASSERT(success);
	}

    void WaitUntilGpuIdle()
    {
		glFinish();
/*
		// You should use this code or similar. Pay attention to the required extensions!
		GLuint64 maxTimeout = GLuint64(5000000000);
		GLsync	 syncId = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 ); 
		while(true)
		{ 
			GLenum syncStatus = glClientWaitSync(syncId, GL_SYNC_FLUSH_COMMANDS_BIT, maxTimeout);
			if(syncStatus != GL_TIMEOUT_EXPIRED) break;
		}
		glDeleteSync(syncId);
*/
    }

    void HandleMessages()
    {
        MSG msg;
        if (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {TranslateMessage(&msg);DispatchMessage(&msg);}
    }

    void OutputFrameTime(double currentTime)
    {
        static double lastTime = 0;
        char tempString[100];
        sprintf_s(tempString,"Frame time = %0.2f ms\n",(currentTime-lastTime)*1000.0f);
        OutputDebugStringA(tempString);
        lastTime = currentTime;
    }

	void ReleaseWindow(HINSTANCE hinst)
    {
        DestroyWindow(OGL.Window); UnregisterClassW(L"OVRAppWindow", hinst);
    };

} OGL;

//--------------------------------------------------------------------------
struct Shader 
{
	GLuint GLShader;

    Shader(const char* src, int which_type) : GLShader(0)
    {
		if (!GLShader) GLShader = glCreateShader(which_type == 0 ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
		glShaderSource(GLShader, 1, &src, 0);
		glCompileShader(GLShader);
		GLint r;
		glGetShaderiv(GLShader, GL_COMPILE_STATUS, &r);
		if (!r)
		{
			GLchar msg[1024];
			glGetShaderInfoLog(GLShader, sizeof(msg), 0, msg);
			if (msg[0]) OVR_DEBUG_LOG(("Compiling shader\n%s\nfailed: %s\n", src, msg));

			return;
		}
    }
};
//------------------------------------------------------------
struct ImageBuffer
{
	GLuint						TexId;	
	GLuint						FboId;
	GLuint						RboId;
	Sizei						Size;
	bool						Depth;

    ImageBuffer::ImageBuffer(bool rendertarget, bool depth, Sizei size, int mipLevels = 1,
                             unsigned char * data = NULL) : Size(size), Depth(depth)
    {
		GLint glinternalformat	= depth ? GL_DEPTH_COMPONENT32	: GL_RGBA8;
		GLenum glformat			= depth ? GL_DEPTH_COMPONENT	: GL_RGBA;
		GLenum gltype			= depth ? GL_FLOAT				: GL_UNSIGNED_BYTE;
		
		glGenTextures(1, &TexId);
		glBindTexture(GL_TEXTURE_2D, TexId);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, glinternalformat, size.w, size.h, 0, glformat, gltype, 0);

		if (rendertarget) 
		{
			glGenFramebuffers(1, &FboId);
			glBindFramebuffer(GL_FRAMEBUFFER, FboId);
			glFramebufferTexture2D(GL_FRAMEBUFFER, ( depth ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0), GL_TEXTURE_2D, TexId, 0);

			glGenRenderbuffers(1, &RboId);
			glBindRenderbuffer(GL_RENDERBUFFER, RboId);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size.w, size.h);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RboId);

			if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) OVR_DEBUG_LOG(("Creating frame buffer failed\n"));
		}

        if (data) // Note data is trashed, as is width and height
        {
			for (int level=0; level < mipLevels; level++)
            {		
				glTexImage2D(GL_TEXTURE_2D, level, glinternalformat, size.w, size.h, 0, glformat, gltype, data);
                for(int j = 0; j < (size.h & ~1); j += 2)
                {
                    const uint8_t* psrc = data + (size.w * j * 4);
                    uint8_t*       pdest = data + ((size.w >> 1) * (j >> 1) * 4);
                    for(int i = 0; i < size.w >> 1; i++, psrc += 8, pdest += 4)
                    {
                        pdest[0] = (((int)psrc[0]) + psrc[4] + psrc[size.w * 4 + 0] + psrc[size.w * 4 + 4]) >> 2;
                        pdest[1] = (((int)psrc[1]) + psrc[5] + psrc[size.w * 4 + 1] + psrc[size.w * 4 + 5]) >> 2;
                        pdest[2] = (((int)psrc[2]) + psrc[6] + psrc[size.w * 4 + 2] + psrc[size.w * 4 + 6]) >> 2;
                        pdest[3] = (((int)psrc[3]) + psrc[7] + psrc[size.w * 4 + 3] + psrc[size.w * 4 + 7]) >> 2;
                    }
                }
                size.w >>= 1;  size.h >>= 1;
            }
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipLevels-1);
        }

		glBindTexture(GL_TEXTURE_2D, 0);
		if (rendertarget)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
		}
	}
};
//-----------------------------------------------------
struct ShaderFill
{
    GLuint				Prog;
	Shader            * VShader, *PShader;
    ImageBuffer       * OneTexture;

	struct VertexAttribDesc
	{
		const char*	Name;
		int			Size;
		GLenum		Type;
		bool		Normalized;
		int			Offset;
	};

    const VertexAttribDesc*	VertexDescInfo;
    size_t				numVertexDescInfo;

    ShaderFill::ShaderFill(const VertexAttribDesc * VertexDesc, int numVertexDesc,
                           char* vertexShader, char* pixelShader, ImageBuffer * t, bool wrap=1)
        : numVertexDescInfo(numVertexDesc), OneTexture(t)
    {
		Prog = glCreateProgram();
		VShader = new Shader(vertexShader,0);
		glAttachShader(Prog, VShader->GLShader);
		PShader	= new Shader(pixelShader,1);
		glAttachShader(Prog, PShader->GLShader);
		VertexDescInfo = new VertexAttribDesc[numVertexDesc];
		memcpy((void *)VertexDescInfo, VertexDesc, numVertexDesc*sizeof(VertexAttribDesc));
		for(int i = 0; i < numVertexDesc; i++) glBindAttribLocation(Prog, (GLuint)i, (const GLchar *)VertexDesc[i].Name);

		glLinkProgram(Prog);
		GLint r;
		glGetProgramiv(Prog, GL_LINK_STATUS, &r);
		if (!r)
		{
			GLchar msg[1024];
			glGetProgramInfoLog(Prog, sizeof(msg), 0, msg);
			OVR_DEBUG_LOG(("Linking shaders failed: %s\n", msg));
			if (!r) return;
		}
		glUseProgram(Prog);

		if(t)
		{
			glBindTexture(GL_TEXTURE_2D, t->TexId);
			GLfloat glparam = GLfloat(wrap ? GL_REPEAT : GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glparam);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glparam);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
		}
    }

	void BeginRender(UINT stride)
	{
		if(OneTexture)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, OneTexture->TexId);
			
			glUseProgram(Prog);
			GLint Texture0 = glGetUniformLocation(Prog, "Texture0");
			if (Texture0 >= 0) glUniform1i(Texture0, 0);
		}	
		
		for (size_t i = 0; i < numVertexDescInfo; i++)
		{
			VertexAttribDesc vad = VertexDescInfo[i];
			glEnableVertexAttribArray((GLuint)i);	
			glVertexAttribPointer((GLuint)i, vad.Size, vad.Type, vad.Normalized, stride, reinterpret_cast<char*>(vad.Offset));
		}
	}
	void EndRender() { for (size_t i = 0; i < numVertexDescInfo; i++) glDisableVertexAttribArray((GLuint)i); }
};

//----------------------------------------------------------------
struct DataBuffer 
{
    GLuint				GLBuffer;
	size_t				Size;
	GLenum				Use;

    DataBuffer(GLenum use, const void* buffer, size_t size) : Size(size), Use(use)
    {
		glGenBuffers(1, &GLBuffer);
		glBindBuffer(use, GLBuffer);
		glBufferData(use, size, buffer, GL_DYNAMIC_DRAW);
    }
    void Refresh(const void* buffer, size_t size)
    {
		glBindBuffer(Use, GLBuffer);
		void* v = glMapBuffer(Use, GL_WRITE_ONLY);
		memcpy((void *)v, buffer, size);
		glUnmapBuffer(Use);
    }
};

//---------------------------------------------------------------------------
struct Model 
{
    struct Color
    { 
        unsigned char R,G,B,A;
        
        Color(unsigned char r = 0,unsigned char g=0,unsigned char b=0, unsigned char a = 0xff)
            : R(r), G(g), B(b), A(a) 
        { }
    };
    struct Vertex
    { 
        Vector3f  Pos;
        Color     C;
        float     U, V;
    };

    Vector3f     Pos;
    Quatf        Rot;
    Matrix4f     Mat;
    int          numVertices, numIndices;
    Vertex       Vertices[2000]; //Note fixed maximum
    uint16_t     Indices[2000];
    ShaderFill * Fill;
    DataBuffer * VertexBuffer, * IndexBuffer;  
   
    Model(Vector3f arg_pos, ShaderFill * arg_Fill ) { numVertices=0;numIndices=0;Pos = arg_pos; Fill = arg_Fill; }
    Matrix4f& GetMatrix()                           { Mat = Matrix4f(Rot); Mat = Matrix4f::Translation(Pos) * Mat; return Mat;   }
    void AddVertex(const Vertex& v)                 { Vertices[numVertices++] = v; OVR_ASSERT(numVertices<2000); }
    void AddIndex(uint16_t a)                       { Indices[numIndices++] = a;   OVR_ASSERT(numIndices<2000);  }

    void AllocateBuffers()
    {
		VertexBuffer = new DataBuffer(GL_ARRAY_BUFFER, &Vertices[0], numVertices * sizeof(Vertex));
        IndexBuffer  = new DataBuffer(GL_ELEMENT_ARRAY_BUFFER, &Indices[0], numIndices * 2);
    }

    void Model::AddSolidColorBox(float x1, float y1, float z1, float x2, float y2, float z2, Color c)
    {
        Vector3f Vert[][2] =
        {   Vector3f(x1, y2, z1), Vector3f(z1, x1),  Vector3f(x2, y2, z1), Vector3f(z1, x2),
            Vector3f(x2, y2, z2), Vector3f(z2, x2),  Vector3f(x1, y2, z2), Vector3f(z2, x1),
            Vector3f(x1, y1, z1), Vector3f(z1, x1),  Vector3f(x2, y1, z1), Vector3f(z1, x2),
            Vector3f(x2, y1, z2), Vector3f(z2, x2),  Vector3f(x1, y1, z2), Vector3f(z2, x1),
            Vector3f(x1, y1, z2), Vector3f(z2, y1),  Vector3f(x1, y1, z1), Vector3f(z1, y1),
            Vector3f(x1, y2, z1), Vector3f(z1, y2),  Vector3f(x1, y2, z2), Vector3f(z2, y2),
            Vector3f(x2, y1, z2), Vector3f(z2, y1),  Vector3f(x2, y1, z1), Vector3f(z1, y1),
            Vector3f(x2, y2, z1), Vector3f(z1, y2),  Vector3f(x2, y2, z2), Vector3f(z2, y2),
            Vector3f(x1, y1, z1), Vector3f(x1, y1),  Vector3f(x2, y1, z1), Vector3f(x2, y1),
            Vector3f(x2, y2, z1), Vector3f(x2, y2),  Vector3f(x1, y2, z1), Vector3f(x1, y2),
            Vector3f(x1, y1, z2), Vector3f(x1, y1),  Vector3f(x2, y1, z2), Vector3f(x2, y1),
            Vector3f(x2, y2, z2), Vector3f(x2, y2),  Vector3f(x1, y2, z2), Vector3f(x1, y2), };

        uint16_t CubeIndices[] = {0, 1, 3,     3, 1, 2,     5, 4, 6,     6, 4, 7,
                                  8, 9, 11,    11, 9, 10,   13, 12, 14,  14, 12, 15,
                                  16, 17, 19,  19, 17, 18,  21, 20, 22,  22, 20, 23 };
        
        for(int i = 0; i < 36; i++)
            AddIndex(CubeIndices[i] + (uint16_t) numVertices);

        for(int v = 0; v < 24; v++)
        {
            Vertex vvv; vvv.Pos = Vert[v][0];  vvv.U = Vert[v][1].x; vvv.V = Vert[v][1].y;
            float dist1 = (vvv.Pos - Vector3f(-2,4,-2)).Length();
            float dist2 = (vvv.Pos - Vector3f(3,4,-3)).Length();
            float dist3 = (vvv.Pos - Vector3f(-4,3,25)).Length();
            int   bri   = rand() % 160;
            float RRR   = c.R * (bri + 192.0f*(0.65f + 8/dist1 + 1/dist2 + 4/dist3)) / 255.0f;
            float GGG   = c.G * (bri + 192.0f*(0.65f + 8/dist1 + 1/dist2 + 4/dist3)) / 255.0f;
            float BBB   = c.B * (bri + 192.0f*(0.65f + 8/dist1 + 1/dist2 + 4/dist3)) / 255.0f;
            vvv.C.R = RRR > 255 ? 255: (unsigned char) RRR;
            vvv.C.G = GGG > 255 ? 255: (unsigned char) GGG;
            vvv.C.B = BBB > 255 ? 255: (unsigned char) BBB;
            AddVertex(vvv);
        }
    }
};
//------------------------------------------------------------------------- 
struct Scene  
{
    int     num_models;
    Model * Models[10];

    void    Add(Model * n)
    { Models[num_models++] = n; }    

    Scene(int reducedVersion) : num_models(0) // Main world
    {
        const ShaderFill::VertexAttribDesc ModelVertexDesc[] =
        {   {"Position", 3, GL_FLOAT,		  false, offsetof(Model::Vertex, Pos)},
            {"Color",    4, GL_UNSIGNED_BYTE, true,  offsetof(Model::Vertex, C)},
            {"TexCoord", 2, GL_FLOAT,		  false, offsetof(Model::Vertex, U)}    };

		char* VertexShaderSrc =
			"#version 110\n"
			"uniform mat4 Proj;\n"
			"uniform mat4 View;\n"
			"attribute vec4 Position;\n"
			"attribute vec4 Color;\n"
			"attribute vec2 TexCoord;\n"
			"varying vec4 oColor;\n"
			"varying vec2 oTexCoord;\n"
			"void main()\n"
			"{\n" 
			"	gl_Position = Proj * (View * Position);\n" 
			"	oTexCoord = TexCoord;\n"
			"	oColor = Color;\n"
			"}";
        char* PixelShaderSrc =
			"#version 110\n"
			"uniform sampler2D Texture0;\n"
			"varying vec4 oColor;\n"
			"varying vec2 oTexCoord;\n"
			"void main()\n"
			"{\n"
			"   gl_FragColor = oColor * texture2D(Texture0, oTexCoord);\n"
			"}\n";

        // Construct textures
        static Model::Color tex_pixels[4][256*256];
        ShaderFill * generated_texture[4];

        for (int k=0;k<4;k++)
        {
            for (int j = 0; j < 256; j++)
            for (int i = 0; i < 256; i++)
            {
                if (k==0) tex_pixels[0][j*256+i] = (((i >> 7) ^ (j >> 7)) & 1) ? Model::Color(180,180,180,255) : Model::Color(80,80,80,255);// floor
                if (k==1) tex_pixels[1][j*256+i] = (((j/4 & 15) == 0) || (((i/4 & 15) == 0) && ((((i/4 & 31) == 0) ^ ((j/4 >> 4) & 1)) == 0))) ?
                                                   Model::Color(60,60,60,255) : Model::Color(180,180,180,255); //wall
                if (k==2) tex_pixels[2][j*256+i] = (i/4 == 0 || j/4 == 0) ? Model::Color(80,80,80,255) : Model::Color(180,180,180,255);// ceiling
                if (k==3) tex_pixels[3][j*256+i] = Model::Color(128,128,128,255);// blank
            }
            ImageBuffer * t      = new ImageBuffer(false,false,Sizei(256,256),8,(unsigned char *)tex_pixels[k]);
			generated_texture[k] = new ShaderFill(ModelVertexDesc,3,VertexShaderSrc,PixelShaderSrc,t);
        }
        // Construct geometry
        Model * m = new Model(Vector3f(0,0,0),generated_texture[2]);  // Moving box
        m->AddSolidColorBox( 0, 0, 0,  +1.0f,  +1.0f, 1.0f,  Model::Color(64,64,64)); 
        m->AllocateBuffers(); Add(m);

        m = new Model(Vector3f(0,0,0),generated_texture[1]);  // Walls
        m->AddSolidColorBox( -10.1f,   0.0f,  -20.0f, -10.0f,  4.0f,  20.0f, Model::Color(128,128,128)); // Left Wall
        m->AddSolidColorBox( -10.0f,  -0.1f,  -20.1f,  10.0f,  4.0f, -20.0f, Model::Color(128,128,128)); // Back Wall
        m->AddSolidColorBox(  10.0f,  -0.1f,  -20.0f,  10.1f,  4.0f,  20.0f, Model::Color(128,128,128));  // Right Wall
        m->AllocateBuffers(); Add(m);
 
        m = new Model(Vector3f(0,0,0),generated_texture[0]);  // Floors
        m->AddSolidColorBox( -10.0f,  -0.1f,  -20.0f,  10.0f,  0.0f, 20.1f,  Model::Color(128,128,128)); // Main floor
        m->AddSolidColorBox( -15.0f,  -6.1f,   18.0f,  15.0f, -6.0f, 30.0f,  Model::Color(128,128,128) );// Bottom floor
        m->AllocateBuffers(); Add(m);

        if (reducedVersion) return;

        m = new Model(Vector3f(0,0,0),generated_texture[2]);  // Ceiling
        m->AddSolidColorBox( -10.0f,  4.0f,  -20.0f,  10.0f,  4.1f, 20.1f,  Model::Color(128,128,128)); 
        m->AllocateBuffers(); Add(m);
 
        m = new Model(Vector3f(0,0,0),generated_texture[3]);  // Fixtures & furniture
        m->AddSolidColorBox(   9.5f,   0.75f,  3.0f,  10.1f,  2.5f,   3.1f,  Model::Color(96,96,96) );   // Right side shelf// Verticals
        m->AddSolidColorBox(   9.5f,   0.95f,  3.7f,  10.1f,  2.75f,  3.8f,  Model::Color(96,96,96) );   // Right side shelf
        m->AddSolidColorBox(   9.55f,  1.20f,  2.5f,  10.1f,  1.30f,  3.75f,  Model::Color(96,96,96) ); // Right side shelf// Horizontals
        m->AddSolidColorBox(   9.55f,  2.00f,  3.05f,  10.1f,  2.10f,  4.2f,  Model::Color(96,96,96) ); // Right side shelf
        m->AddSolidColorBox(   5.0f,   1.1f,   20.0f,  10.0f,  1.2f,  20.1f, Model::Color(96,96,96) );   // Right railing   
        m->AddSolidColorBox(  -10.0f,  1.1f, 20.0f,   -5.0f,   1.2f, 20.1f, Model::Color(96,96,96) );   // Left railing  
        for (float f=5.0f;f<=9.0f;f+=1.0f)
        {
            m->AddSolidColorBox(   f,   0.0f,   20.0f,   f+0.1f,  1.1f,  20.1f, Model::Color(128,128,128) );// Left Bars
            m->AddSolidColorBox(  -f,   1.1f,   20.0f,  -f-0.1f,  0.0f,  20.1f, Model::Color(128,128,128) );// Right Bars
        }
        m->AddSolidColorBox( -1.8f, 0.8f, 1.0f,   0.0f,  0.7f,  0.0f,   Model::Color(128,128,0)); // Table
        m->AddSolidColorBox( -1.8f, 0.0f, 0.0f,  -1.7f,  0.7f,  0.1f,   Model::Color(128,128,0)); // Table Leg 
        m->AddSolidColorBox( -1.8f, 0.7f, 1.0f,  -1.7f,  0.0f,  0.9f,   Model::Color(128,128,0)); // Table Leg 
        m->AddSolidColorBox(  0.0f, 0.0f, 1.0f,  -0.1f,  0.7f,  0.9f,   Model::Color(128,128,0)); // Table Leg 
        m->AddSolidColorBox(  0.0f, 0.7f, 0.0f,  -0.1f,  0.0f,  0.1f,   Model::Color(128,128,0)); // Table Leg 
        m->AddSolidColorBox( -1.4f, 0.5f, -1.1f, -0.8f,  0.55f, -0.5f,  Model::Color(44,44,128) ); // Chair Set
        m->AddSolidColorBox( -1.4f, 0.0f, -1.1f, -1.34f, 1.0f,  -1.04f, Model::Color(44,44,128) ); // Chair Leg 1
        m->AddSolidColorBox( -1.4f, 0.5f, -0.5f, -1.34f, 0.0f,  -0.56f, Model::Color(44,44,128) ); // Chair Leg 2
        m->AddSolidColorBox( -0.8f, 0.0f, -0.5f, -0.86f, 0.5f,  -0.56f, Model::Color(44,44,128) ); // Chair Leg 2
        m->AddSolidColorBox( -0.8f, 1.0f, -1.1f, -0.86f, 0.0f,  -1.04f, Model::Color(44,44,128) ); // Chair Leg 2
        m->AddSolidColorBox( -1.4f, 0.97f,-1.05f,-0.8f,  0.92f, -1.10f, Model::Color(44,44,128) ); // Chair Back high bar

        for (float f=3.0f;f<=6.6f;f+=0.4f)
            m->AddSolidColorBox( -3,  0.0f, f,   -2.9f, 1.3f, f+0.1f, Model::Color(64,64,64) );//Posts
        
        m->AllocateBuffers(); Add(m);
     }

    // Simple latency box (keep similar vertex format and shader params same, for ease of code)
    Scene() : num_models(0) 
    {
        const ShaderFill::VertexAttribDesc ModelVertexDesc[] =
        { {"Position", 3, GL_FLOAT,		  false, offsetof(Model::Vertex, Pos)} };

		char* VertexShaderSrc =
			"#version 110\n"
			"uniform mat4 Proj;\n"
			"uniform mat4 View;\n"
			"uniform vec4 NewCol;\n"
			"attribute vec4 Position;\n"
			"varying vec4 oColor;\n"
			"void main()\n"
			"{\n" 
			"	gl_Position = (Proj * Position);\n" 
			"	oColor = NewCol;\n"
			"}";
        char* PixelShaderSrc =
			"#version 110\n"
			"varying vec4 oColor;\n"
			"void main()\n"
			"{\n"
			"   gl_FragColor = oColor;\n"
			"}\n";

        Model* m = new Model(Vector3f(0,0,0),new ShaderFill(ModelVertexDesc,1,VertexShaderSrc,PixelShaderSrc,0));  
        float scale = 0.04f;  float extra_y = ((float)OGL.WinSize.w/(float)OGL.WinSize.h);
        m->AddSolidColorBox( 1-scale,  1-(scale*extra_y), -1, 1+scale,  1+(scale*extra_y), -1,  Model::Color(0,128,0)); 
        m->AllocateBuffers(); Add(m);
     }
 
    void Render(Matrix4f view, Matrix4f proj)
    {
        for(int i = 0; i < num_models; i++)
        {
            Matrix4f modelmat	= Models[i]->GetMatrix();
            Matrix4f mat		= (view * modelmat).Transposed();

			glUseProgram(Models[i]->Fill->Prog);
			GLint ProjLoc = glGetUniformLocation(Models[i]->Fill->Prog, "Proj");
			GLint ViewLoc = glGetUniformLocation(Models[i]->Fill->Prog, "View");
			if (ProjLoc >= 0) glUniformMatrix4fv(ProjLoc, 1, 0, &proj.M[0][0]);
			if (ViewLoc >= 0) glUniformMatrix4fv(ViewLoc, 1, 0, &mat.M[0][0]);

            OGL.Render(Models[i]->Fill, Models[i]->VertexBuffer,  Models[i]->IndexBuffer,
                        sizeof(Model::Vertex), Models[i]->numIndices);
        }
    }
};

//----------------------------------------------------------------------------------------------------------
void OpenGL::ClearAndSetRenderTarget(ImageBuffer * imagebuffer, Recti vp)
{
	if(imagebuffer) 
	{
		glBindFramebuffer(GL_FRAMEBUFFER, imagebuffer->FboId);
		glFramebufferTexture2D(GL_FRAMEBUFFER, ( imagebuffer->Depth ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0), GL_TEXTURE_2D, imagebuffer->TexId, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, imagebuffer->RboId);
	}
	else { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glViewport(vp.x, vp.y, vp.w, vp.h);
	glDepthRange(0, 1);
}

//---------------------------------------------------------------
LRESULT CALLBACK SystemWindowProc(HWND arg_hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case(WM_NCCREATE):  OGL.Window = arg_hwnd;                      break;
        case WM_KEYDOWN:    OGL.Key[(unsigned)wp] = true;               break;
        case WM_KEYUP:      OGL.Key[(unsigned)wp] = false;              break;
        case WM_SETFOCUS:   SetCapture(OGL.Window); ShowCursor(FALSE);  break;
        case WM_KILLFOCUS:  ReleaseCapture(); ShowCursor(TRUE);         break;
     }
    return DefWindowProc(OGL.Window, msg, wp, lp);
}

//-----------------------------------------------------------------------
bool OpenGL::InitWindowAndDevice(HINSTANCE hinst, Recti vp, bool windowed)
{
    WNDCLASSW wc; memset(&wc, 0, sizeof(wc));
    wc.lpszClassName = L"OVRAppWindow";
    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = SystemWindowProc;
    wc.cbWndExtra    = NULL;
    RegisterClassW(&wc);
     
    DWORD wsStyle     = WS_POPUP;
    DWORD sizeDivisor = 1;

    if (windowed)
    {
        wsStyle |= WS_OVERLAPPEDWINDOW; sizeDivisor = 2;
    }

    RECT winSize = { 0, 0, vp.w / sizeDivisor, vp.h / sizeDivisor};
    AdjustWindowRect(&winSize, wsStyle, false);
    Window = CreateWindowW(L"OVRAppWindow", L"OculusRoomTiny",wsStyle |WS_VISIBLE,
                         vp.x, vp.y, winSize.right-winSize.left, winSize.bottom-winSize.top,
                         NULL, NULL, hinst, NULL);

    if (!Window)
        return(false);
    if (windowed)
        WinSize = vp.GetSize();
    else
    {
        RECT rc; GetClientRect(Window, &rc);
        WinSize = Sizei(rc.right-rc.left,rc.bottom-rc.top);
    }

	HDC dc = GetDC(Window);

    bool dwmCompositionEnabled = false;

    #if defined(OVR_BUILD_DEBUG) && !defined(OVR_OS_CONSOLE) // Console platforms have no getenv function.
        OVR_DISABLE_MSVC_WARNING(4996) // "This function or variable may be unsafe..."
        const char* value = getenv("Oculus_LibOVR_DwmCompositionEnabled"); // Composition is temporarily configurable via an environment variable for the purposes of testing.
        if(value && (strcmp(value, "1") == 0))
            dwmCompositionEnabled = true;
        OVR_RESTORE_MSVC_WARNING()
    #endif

    if (!dwmCompositionEnabled)
    {
        // To consider: Make a generic helper macro/class for managing the loading of libraries and functions.
        typedef HRESULT (WINAPI *PFNDWMENABLECOMPOSITIONPROC) (UINT);
        PFNDWMENABLECOMPOSITIONPROC DwmEnableComposition;

	    HINSTANCE HInstDwmapi = LoadLibraryW( L"dwmapi.dll" );
	    OVR_ASSERT(HInstDwmapi);

        if (HInstDwmapi)
        {
            DwmEnableComposition = (PFNDWMENABLECOMPOSITIONPROC)GetProcAddress( HInstDwmapi, "DwmEnableComposition" );
            OVR_ASSERT(DwmEnableComposition);

            if (DwmEnableComposition)
            {
                DwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
            }

            FreeLibrary(HInstDwmapi);
	        HInstDwmapi = NULL;
        }
    }

    {
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(pfd));

		pfd.nSize       = sizeof(pfd);
		pfd.nVersion    = 1;
		pfd.iPixelType  = PFD_TYPE_RGBA;
		pfd.dwFlags     = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
		pfd.cColorBits  = 32;
		pfd.cDepthBits  = 16;

		int pf = ChoosePixelFormat(dc, &pfd);
		if (!pf)
		{
			ReleaseDC(Window, dc);
			return NULL;
		}
		
		if (!SetPixelFormat(dc, pf, &pfd))
		{
			ReleaseDC(Window, dc);
			return NULL;
		}

		HGLRC context = wglCreateContext(dc);
		if (!wglMakeCurrent(dc, context))
		{
			wglDeleteContext(context);
			ReleaseDC(Window, dc);
			return NULL;
		}
		
		wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
		wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

		wglDeleteContext(context);
    }

	int iAttributes[] = {
		//WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 16,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
        0, 0};

    float fAttributes[] = {0,0};

	int pf = 0;
	UINT numFormats = 0;

	if (!wglChoosePixelFormatARB(dc, iAttributes, fAttributes, 1, &pf, &numFormats))
    {
        ReleaseDC(Window, dc);
        return NULL;
    }

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));

    if (!SetPixelFormat(dc, pf, &pfd))
    {
        ReleaseDC(Window, dc);
        return NULL;
    }

	GLint attribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 2,
		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	HGLRC context = wglCreateContextAttribsARB(dc, 0, attribs);
	if (!wglMakeCurrent(dc, context))
	{
		wglDeleteContext(context);
		ReleaseDC(Window, dc);
		return NULL;
	}

    // Get the GLEContext instance from libOVR. We use that shared version instead of declaring our own.
    OVR::GLEContext* pGLEContext = OVR::GetGLEContext();
    OVR_ASSERT(pGLEContext);
    if(!pGLEContext->IsInitialized())
    {
        OVR::GLEContext::SetCurrentContext(pGLEContext);
        pGLEContext->Init();
    }

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return(true);
}

//---------------------------------------------------------------------------------------------
void OpenGL::Render(ShaderFill* fill, DataBuffer* vertices, DataBuffer* indices,UINT stride, int count)
{
	glBindBuffer(GL_ARRAY_BUFFER, vertices->GLBuffer);
	fill->BeginRender(stride);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices->GLBuffer);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, NULL);
	fill->EndRender();
}

//--------------------------------------------------------------------------------
// Due to be removed once the functionality is in the SDK
void UtilFoldExtraYawIntoTimewarpMatrix(Matrix4f * timewarpMatrix, Quatf eyePose, Quatf extraQuat)
{
       timewarpMatrix->M[0][1] = -timewarpMatrix->M[0][1];
       timewarpMatrix->M[0][2] = -timewarpMatrix->M[0][2];
       timewarpMatrix->M[1][0] = -timewarpMatrix->M[1][0];
       timewarpMatrix->M[2][0] = -timewarpMatrix->M[2][0];
       Quatf newtimewarpStartQuat = eyePose * extraQuat * (eyePose.Inverted())*(Quatf(*timewarpMatrix));
       *timewarpMatrix = Matrix4f(newtimewarpStartQuat);
       timewarpMatrix->M[0][1] = -timewarpMatrix->M[0][1];
       timewarpMatrix->M[0][2] = -timewarpMatrix->M[0][2];
       timewarpMatrix->M[1][0] = -timewarpMatrix->M[1][0];
       timewarpMatrix->M[2][0] = -timewarpMatrix->M[2][0];
}
