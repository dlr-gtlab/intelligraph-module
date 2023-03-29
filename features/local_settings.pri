#             ______________      __
#            / ____/_  __/ /___  / /_
#           / / __  / / / / __ `/ __ \
#          / /_/ / / / / / /_/ / /_/ /
#          \____/ /_/ /_/\__,_/_.___/              

# path to dev tools
DEV_TOOLS = ""

#  path to GTlab
GTLAB_DIR = ""

# target major version
MAJOR_VERSION = 

BUILD_MODULE  = true

# First Party
GTLAB_CORE_PATH                 = $${DEV_TOOLS}
GTLAB_LOGGING_PATH              = $${DEV_TOOLS}
GTLAB_NUMERICS_PATH             = $${DEV_TOOLS}
GTLAB_PHYSICS_PATH              = $${DEV_TOOLS}
GENH5_PATH                      = $${DEV_TOOLS}

COMPAT_PATH = $${DEV_TOOLS}/../../tools/CompatibilityLib

# Collections
GTLAB_FUELCOLLECTION_PATH       = $${DEV_TOOLS}
GTLAB_FLUIDCOLLECTION_PATH      = $${DEV_TOOLS}
GTLAB_MAPCOLLECTION_PATH        = $${DEV_TOOLS}
GTLAB_MATERIALCOLLECTION_PATH   = $${DEV_TOOLS}
GTLAB_AIRFOILCOLLECTION_PATH    = $${DEV_TOOLS}

# Modules
GTLAB_POST_MODULE_PATH          = $${DEV_TOOLS}
GTLAB_PERFORMANCE_MODULE_PATH   = $${DEV_TOOLS}
GTLAB_PRE_DESIGN_MODULE_PATH    = $${DEV_TOOLS}
GTLAB_PYTHON_MODULE_PATH        = $${DEV_TOOLS}
GTLAB_RCE_MODULE_PATH           = $${DEV_TOOLS}
GTLAB_BASIC_TOOLS_PATH          = $${DEV_TOOLS}

GTLAB_CAD_KERNEL_PATH           = $${DEV_TOOLS}

# Third Party
QWT_PATH            = $${DEV_TOOLS}/ThirdPartyLibraries/Qwt
MINPACK_PATH        = $${DEV_TOOLS}/ThirdPartyLibraries/minpack
NLOPT_PATH          = $${DEV_TOOLS}/ThirdPartyLibraries/NLopt
HDF5_PATH           = $${DEV_TOOLS}/ThirdPartyLibraries/hdf5
GOOGLE_TEST_PATH    = $${DEV_TOOLS}/ThirdPartyLibraries/GoogleTest

# Deployment
GTLAB_ENVIRONMENT_PATH = "$${GTLAB_DIR}/"
