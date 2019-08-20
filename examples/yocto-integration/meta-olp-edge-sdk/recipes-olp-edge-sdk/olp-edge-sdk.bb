SUMMARY = "OLP EDGE SDK"
SECTION = "olp-edge-sdk"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${S}/LICENSE;md5=e3fc50a88d0a364313df4b21ef20c29e"

SRC_URI = "gitsm://github.com/heremaps/here-olp-edge-sdk-cpp;branch=master;"

SRCREV = "${AUTOREV}"

DEPENDS += " boost curl openssl "

S = "${WORKDIR}/git/"

inherit cmake

do_install_append() {
          install -d ${D}${libdir}/cmake/Snappy
	  install -d ${D}${libdir}/cmake/leveldb
	  install -m 755 ${WORKDIR}/build/external/install/lib/*.a ${D}${libdir}
	  cp -r ${WORKDIR}/build/external/install/lib/cmake/Snappy ${D}${libdir}/cmake/
	  cp -r ${WORKDIR}/build/external/install/lib/cmake/leveldb ${D}${libdir}/cmake/
}

FILES_${PN} += "${libdir}/libolp-cpp-sdk* ${libdir}/cmake"
