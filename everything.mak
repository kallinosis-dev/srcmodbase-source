# VPC MASTER MAKEFILE


ifneq "$(LINUX_TOOLS_PATH)" ""
TOOL_PATH = $(LINUX_TOOLS_PATH)/
SHELL := $(TOOL_PATH)bash
else
SHELL := /bin/bash
endif

ifdef MAKE_CHROOT
    ifneq ("$(wildcard /etc/schroot/chroot.d/$(MAKE_CHROOT).conf)","")
        export CHROOT_NAME ?= $(MAKE_CHROOT)
    endif
    export CHROOT_NAME ?= $(subst /,_,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
    CHROOT_CONF := /etc/schroot/chroot.d/$(CHROOT_NAME).conf
    ifeq "$(CHROOT_NAME)" "steamrt_scout_amd64"
        CHROOT_DIR := /var/chroots
    else
        CHROOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/tools/runtime/linux)
    endif
    RUNTIME_NAME ?= steamrt_scout_amd64
    ifneq ("$(SCHROOT_CHROOT_NAME)", "$(CHROOT_NAME)")
        SHELL:=schroot --chroot $(CHROOT_NAME) -- /bin/bash
    endif

    CHROOT_TARBALL = $(CHROOT_DIR)/$(RUNTIME_NAME).tar.xz
    CHROOT_TIMESTAMP_FILE = $(CHROOT_DIR)/$(RUNTIME_NAME)/timestamp

    ifneq ("$(wildcard $(CHROOT_TIMESTAMP_FILE))","")
        ifneq ("$(wildcard $(CHROOT_TARBALL))","")
            CHROOT_DEPENDENCY = $(CHROOT_CONF)
        endif
    endif
endif
ECHO = $(TOOL_PATH)echo
ETAGS = $(TOOL_PATH)etags
FIND = $(TOOL_PATH)find
UNAME = $(TOOL_PATH)uname
XARGS = $(TOOL_PATH)xargs

# to control parallelism, set the MAKE_JOBS environment variable
ifeq ($(strip $(MAKE_JOBS)),)
    ifeq ($(shell $(UNAME)),Darwin)
        CPUS := $(shell /usr/sbin/sysctl -n hw.ncpu)
    endif
    ifeq ($(shell $(UNAME)),Linux)
        CPUS := $(shell $(TOOL_PATH)grep processor /proc/cpuinfo | $(TOOL_PATH)wc -l)
    endif
    MAKE_JOBS := $(CPUS)
endif

ifeq ($(strip $(MAKE_JOBS)),)
    MAKE_JOBS := 8
endif

# All projects (default target)
all: $(CHROOT_DEPENDENCY)
	$(MAKE) -f $(lastword $(MAKEFILE_LIST)) -j$(MAKE_JOBS) all-targets

all-targets : appframework bitmap bonesetup bsppack Bspzip Bugreporter_public bugreporter_filequeue Bzip2 choreoobjects Client_CSGO datacache Datamodel Dedicated Dedicated_main Dmserializers Dmxloader engine Fgdlib filesystem_stdio game_controls gcsdk gcsdk_gc hk_physics hk_base hk_math inputsystem interfaces ivp_compactbuilder ivp_physics Jpeglib launcher launcher_main lzma matchmakingbase matchmakingbase_ds Matchmaking_CSGO Matchmaking_DS_CSGO materialsystem mathlib mathlib_extended matsys_controls meshutils Movieobjects vpklib Panel_zoo particles quickhull Raytrace resourcefile responserules_runtime SceneFileCache Server_CSGO ServerBrowser Serverplugin_empty shaderapidx9 shaderapiempty shaderlib soundemittersystem soundsystem_lowlevel stdshader_dbg stdshader_dx9 studiorender tier0 tier1 tier2 togl tier3 Unitlib valve_avi VAudio_Miles vaudio_speex vaudio_celt vgui_controls vgui2 vgui_surfacelib vguimatsurface videocfg vphysics vscript vstdlib vtf localize 


# Individual projects + dependencies

appframework : 
	@$(ECHO) "Building: appframework"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/appframework -f appframework_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

