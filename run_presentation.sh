#!/bin/bash
cd /home/avikc/ppt_controller
source venv/bin/activate

sudo -E DISPLAY=$DISPLAY XAUTHORITY=$XAUTHORITY venv/bin/python presentation_controller.py
