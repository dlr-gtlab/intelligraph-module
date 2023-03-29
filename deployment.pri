#             ______________      __
#            / ____/_  __/ /___  / /_
#           / / __  / / / / __ `/ __ \
#          / /_/ / / / / / /_/ / /_/ /
#          \____/ /_/ /_/\__,_/_.___/

######################################################################
#### Version 1.2 (custom copy)
#### DO NOT CHANGE!
######################################################################

# custom copy command
MY_COPY = $$QMAKE_COPY

# QMAKE_COPY may not work reliable on win
win32: {
    MY_COPY = "xcopy /y "
}

## CREATE INCLUDE DIRECTORY
HEADERS_DIR_PATH = $${PWD}/include
mkpath($${HEADERS_DIR_PATH})

META_BASIC_DIR_PATH = $${PWD}/meta
mkpath($${META_BASIC_DIR_PATH})

## FUNCTION DEFINITION FOR COPY FUNCTION
defineTest(copyHeaders) {

    files = $$1
    dir = $${HEADERS_DIR_PATH}/$${TARGET_DIR_NAME}

    meta_dir = $${META_BASIC_DIR_PATH}/$${GT_MODULE_ID}


    win32 {

        dir ~= s,/,\\,g
        meta_dir ~= s,/,\\,g

        QMAKE_POST_LINK += if not exist $$shell_quote($$dir) $$QMAKE_MKDIR $$shell_quote($$dir) $$escape_expand(\\n\\t)
        QMAKE_POST_LINK += if not exist $$shell_quote($$meta_dir) $$QMAKE_MKDIR $$shell_quote($$meta_dir) $$escape_expand(\\n\\t)

        exists(*.h) {
            QMAKE_POST_LINK += $$MY_COPY $$shell_quote(*.h) $$shell_quote($$dir) $$escape_expand(\\n\\t)
        }

        exists(..\\*.md) {
            QMAKE_POST_LINK += $$MY_COPY $$shell_quote(..\\*.md) $$shell_quote($$meta_dir) $$escape_expand(\\n\\t)
        }

        dirNames =

        for(file, files) {

            exists($$file) {

                dirName = $$dirname(file)

                !isEmpty(dirName) {

                    !contains(dirNames, $$dirName) {

                        dirNames += $$dirName
                        sourceDir = $${PWD}/$${dirName}/*.h

                        sourceDir ~= s,/,\\,g

                        exists($${sourceDir}) {

                            QMAKE_POST_LINK += $$MY_COPY $$shell_quote($${sourceDir}) $$shell_quote($$dir) $$escape_expand(\\n\\t)
                        }
                    }
                }
            }
        }

    }
    unix {

        QMAKE_POST_LINK += $$QMAKE_CHK_DIR_EXISTS $$shell_quote($$dir) || $$QMAKE_MKDIR $$shell_quote($$dir) $$escape_expand(\\n\\t)
        QMAKE_POST_LINK += find . -name $$shell_quote(*.h) -exec cp $$shell_quote({}) $$shell_quote($$dir) \; $$escape_expand(\\n\\t)

        QMAKE_POST_LINK += $$QMAKE_CHK_DIR_EXISTS $$shell_quote($$meta_dir) || $$QMAKE_MKDIR $$shell_quote($$meta_dir) $$escape_expand(\\n\\t)
        QMAKE_POST_LINK += find ./../. -name $$shell_quote(*README.md) -exec cp $$shell_quote({}) $$shell_quote($$meta_dir) \; $$escape_expand(\\n\\t)
        QMAKE_POST_LINK += find ./../. -name $$shell_quote(*CHANGELOG.md) -exec cp $$shell_quote({}) $$shell_quote($$meta_dir) \; $$escape_expand(\\n\\t)
    }

    export(QMAKE_POST_LINK)

    return(true)
}

defineTest(copyToEnvironmentPathModules) {

    isEmpty(GTLAB_ENVIRONMENT_PATH) {

        return(false)
    }

    environmentPath = $${GTLAB_ENVIRONMENT_PATH}/modules

    copyToEnvironmentPath($$environmentPath)
}

defineTest(copyToEnvironmentPath) {

    isEmpty(GTLAB_ENVIRONMENT_PATH) {

        return(false)
    }

    isEmpty(DESTDIR) {

        return(false)
    }

    args = $$ARGS

    count(args, 1) {

        environmentPath = $$1
    } else {

        environmentPath = $${GTLAB_ENVIRONMENT_PATH}
    }

    win32:environmentPath ~= s,/,\\,g



    ### for modules this is the target modules folder
    exists($$environmentPath) {

        win32 {

            finalMetaDir = $$environmentPath/meta/$${GT_MODULE_ID}
            metaOriginDir = $${DESTDIR}/../../meta/$${GT_MODULE_ID}/*.*

            dllPath = $${DESTDIR}/$${TARGET}.dll
            dllPath ~= s,/,\\,g

            finalMetaDir ~= s,/,\\,g
            metaOriginDir ~= s,/,\\,g
            QMAKE_POST_LINK += if not exist $$shell_quote($$finalMetaDir) $$QMAKE_MKDIR $$shell_quote($$finalMetaDir) $$escape_expand(\\n\\t)


            QMAKE_POST_LINK += $$MY_COPY $$shell_quote($$dllPath) $$shell_quote($$environmentPath) $$escape_expand(\\n\\t)
            # copy of meta directory
            QMAKE_POST_LINK += $$MY_COPY $$shell_quote($$metaOriginDir) $$shell_quote($$finalMetaDir) $$escape_expand(\\n\\t)
        }
        unix {
            finalMetaDir = $$environmentPath/meta/$${GT_MODULE_ID}
            metaOriginDir = \"$${PWD}/../meta/$${GT_MODULE_ID}\"

            QMAKE_POST_LINK += $$QMAKE_CHK_DIR_EXISTS $$shell_quote($$finalMetaDir) || $$QMAKE_MKDIR $$shell_quote($$finalMetaDir) $$escape_expand(\\n\\t)
            QMAKE_POST_LINK += find $${DESTDIR} -name $$shell_quote(*$${TARGET}.so*) -exec cp $$shell_quote({}) $$shell_quote($$environmentPath) \; $$escape_expand(\\n\\t)
            # copy of meta directory
            QMAKE_POST_LINK += find $$metaOriginDir -name $$shell_quote(*.*) -exec cp $$shell_quote({}) $$shell_quote($$finalMetaDir) \; $$escape_expand(\\n\\t)

        }
        export(QMAKE_POST_LINK)

        return(true)
    } else {

        warning(GTLAB_ENVIRONMENT_PATH ($${environmentPath}) does not exist!)
    }

    return(true)
}
######################################################################