bitmap : 
	@$(ECHO) "Building: bitmap"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/bitmap -f bitmap_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

bonesetup : 
	@$(ECHO) "Building: bonesetup"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/bonesetup -f bonesetup_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

bsppack : interfaces lzma mathlib tier1 tier2 
	@$(ECHO) "Building: bsppack"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/utils/bsppack -f bsppack_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Bspzip : interfaces lzma mathlib tier1 tier2 
	@$(ECHO) "Building: Bspzip"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/utils/bspzip -f bspzip_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Bugreporter_public : interfaces mathlib tier1 
	@$(ECHO) "Building: Bugreporter_public"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/utils/bugreporter_public -f bugreporter_public_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

bugreporter_filequeue : interfaces tier1 
	@$(ECHO) "Building: bugreporter_filequeue"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/utils/bugreporter_filequeue -f bugreporter_filequeue_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Bzip2 : 
	@$(ECHO) "Building: Bzip2"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/utils/bzip2 -f bzip2_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

choreoobjects : 
	@$(ECHO) "Building: choreoobjects"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/choreoobjects -f choreoobjects_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Client_CSGO : bitmap bonesetup Bzip2 choreoobjects Dmxloader interfaces Jpeglib mathlib mathlib_extended matsys_controls meshutils vpklib particles Raytrace resourcefile tier1 tier2 tier3 vgui_controls videocfg vtf 
	@$(ECHO) "Building: Client_CSGO"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/game/client -f client_linux64_csgo.mak $(CLEANPARAM) SHELL=$(SHELL)

datacache : interfaces mathlib tier1 tier2 tier3 
	@$(ECHO) "Building: datacache"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/datacache -f datacache_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Datamodel : 
	@$(ECHO) "Building: Datamodel"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/datamodel -f datamodel_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Dedicated : appframework Dmxloader interfaces mathlib vpklib tier1 tier2 tier3 
	@$(ECHO) "Building: Dedicated"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/dedicated -f dedicated_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Dedicated_main : 
	@$(ECHO) "Building: Dedicated_main"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/dedicated_main -f dedicated_main_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Dmserializers : 
	@$(ECHO) "Building: Dmserializers"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/dmserializers -f dmserializers_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Dmxloader : 
	@$(ECHO) "Building: Dmxloader"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/dmxloader -f dmxloader_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

engine : appframework bitmap Bzip2 Dmxloader interfaces Jpeglib mathlib matsys_controls quickhull soundsystem_lowlevel tier1 tier2 tier3 vgui_controls videocfg vtf 
	@$(ECHO) "Building: engine"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/engine -f engine_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Fgdlib : 
	@$(ECHO) "Building: Fgdlib"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/fgdlib -f fgdlib_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

filesystem_stdio : interfaces vpklib tier1 tier2 
	@$(ECHO) "Building: filesystem_stdio"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/filesystem -f filesystem_stdio_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

game_controls : 
	@$(ECHO) "Building: game_controls"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/vgui2/game_controls -f game_controls_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

gcsdk : 
	@$(ECHO) "Building: gcsdk"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/gcsdk -f gcsdk_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

gcsdk_gc : 
	@$(ECHO) "Building: gcsdk_gc"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/gcsdk -f gcsdk_gc_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

hk_physics : 
	@$(ECHO) "Building: hk_physics"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/ivp/havana/havok/hk_physics -f hk_physics_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

hk_base : 
	@$(ECHO) "Building: hk_base"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/ivp/havana/havok/hk_base -f hk_base_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

hk_math : 
	@$(ECHO) "Building: hk_math"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/ivp/havana/havok/hk_math -f hk_math_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

inputsystem : interfaces mathlib tier1 tier2 
	@$(ECHO) "Building: inputsystem"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/inputsystem -f inputsystem_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

interfaces : 
	@$(ECHO) "Building: interfaces"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/interfaces -f interfaces_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

ivp_compactbuilder : 
	@$(ECHO) "Building: ivp_compactbuilder"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/ivp/ivp_compact_builder -f ivp_compactbuilder_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

