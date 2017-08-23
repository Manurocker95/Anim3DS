@echo off
bannertool.exe makebanner -i banner.png -a audio.wav -o banner.bnr
bannertool.exe makesmdh -s "Anim3DS" -l "Anim3DS" -p "Manurocker95" -i icon.png  -o icon.icn
echo Finished! Banner built!
pause