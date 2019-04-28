/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/

 #version 330

layout (location=0) in vec3 inPosition;
layout (location=1) in vec2 inTexcoord;
layout(location = 2) in vec2 inLightmapCoord;

//uniform Matrix model_view_mat;

smooth out vec2 Texcoord;
out vec2 LightmapCoord;
out vec3 WorldPos;

void main()
{
    Texcoord = inTexcoord;
	LightmapCoord = inLightmapCoord;
	WorldPos = mul(model_view_mat, inPosition);
	vec2 screen_pos = inLightmapCoord * 2.0f - 1.0f;
    gl_Position = vec4(screen_pos, 0.0, 1.0);
}

