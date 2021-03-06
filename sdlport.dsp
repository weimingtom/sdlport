# Microsoft Developer Studio Project File - Name="sdlport" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=sdlport - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sdlport.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sdlport.mak" CFG="sdlport - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sdlport - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "sdlport - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sdlport - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "./sdl" /I "./include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 winmm.lib pthreadVC2.lib ws2_32.lib freetype235MT.lib /nologo /subsystem:console /machine:I386 /libpath:"./lib"

!ELSEIF  "$(CFG)" == "sdlport - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "./sdl" /I "./include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib pthreadVC2.lib ws2_32.lib freetype235MT_D.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"./lib"

!ENDIF 

# Begin Target

# Name "sdlport - Win32 Release"
# Name "sdlport - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=.\test.c
# End Source File
# Begin Source File

SOURCE=.\TextureLoader.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\SDL_mixer.h
# End Source File
# Begin Source File

SOURCE=.\test.h
# End Source File
# Begin Source File

SOURCE=.\TextureLoader.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "sdl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sdl\COPYING
# End Source File
# Begin Source File

SOURCE=.\sdl\CREDITS
# End Source File
# Begin Source File

SOURCE=.\sdl\README
# End Source File
# Begin Source File

SOURCE=".\sdl\README-SDL.txt"
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_audio.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_audio.h
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_blit_0.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_blit_1.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_blit_A.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_blit_N.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_effect_position.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_effect_stereoreverse.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_effects_internal.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_endian.h
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_events.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_events.h
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_ext_pixel.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_ext_pixel.h
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_image.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_image.h
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_mixer.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_mixer.h
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_music.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_surface.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_timer.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_timer.h
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_ttf.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_ttf.h
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_video.h
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_wavestream.c
# End Source File
# Begin Source File

SOURCE=.\sdl\SDL_yuv.c
# End Source File
# Begin Source File

SOURCE=.\sdl\version.txt
# End Source File
# End Group
# End Target
# End Project
