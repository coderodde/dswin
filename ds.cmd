@echo off
dswin.exe %*
%USERPROFILE%\ds_command.cmd
del %USERPROFILE%\ds_command.cmd