ivp_physics : 
	@$(ECHO) "Building: ivp_physics"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/ivp/ivp_physics -f ivp_physics_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Jpeglib : 
	@$(ECHO) "Building: Jpeglib"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/utils/jpeglib -f jpeglib_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

launcher : appframework interfaces tier1 tier2 tier3 
	@$(ECHO) "Building: launcher"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/launcher -f launcher_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

launcher_main : interfaces 
	@$(ECHO) "Building: launcher_main"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/launcher_main -f launcher_main_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

lzma : 
	@$(ECHO) "Building: lzma"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/utils/lzma -f lzma_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

matchmakingbase : gcsdk 
	@$(ECHO) "Building: matchmakingbase"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/matchmaking -f matchmakingbase_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

matchmakingbase_ds : gcsdk 
	@$(ECHO) "Building: matchmakingbase_ds"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/matchmaking -f matchmakingbase_ds_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Matchmaking_CSGO : gcsdk interfaces matchmakingbase mathlib tier1 tier2 tier3 
	@$(ECHO) "Building: Matchmaking_CSGO"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/matchmaking -f matchmaking_linux64_csgo.mak $(CLEANPARAM) SHELL=$(SHELL)

Matchmaking_DS_CSGO : gcsdk interfaces matchmakingbase_ds mathlib tier1 tier2 tier3 
	@$(ECHO) "Building: Matchmaking_DS_CSGO"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/matchmaking -f matchmaking_ds_linux64_csgo.mak $(CLEANPARAM) SHELL=$(SHELL)

materialsystem : bitmap interfaces mathlib shaderlib tier1 tier2 tier3 vtf 
	@$(ECHO) "Building: materialsystem"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/materialsystem -f materialsystem_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

mathlib : 
	@$(ECHO) "Building: mathlib"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/mathlib -f mathlib_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

mathlib_extended : 
	@$(ECHO) "Building: mathlib_extended"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/mathlib -f mathlib_extended_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

matsys_controls : 
	@$(ECHO) "Building: matsys_controls"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/vgui2/matsys_controls -f matsys_controls_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

meshutils : 
	@$(ECHO) "Building: meshutils"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/meshutils -f meshutils_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Movieobjects : 
	@$(ECHO) "Building: Movieobjects"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/movieobjects -f movieobjects_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

vpklib : 
	@$(ECHO) "Building: vpklib"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/vpklib -f vpklib_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Panel_zoo : appframework Dmxloader interfaces mathlib tier1 tier2 tier3 vgui_controls 
	@$(ECHO) "Building: Panel_zoo"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/utils/vgui_panel_zoo -f panel_zoo_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

particles : 
	@$(ECHO) "Building: particles"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/particles -f particles_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

quickhull : 
	@$(ECHO) "Building: quickhull"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/thirdparty/quickhull -f quickhull_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Raytrace : 
	@$(ECHO) "Building: Raytrace"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/raytrace -f raytrace_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

resourcefile : 
	@$(ECHO) "Building: resourcefile"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/resourcefile -f resourcefile_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

responserules_runtime : 
	@$(ECHO) "Building: responserules_runtime"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/responserules/runtime -f responserules_runtime_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

SceneFileCache : interfaces tier1 
	@$(ECHO) "Building: SceneFileCache"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/scenefilecache -f scenefilecache_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Server_CSGO : bitmap bonesetup choreoobjects Dmxloader interfaces mathlib mathlib_extended particles responserules_runtime tier1 tier2 tier3 vgui_controls 
	@$(ECHO) "Building: Server_CSGO"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/game/server -f server_linux64_csgo.mak $(CLEANPARAM) SHELL=$(SHELL)

ServerBrowser : Dmxloader interfaces mathlib tier1 tier2 tier3 vgui_controls 
	@$(ECHO) "Building: ServerBrowser"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/serverbrowser -f serverbrowser_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Serverplugin_empty : interfaces mathlib tier1 tier2 
	@$(ECHO) "Building: Serverplugin_empty"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/utils/serverplugin_sample -f serverplugin_empty_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

