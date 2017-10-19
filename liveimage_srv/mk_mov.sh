#/bin/bash
if [ ! -z ${2} ];then
    echo "------------------------------------"
    echo ${1}
    echo ${2}
    echo "------------------------------------"
    fname=$(date)

    pushd ${1}  ##  where what
        echo "ffmpeg -framerate 30 -pattern_type glob -i '*.jpg'  -c:v libx264 -pix_fmt yuv420p \"${fname}.mp4\""
        ffmpeg -framerate 30 -pattern_type glob -i '*.jpg'  -c:v libx264 -pix_fmt yuv420p "${fname}.mp4"
        if [ -f "${fname}.mp4" ];then
            rm ./*.jpg
        fi
    popd
fi
