/*
The MIT License (MIT)

Copyright (c) 2014-2020 Omar Cornut
Modifications Copyright (c) 2020 Panasonic Corporation of North America

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "LogWindow.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_log.h"

#ifdef _WIN32
#include <Windows.h>
#endif

LogWindow LogWindow::instance;

LogWindow* LogWindow::get() { return &instance; }

static const char* priority[] = { "?","V","D","I","W","E","C" };
static const char* category[] = { "APP","ERR","ASRT","SYS","AUD","VID","RNDR","INP","TEST"};

inline const char* getCatStr(int cat)
{
	return cat <= SDL_LOG_CATEGORY_TEST ? category[cat] : "UNK";
}

inline const char* getPri(SDL_LogPriority pri)
{
	return pri <= SDL_LOG_PRIORITY_CRITICAL ? priority[pri] : "U";
}

void logfunc(void*           userdata,
	int             category,
	SDL_LogPriority priority,
	const char*     message)
{
	LogWindow* logwindow = reinterpret_cast<LogWindow*>(userdata);
	logwindow->AddLog("[%s]%s> %s\n", getPri(priority), getCatStr(category), message);
	#ifdef _WIN32
	OutputDebugStringA(message);
	OutputDebugStringA("\n");
	#else
	printf("%s\n",message);
	#endif // _WIN32
}

LogWindow::LogWindow()
{
	SDL_LogSetOutputFunction(logfunc, &instance);
}


LogWindow::~LogWindow()
{
}




void LogWindow::AddLog(const char* fmt, ...)
{
	int old_size = Buf.size();
	va_list args;
	va_start(args, fmt);
	Buf.appendfv(fmt, args);
	va_end(args);
	for (int new_size = Buf.size(); old_size < new_size; old_size++)
		if (Buf[old_size] == '\n')
			LineOffsets.push_back(old_size);
	ScrollToBottom = true;
}

void LogWindow::Draw(const char* title, bool* p_opened)
{
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiSetCond_FirstUseEver);
	if (ImGui::Begin(title, p_opened))
	{
		if (ImGui::Button("Clear")) Clear();
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy");
		ImGui::SameLine();
		Filter.Draw("Filter", -100.0f);
		ImGui::Separator();
		ImGui::BeginChild("scrolling");
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
		if (copy) ImGui::LogToClipboard();

		if (Filter.IsActive())
		{
			const char* buf_begin = Buf.begin();
			const char* line = buf_begin;
			for (int line_no = 0; line != NULL; line_no++)
			{
				const char* line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : NULL;
				if (Filter.PassFilter(line, line_end))
					ImGui::TextUnformatted(line, line_end);
				line = line_end && line_end[1] ? line_end + 1 : NULL;
			}
		}
		else
		{
			ImGui::TextUnformatted(Buf.begin());
		}

		if (ScrollToBottom)
			ImGui::SetScrollHere(1.0f);
		ScrollToBottom = false;
		ImGui::PopStyleVar();
		ImGui::EndChild();
	}
	ImGui::End();
}

void LogWindow::Clear() { Buf.clear(); LineOffsets.clear(); }