shaderapidx9 : bitmap Bzip2 interfaces mathlib tier1 tier2 videocfg vtf 
	@$(ECHO) "Building: shaderapidx9"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/materialsystem/shaderapidx9 -f shaderapidx9_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

shaderapiempty : interfaces tier1 
	@$(ECHO) "Building: shaderapiempty"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/materialsystem/shaderapiempty -f shaderapiempty_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

shaderlib : 
	@$(ECHO) "Building: shaderlib"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/materialsystem/shaderlib -f shaderlib_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

soundemittersystem : interfaces tier1 tier2 
	@$(ECHO) "Building: soundemittersystem"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/soundemittersystem -f soundemittersystem_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

soundsystem_lowlevel : 
	@$(ECHO) "Building: soundsystem_lowlevel"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/soundsystem/lowlevel -f soundsystem_lowlevel_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

stdshader_dbg : interfaces tier1 
	@$(ECHO) "Building: stdshader_dbg"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/materialsystem/stdshaders -f stdshader_dbg_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

stdshader_dx9 : interfaces mathlib shaderlib tier1 
	@$(ECHO) "Building: stdshader_dx9"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/materialsystem/stdshaders -f stdshader_dx9_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

studiorender : bitmap interfaces mathlib tier1 tier2 tier3 
	@$(ECHO) "Building: studiorender"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/studiorender -f studiorender_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

tier0 : 
	@$(ECHO) "Building: tier0"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/tier0 -f tier0_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

tier1 : 
	@$(ECHO) "Building: tier1"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/tier1 -f tier1_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

tier2 : 
	@$(ECHO) "Building: tier2"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/tier2 -f tier2_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

togl : interfaces mathlib tier1 tier2 
	@$(ECHO) "Building: togl"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/togl -f togl_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

tier3 : 
	@$(ECHO) "Building: tier3"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/tier3 -f tier3_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

Unitlib : interfaces tier1 
	@$(ECHO) "Building: Unitlib"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/unitlib -f unitlib_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

valve_avi : interfaces tier1 tier2 tier3 
	@$(ECHO) "Building: valve_avi"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/avi -f valve_avi_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

VAudio_Miles : interfaces tier1 
	@$(ECHO) "Building: VAudio_Miles"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/engine/voice_codecs/miles -f vaudio_miles_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

vaudio_speex : interfaces tier1 
	@$(ECHO) "Building: vaudio_speex"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/engine/voice_codecs/speex -f vaudio_speex_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

vaudio_celt : interfaces tier1 
	@$(ECHO) "Building: vaudio_celt"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/engine/voice_codecs/celt -f vaudio_celt_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

vgui_controls : 
	@$(ECHO) "Building: vgui_controls"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/vgui2/vgui_controls -f vgui_controls_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

vgui2 : interfaces tier1 tier2 tier3 vgui_surfacelib 
	@$(ECHO) "Building: vgui2"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/vgui2/src -f vgui_dll_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

vgui_surfacelib : 
	@$(ECHO) "Building: vgui_surfacelib"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/vgui2/vgui_surfacelib -f vgui_surfacelib_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

vguimatsurface : bitmap Dmxloader interfaces mathlib tier1 tier2 tier3 vgui_controls vgui_surfacelib 
	@$(ECHO) "Building: vguimatsurface"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/vguimatsurface -f vguimatsurface_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

videocfg : 
	@$(ECHO) "Building: videocfg"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/videocfg -f videocfg_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

vphysics : hk_physics hk_base hk_math interfaces ivp_compactbuilder ivp_physics mathlib tier1 tier2 
	@$(ECHO) "Building: vphysics"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/vphysics -f vphysics_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

vscript : interfaces mathlib tier1 
	@$(ECHO) "Building: vscript"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/vscript -f vscript_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

vstdlib : interfaces tier1 
	@$(ECHO) "Building: vstdlib"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/vstdlib -f vstdlib_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

vtf : 
	@$(ECHO) "Building: vtf"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/vtf -f vtf_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

localize : interfaces tier1 tier2 
	@$(ECHO) "Building: localize"
	@+$(MAKE) -C /home/decaloverdose/Projects/srcmodbase-source/localize -f localize_linux64.mak $(CLEANPARAM) SHELL=$(SHELL)

