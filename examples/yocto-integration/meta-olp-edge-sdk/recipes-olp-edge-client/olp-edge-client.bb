SUMMARY = "OLP EDGE Client"
SECTION = "olp-edge-client"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "gitsm://github.com/heremaps/here-olp-edge-sdk-cpp;branch=master"

SRCREV = "${AUTOREV}"

DEPENDS += " olp-edge-sdk "

inherit cmake

S = "${WORKDIR}/git/examples/dataservice-write"

EXTRA_OECMAKE_BUILD = "dataservice-write-example"

do_install() {
         install -d ${D}${bindir}
	 chrpath -d dataservice-write-example
         install -m 0755 dataservice-write-example ${D}${bindir}
}
