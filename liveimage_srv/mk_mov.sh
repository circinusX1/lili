#/bin/bash
if [ ! -z ${2} ];then
    echo ${1}
    fname1=$(date)
    fname=$(echo ${fname1} | sed 's/[ :]//g')
    pw=$(pwd)
    cd ${1}  ##  where what
        echo "-----------------------------------------------------------------------------------------------------"
        echo "ffmpeg -framerate 30 -pattern_type glob -i '*.jpg'  -c:v libx264 -pix_fmt yuv420p \"${fname}.mp4\""
        echo "-----------------------------------------------------------------------------------------------------"
        ffmpeg -framerate 30 -pattern_type glob -i '*.jpg'  -c:v libx264 -pix_fmt yuv420p "${fname}.mp4"
        if [ -f "${fname}.mp4" ];then
            rm ./*.jpg
        else
            echo "$(pwd)/${fname}.mp4 not found"
        fi
     cd ${pw}
fi
