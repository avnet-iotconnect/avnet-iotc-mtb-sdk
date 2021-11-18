#!/bin/bash


# prevent accidental commits on these files
# need to undo, if changes to these files are actually needed
git update-index --assume-unchanged samples/basic-sample/configs/app_config.h
git update-index --assume-unchanged samples/basic-sample/configs/wifi_config.h
