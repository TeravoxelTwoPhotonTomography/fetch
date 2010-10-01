@echo off
rem Delete VisualC++ intermediate and output files

echo Deleting VisualC++ intermediate and output files ...
for %%f in (aps bsc clw exp idb ilk lib mac ncb obj opt pch pdb plg res sbr scc) do del /s *.%%f
for %%f in (err wrn log) do del /s build.%%f
for %%f in (gid manifest bak) do del /s *.%%f
for %%f in (bin) do del /s *.%%f
del /s buildlog.htm
del /s *.vcproj.*.user
del /s mt.dep