# this is a bit over-inclusive, but the alternative (actually adding each referenced c/cpp/h file to
# the tags file) seems like more work than it's worth.  feel free to fix that up if it bugs you. 
TAGS:
	@$(TOOL_PATH)rm -f TAGS
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.cpp' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append
	@$(FIND)  -name '*.h' -print0 | $(XARGS) -r0 $(ETAGS) --language=c++ --declarations --ignore-indentation --append
	@$(FIND)  -name '*.c' -print0 | $(XARGS) -r0 $(ETAGS) --declarations --ignore-indentation --append



# Mark all the projects as phony or else make will see the directories by the same name and think certain targets 

.PHONY: TAGS all all-targets showtargets regen showregen clean cleantargets cleanandremove relink appframework bitmap bonesetup bsppack Bspzip Bugreporter_public bugreporter_filequeue Bzip2 choreoobjects Client_CSGO datacache Datamodel Dedicated Dedicated_main Dmserializers Dmxloader engine Fgdlib filesystem_stdio game_controls gcsdk gcsdk_gc hk_physics hk_base hk_math inputsystem interfaces ivp_compactbuilder ivp_physics Jpeglib launcher launcher_main lzma matchmakingbase matchmakingbase_ds Matchmaking_CSGO Matchmaking_DS_CSGO materialsystem mathlib mathlib_extended matsys_controls meshutils Movieobjects vpklib Panel_zoo particles quickhull Raytrace resourcefile responserules_runtime SceneFileCache Server_CSGO ServerBrowser Serverplugin_empty shaderapidx9 shaderapiempty shaderlib soundemittersystem soundsystem_lowlevel stdshader_dbg stdshader_dx9 studiorender tier0 tier1 tier2 togl tier3 Unitlib valve_avi VAudio_Miles vaudio_speex vaudio_celt vgui_controls vgui2 vgui_surfacelib vguimatsurface videocfg vphysics vscript vstdlib vtf localize 



# The standard clean command to clean it all out.

clean: 
	@$(MAKE) -f $(lastword $(MAKEFILE_LIST)) -j$(MAKE_JOBS) all-targets CLEANPARAM=clean



# clean targets, so we re-link next time.

cleantargets: 
	@$(MAKE) -f $(lastword $(MAKEFILE_LIST)) -j$(MAKE_JOBS) all-targets CLEANPARAM=cleantargets



# p4 edit and remove targets, so we get an entirely clean build.

cleanandremove: 
	@$(MAKE) -f $(lastword $(MAKEFILE_LIST)) -j$(MAKE_JOBS) all-targets CLEANPARAM=cleanandremove



#relink

relink: cleantargets 
	@$(MAKE) -f $(lastword $(MAKEFILE_LIST)) -j$(MAKE_JOBS) all-targets



# Here's a command to list out all the targets


