#!/bin/bash -e

##
# Upload any file to HERE Artifactory
#
# Requirements: curl, $ARTIFACTORY_BOT_USER and $ARTIFACTORY_BOT_HTTP_PASSWORD as environmental variable.
# Usage:        artifactory_upload.sh <file_path_on_artifactory> <file_path_on_disk>
#
# Script require parameters:
# param 1 - full URL to file on Artifactory
# param 2 - full or relative path to file on Disk
##
if [[ $1 == "--help" || $1 == "-help" ]]; then
    echo ""
    echo "### Artifactory upload script help ###"
    echo "Usage: artifactory_upload.sh <file_path_on_artifactory> <file_path_on_disk>"
    echo ""
    echo "- <file_path_on_artifactory> is repo path+filename on HERE Artifactory (https://${ARTIFACTORY_HOST}/artifactory) : "
    echo "for example, product/releases/test-reports/job-type/test/0/CHANGELOG.md "
    echo ""
    echo "- <file_path_on_disk> is full or relative path+filename on your local disk : "
    echo "for example, /workspace/CHANGELOG.md or ./CHANGELOG.md "
    echo "Be aware if file exist, it will be overwritten."
    echo "### ... ###"
    echo ""
    exit 0
fi

if [[ ! -z "$ARTIFACTORY_BOT_USER" && ! -z "$ARTIFACTORY_BOT_HTTP_PASSWORD" ]]; then
    if [[ $(which curl &>/dev/null;echo $?) -eq 0 ]]; then
        if ! [[ $1 == " " || $2 == " " || $1 == "" || $2 == "" ]]; then
            curl -u $ARTIFACTORY_BOT_USER:$ARTIFACTORY_BOT_HTTP_PASSWORD -X PUT https://${ARTIFACTORY_HOST}/artifactory/$1 -T $2
            echo "File uploaded to $1"
            exit 0
        else
            echo "Parameter 1 an 2 are not specified, please run: artifactory_upload.sh --help "
            exit 1
        fi
    else
        echo "No curl installed. Visit: https://curl.se/download.html "
        exit 127
    fi
else
    echo "Variables ARTIFACTORY_BOT_USER/ARTIFACTORY_BOT_HTTP_PASSWORD are not defined"
    exit 1
fi
