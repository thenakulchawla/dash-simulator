#!/bin/bash

# Configure the following values
RAPIDJSON_FOLDER=~/rapidjson
NS3_FOLDER=~/workspaceDash/ns-allinone-3.25/ns-3.25

# Do not change
mkdir $NS3_FOLDER/rapidjson
cp  -r $RAPIDJSON_FOLDER/include/rapidjson/* $NS3_FOLDER/rapidjson/
cp  src/applications/model/* $NS3_FOLDER/src/applications/model/
cp  src/applications/helper/* $NS3_FOLDER/src/applications/helper/
cp  src/internet/helper/* $NS3_FOLDER/src/internet/helper/
cp  scratch/* $NS3_FOLDER/scratch/
yes | cp -rf wscript ~NS3_FOLDER/
yes | cp -rf src/applications/wscript ~NS3_FOLDER/src/applications/
cp .gitignore $NS3_FOLDER/
