# =============================================================================
# Nexor — top-level subdirs project
# =============================================================================
# Three sub-projects:
#   nexor_core   — backend server
#   nexor_flux   — client application
#   nexor_studio — IDE
#
# Build with Qt 5.14.0 (mingw73_64 or matching toolchain).
# =============================================================================

TEMPLATE = subdirs
CONFIG  += ordered

SUBDIRS = \
    nexor_core \
    nexor_flux \
    nexor_studio \
    nexor_command

nexor_core.subdir    = nexor_core
nexor_flux.subdir    = nexor_flux
nexor_studio.subdir  = nexor_studio
nexor_command.subdir = nexor_command
