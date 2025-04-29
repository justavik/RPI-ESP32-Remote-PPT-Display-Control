#!/bin/bash

while [ ! -e /tmp/.X11-unix/X0 ]; do
    sleep 1
done

export DISPLAY=:0
export XAUTHORITY=/home/avikc/.Xauthority

xhost +local:

cd /home/avikc/ppt_controller
source venv/bin/activate
python3 presentation_controller.py