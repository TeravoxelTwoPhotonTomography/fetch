@echo off
rem Delete VisualC++ intermediate and output files

echo Deleting VisualC++ intermediate and output files ...
for %%f in (aps bak bsc clw dep exp idb ilk lib mac ncb obj opt pch pdb plg res sbr scc) do del /s *.%%f
for %%f in (err wrn) do del /s build.%%f
for %%f in (gid  intermediate.manifest embed.manifest) do del /s *.%%f
for %%f in (vcproj.%USERDOMAIN%.%USERNAME%*) do del /s *.%%f
del /s BuildLog.htm 

pause