showtargets: 
	@$(ECHO) '-------------------' && \
	$(ECHO) '----- TARGETS -----' && \
	$(ECHO) '-------------------' && \
	$(ECHO) 'clean' && \
	$(ECHO) 'regen' && \
	$(ECHO) 'showregen' && \
	$(ECHO) 'appframework' && \
	$(ECHO) 'bitmap' && \
	$(ECHO) 'bonesetup' && \
	$(ECHO) 'bsppack' && \
	$(ECHO) 'Bspzip' && \
	$(ECHO) 'Bugreporter_public' && \
	$(ECHO) 'bugreporter_filequeue' && \
	$(ECHO) 'Bzip2' && \
	$(ECHO) 'choreoobjects' && \
	$(ECHO) 'Client_CSGO' && \
	$(ECHO) 'datacache' && \
	$(ECHO) 'Datamodel' && \
	$(ECHO) 'Dedicated' && \
	$(ECHO) 'Dedicated_main' && \
	$(ECHO) 'Dmserializers' && \
	$(ECHO) 'Dmxloader' && \
	$(ECHO) 'engine' && \
	$(ECHO) 'Fgdlib' && \
	$(ECHO) 'filesystem_stdio' && \
	$(ECHO) 'game_controls' && \
	$(ECHO) 'gcsdk' && \
	$(ECHO) 'gcsdk_gc' && \
	$(ECHO) 'hk_physics' && \
	$(ECHO) 'hk_base' && \
	$(ECHO) 'hk_math' && \
	$(ECHO) 'inputsystem' && \
	$(ECHO) 'interfaces' && \
	$(ECHO) 'ivp_compactbuilder' && \
	$(ECHO) 'ivp_physics' && \
	$(ECHO) 'Jpeglib' && \
	$(ECHO) 'launcher' && \
	$(ECHO) 'launcher_main' && \
	$(ECHO) 'lzma' && \
	$(ECHO) 'matchmakingbase' && \
	$(ECHO) 'matchmakingbase_ds' && \
	$(ECHO) 'Matchmaking_CSGO' && \
	$(ECHO) 'Matchmaking_DS_CSGO' && \
	$(ECHO) 'materialsystem' && \
	$(ECHO) 'mathlib' && \
	$(ECHO) 'mathlib_extended' && \
	$(ECHO) 'matsys_controls' && \
	$(ECHO) 'meshutils' && \
	$(ECHO) 'Movieobjects' && \
	$(ECHO) 'vpklib' && \
	$(ECHO) 'Panel_zoo' && \
	$(ECHO) 'particles' && \
	$(ECHO) 'quickhull' && \
	$(ECHO) 'Raytrace' && \
	$(ECHO) 'resourcefile' && \
	$(ECHO) 'responserules_runtime' && \
	$(ECHO) 'SceneFileCache' && \
	$(ECHO) 'Server_CSGO' && \
	$(ECHO) 'ServerBrowser' && \
	$(ECHO) 'Serverplugin_empty' && \
	$(ECHO) 'shaderapidx9' && \
	$(ECHO) 'shaderapiempty' && \
	$(ECHO) 'shaderlib' && \
	$(ECHO) 'soundemittersystem' && \
	$(ECHO) 'soundsystem_lowlevel' && \
	$(ECHO) 'stdshader_dbg' && \
	$(ECHO) 'stdshader_dx9' && \
	$(ECHO) 'studiorender' && \
	$(ECHO) 'tier0' && \
	$(ECHO) 'tier1' && \
	$(ECHO) 'tier2' && \
	$(ECHO) 'togl' && \
	$(ECHO) 'tier3' && \
	$(ECHO) 'Unitlib' && \
	$(ECHO) 'valve_avi' && \
	$(ECHO) 'VAudio_Miles' && \
	$(ECHO) 'vaudio_speex' && \
	$(ECHO) 'vaudio_celt' && \
	$(ECHO) 'vgui_controls' && \
	$(ECHO) 'vgui2' && \
	$(ECHO) 'vgui_surfacelib' && \
	$(ECHO) 'vguimatsurface' && \
	$(ECHO) 'videocfg' && \
	$(ECHO) 'vphysics' && \
	$(ECHO) 'vscript' && \
	$(ECHO) 'vstdlib' && \
	$(ECHO) 'vtf' && \
	$(ECHO) 'localize'



# Here's a command to regenerate this makefile


regen: 
	./devtools/bin/vpc_linux /csgo /f +everything -protobuf -protoc -utf8_range -utf8_validity -abseil -cryptlib /no_scaleform /mksln everything.mak /linux64 


# Here's a command to list out all the targets


showregen: 
	@$(ECHO) ./devtools/bin/vpc_linux /csgo /f +everything -protobuf -protoc -utf8_range -utf8_validity -abseil -cryptlib /no_scaleform /mksln everything.mak /linux64 

ifdef CHROOT_DEPENDENCY
$(CHROOT_DEPENDENCY): $(CHROOT_TIMESTAMP_FILE)
$(CHROOT_TIMESTAMP_FILE): $(CHROOT_TARBALL)
	@echo "chroot ${CHROOT_NAME} at $(CHROOT_DIR) is out of date"
	@echo You need to re-run sudo src/tools/runtime/linux/configure_runtime.sh ${CHROOT_NAME}
endif
