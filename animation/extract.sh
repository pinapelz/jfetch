rm -r ./images/
mkdir ./images/
ffmpeg -i jorb.gif -vf "crop=152:152:24:24" "./images/frame%02d.png"
