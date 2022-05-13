# OpenCP Module Player
# copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
#
# OpenCP/DOS Makefile for Watcom C++ 11.0 with DOS/4GW
#
# revision history: (please note changes here)
#  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
#    -first release
#  -kb980717   Tammo Hinrichs <opencp@gmx.net>
#    -various fixes, additions etc. to make this not only work (pascal's
#     version quite did not), but even make convenient use possible
#    -of course, changes over changes during the development
#  -fd981014   Felix Domke <tmbinc@gmx.net>
#    -changed the methode of creating the .LIBs (now it uses a utility)
#    -changed some other things
#    -win32-port (yezza :)
#  -doj981211  Dirk Jagdmann <doj@cubic.org>
#    -CVS temp files are now removed my "wmake distclean"
#    -daposer.gif is copied into the BIN\ directory
#  -doj982020  Dirk Jagdmann <doj@cubic.org>
#    -added removel of "cparcs.dat" and "cpmodnfo.dat" to distclean:
#  -fd990515   Felix Domke <tmbinc@gmx.net>
#    -corrected /dDEBUG, added DEFINES and WASM-support (.was)
#  -doj20010723 Dirk Jagdmann <doj@cubic.org>
#    -messed around much
#    -removed watcom10.6 targets

# Note... if you wonder about the strange version numbers:
# Multiply them by 100, convert them to hex (0xXXYYZZ) and
# you get Version XX.YY.ZZ :)

cp_ver  = 1556.86

#
#  defines:  CPDOS      - Dos32-Version, WATCOM, player (and ims?)
#            CPWIN      - WinIMS Hack ... DO NOT USE!
#            DOS32      - Target is Dos32 WATCOM
#            WIN32      - Target is Win32 WATCOM

# do not change them according to the DOS32/WIN32 etc., this is done
# automatically.
defines = /dFDCTDEXT /dFDCTBEXT /dFASTBITS
#             ^^         ^^     for the ampg-asm-extensions

# MAKE SURE THAT ALL THESE SETTINGS ARE CORRECT

buildversion=RELEASE		# either 'DEBUG' or 'RELEASE'
buildtarget=DOS32		# either 'DOS32' or 'WIN32'
build=v2.6.0pre6
defaultlibrarysfordlls=NO
author=Dirk\040Jagdmann\040<doj@cubic.org>
warnlevel=3
optsw=/oabklmnrt


srcdirs=.;btools;playsid;playit;playxm;playmp;playgmd;playgmi;devs;devp;devw;boot;playinp;playwav;playcda;stuff;dev;filesel;cpiface;recmp;dosdll;pack;binfile;ims;plugins;winims;help;wma;vxdapc

!ifeq defaultlibrarysfordlls NO
defaultlibs={ clib3r.lib plib3r.lib math387r.lib }
!endif

runtime=inirt11.obj

!ifeq buildtarget DOS32
!ifeq defaultlibrarysfordlls NO
copt = /w$(warnlevel) $(optsw) /zp1 /5r /s /dCPDOS /dDOS32 /fp5 /fpi87 /i=$(srcdirs) /dVERSION="$(build)" /zl /bt=DOS $(defines)
!else
copt = /w$(warnlevel) $(optsw) /zp1 /5r /s /dCPDOS /dDOS32 /fp5 /fpi87 /i=$(srcdirs) /dVERSION="$(build)" /bt=DOS $(defines)
!endif
!else ifeq buildtarget WIN32
copt = /w$(warnlevel) $(optsw) /zp1 /5r /s /dCPWIN /dWIN32 /fp5 /fpi87 /i=$(srcdirs) /dVERSION="$(build)" /bt=nt /zl $(defines)
!else
error "no or illegal target."
!endif

aopt = /ml /m5
waopt = /q $(defines)

!ifeq buildversion DEBUG
cdebopt = /d1 /d__DEBUG__ /dDEBUG /hw
adebopt = /zd
wadebopt = /d1
lopt = debug watcom all
dlllopt = debug watcom lines
!else
cdebopt =
adebopt =
lopt =
dlllopt =
!endif

.silent
.erase

!ifeq buildtarget DOS32
dlls = pfilesel.dll poutput.dll &
       devi.dll hardware.dll mchasm.dll inflate.dll sets.dll mmcmphlp.dll dos4gfix.dll cphelper.dll &
       arcrar.dll arczip.dll arcarj.dll arcumx.dll arcbpa.dll arclha.dll arcace.dll arcpaq.dll &
       playinp.dll playcda.dll playwav.dll playgmd.dll playgmi.dll &
       &
       cpiface.dll cphlpif.dll &
       &
       mcpbase.dll &
       devwgus.dll devwiw.dll devwmix.dll devwnone.dll &
       devwsb32.dll devwdgus.dll devwmixf.dll &
       &
       freverb.dll &
       freverb.dll freverb2.dll &
       &
       plrbase.dll &
       devpsb.dll  devpwss.dll devpnone.dll devppas.dll devpdisk.dll &
       devpgus.dll devpess.dll devpmpx.dll devpews.dll devpvxd.dll  &
       &
       smpbase.dll &
       devssb.dll devsgus.dll devsnone.dll devswss.dll &
       &
       loads3m.dll load669.dll loadmdl.dll loadams.dll &
       loaddmf.dll loadmtm.dll loadptm.dll loadult.dll loadokt.dll &
       &
       playmp.dll &
       playxm.dll &
       playit.dll &
       playsid.dll &
       fstypes.dll &
       playwma.dll &
       readasf.dll
exes = cp.exe imstest.exe
!else ifeq buildtarget WIN32
dlls = poutput.dll hardware.dll &
       pfilesel.dll devi.dll mchasm.dll inflate.dll sets.dll &
       arcrar.dll arczip.dll arcarj.dll arcumx.dll arcbpa.dll arclha.dll arcace.dll arcace.dll cphelper.dll &
       playwav.dll playgmd.dll playgmi.dll &
       &
       cpiface.dll cphlpif.dll &
       mcpbase.dll &
       &
       plrbase.dll &
       devpwin.dll devpdisk.dll devpmpx.dll &
       devwmix.dll &
       &
       loads3m.dll load669.dll loadmdl.dll loadams.dll &
       loaddmf.dll loadmtm.dll loadptm.dll loadult.dll loadokt.dll &
       &
       playmp.dll &
       playxm.dll &
       playit.dll &
       playsid.dll &
       fstypes.dll

exes = cp.dll cpwin.exe winims.exe
#       smpbase.dll
!else
error no or illegal target.
!endif

dest = $(exes) $(dlls)

.extensions: .was

.cpp: $(srcdirs)
.asm: $(srcdirs)
.was: $(srcdirs)

cpwin_desc = 'OpenCP/WiN32 launcher (c) 1998 Felix Domke'
cpwin_objs = cpwin.obj
cpwin_libs = cp.lib
cpwin_exp  = cpwin.exp

cp_desc = 'OpenCP (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
cp_exp  = cp.exp
!ifeq buildtarget DOS32
cp_objs = binfile.lib binfpak.obj binfdel.obj &
          dpmi.obj &
          plinkman.obj psetting.obj pmain.obj pmainasm.obj pindos.obj &
          usedll.obj err.obj
!else ifeq buildtarget WIN32
cp_objs = binfile.lib binfpak.obj binfdel.obj &
          plinkman.obj psetting.obj pmain.obj err.obj
!endif

playsid_desc = 'OpenCP SID Player (c) 1993-99 Michael Schwendt, Tammo Hinrichs'
playsid_objs = 6510_.obj 6581_.obj eeconfig.obj envelope.obj fformat_.obj &
               mixing.obj player_.obj psid_.obj raw_.obj samples.obj sidtune.obj &
               sidplay.obj sidpplay.obj sidpasm.obj
playsid_libs = poutput.lib plrbase.lib cpiface.lib sets.lib hardware.lib
playsid_exp  = playsid.exp

playmp_desc = 'OpenCP Audio MPEG Player (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
playmp_objs = mpplay.obj mppplay.obj ampdec.obj ampsynwc.obj ampsynth.obj amp1dec.obj amp2dec.obj amp3dec.obj amp3decw.obj
playmp_libs = plrbase.lib hardware.lib cpiface.lib sets.lib poutput.lib
playmp_exp  = playmp.exp

playwma_desc = 'OpenCP Audio WMA-ACM Loader (c) 1999 Felix Domke'
playwma_objs = wmaplay.obj wavpasm.obj wmapplay.obj loadpe.obj codec.obj mts.obj mtw.obj acm.obj
playwma_libs = plrbase.lib hardware.lib cpiface.lib sets.lib poutput.lib readasf.lib
playwma_exp  = playwma.exp

readasf_desc = 'OpenCP Audio ASF-reader (c) 1999 Felix Domke'
readasf_objs = asfread.obj
readasf_libs = plrbase.lib
readasf_exp  = readasf.exp

mmcmphlp_desc = 'OpenCP MMCMP Support Driver (c) 1994-99 Niklas Beisert'
mmcmphlp_objs = mmcmphlp.obj
mmcmphlp_libs =
mmcmphlp_exp  = mmcmphlp.exp

playxm_desc = 'OpenCP XM/MOD Player (c) 1995-99 Niklas Beisert, Tammo Hinrichs'
playxm_objs = xmplay.obj xmload.obj xmlmod.obj xmlmxm.obj xmrtns.obj xmpplay.obj xmptrak.obj xmpinst.obj
playxm_libs = poutput.lib mcpbase.lib cpiface.lib
playxm_exp  = playxm.exp

playit_desc = 'OpenCP IT Player (c) 1997-99 Tammo Hinrichs, Niklas Beisert'
playit_objs = itplay.obj itload.obj itpplay.obj itpinst.obj itptrak.obj itrtns.obj ittime.obj itsex.obj
playit_libs = poutput.lib mcpbase.lib cpiface.lib
playit_exp  = playit.exp

inflate_desc = 'OpenCP Inflate (c) 1994-99 Niklas Beisert'
inflate_objs = inflate.obj
inflate_libs =
inflate_exp  = inflate.exp

!ifeq buildtarget WIN32
poutput_desc = 'OpenCP Output Routines (c) 1994-99 Niklas Beisert, Tammo Hinrichs, Felix Domke'
poutput_objs = poutwin.obj pfonts.obj
poutput_libs =
poutput_exp  = poutput.exp
poutput_stdlibs = gdi32.lib kernel32.lib user32.lib
!else ifeq buildtarget DOS32
poutput_desc = 'OpenCP Output Routines (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
poutput_objs = poutput.obj pfonts.obj
poutput_exp  = poutput.exp
poutput_libs =
!endif

pfilesel_desc = 'OpenCP Fileselector (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
pfilesel_objs = pfilesel.obj pfsmain.obj fsptype.obj cphlpfs.obj
pfilesel_libs = poutput.lib inflate.lib cphelper.lib cp.lib
pfilesel_exp = pfilesel.exp

devwmix_desc = 'OpenCP Wavetable Device: Mixer (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
devwmix_objs = devwmix.obj dwmixa.obj dwmixqa.obj
!ifeq buildtarget WIN32
devwmix_libs = mcpbase.lib plrbase.lib
!else ifeq buildtarget DOS32
devwmix_libs = mcpbase.lib plrbase.lib hardware.lib
!endif
devwmix_exp  = devwmix.exp

devwmixf_desc = 'OpenCP Wavetable Device: FPU HighQuality Mixer (c) 1999 Tammo Hinrichs, Fabian Giesen'
devwmixf_objs = dwmixfa.obj devwmixf.obj
!ifeq buildtarget WIN32
devwmixf_libs = mcpbase.lib plrbase.lib
!else ifeq buildtarget DOS32
devwmixf_libs = mcpbase.lib plrbase.lib hardware.lib
!endif
devwmixf_exp  = devwmixf.exp

devwgus_desc = 'OpenCP Wavetable Device: Gravis UltraSound (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
devwgus_objs = devwgus.obj
devwgus_libs = mcpbase.lib hardware.lib
devwgus_exp  = devwgus.exp

devwsb32_desc = 'OpenCP Wavetable Device: SoundBlaster 32 (c) 1994-99 Niklas Beisert'
devwsb32_objs = devwsb32.obj
devwsb32_libs = mcpbase.lib hardware.lib
devwsb32_exp  = devwsb32.exp

devwnone_desc = 'OpenCP Wavetable Device: None (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
devwnone_objs = devwnone.obj dwnonea.obj
devwnone_libs = mcpbase.lib hardware.lib
devwnone_exp  = devwnone.exp

devwdgus_desc = 'OpenCP Wavetable Device: Double GUS (c) 1994-99 Niklas Beisert'
devwdgus_objs = devwdgus.obj
devwdgus_libs = mcpbase.lib hardware.lib
devwdgus_exp  = devwdgus.exp

devwiw_desc = 'OpenCP Wavetable Device: AMD InterWave (c) 1994-99 Tammo Hinrichs, Niklas Beisert'
devwiw_objs = devwiw.obj
devwiw_libs = mcpbase.lib hardware.lib
devwiw_exp  = devwiw.exp

reverb_desc = 'OpenCP Reverb (c) 1998-99 Tammo Hinrichs, Fabian Giesen'
reverb_objs = ireverb.obj
reverb_libs = mcpbase.lib
reverb_exp  = reverb.exp

freverb_desc = 'OpenCP Float-Reverb (c) 1998-99 Fabian Giesen, Tammo Hinrichs'
freverb_objs = freverb.obj
freverb_libs = mcpbase.lib
freverb_exp  = freverb.exp

freverb2_desc = 'OpenCP Float-Reverb (ASM version) (c) 1998-99 Fabian Giesen'
freverb2_objs = freverb2.obj freverba.obj
freverb2_libs = mcpbase.lib
freverb2_exp  = freverb2.exp

devpsb_desc = 'OpenCP Player Device: SoundBlaster (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
devpsb_objs = devpsb.obj
devpsb_libs = plrbase.lib hardware.lib
devpsb_exp  = devpsb.exp

devpvxd_desc = 'OpenCP Virtual Player Device: APC-Gateway (c) 1999 Felix Domke'
devpvxd_objs = devpvxd.obj vxd.obj
devpvxd_libs = plrbase.lib hardware.lib
devpvxd_exp  = devpvxd.exp

devpwin_desc = 'OpenCP Player Device: Windows MME *HACK* (c) 1998 Niklas Beisert, Felix Domke'
devpwin_objs = devpwin.obj
devpwin_libs = plrbase.lib
devpwin_stdlibs = winmm.lib kernel32.lib
devpwin_exp  = devpwin.exp

devpess_desc = 'OpenCP Player Device: ESS AudioDrive 688 (c) 1997-99 Oleg Prokhorov'
devpess_objs = devpess.obj
devpess_libs = plrbase.lib hardware.lib
devpess_exp  = devpess.exp

devpwss_desc = 'OpenCP Player Device: Windows Sound System (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
devpwss_objs = devpwss.obj
devpwss_libs = plrbase.lib hardware.lib
devpwss_exp  = devpwss.exp

devpgus_desc = 'OpenCP Player Device: Gravis UltraSound (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
devpgus_objs = devpgus.obj
devpgus_libs = plrbase.lib hardware.lib
devpgus_exp  = devpgus.exp

devppas_desc = 'OpenCP Player Device: Pro Audio Spectrum, (c) 1994-99 Niklas Beisert'
devppas_objs = devppas.obj
devppas_libs = plrbase.lib hardware.lib
devppas_exp  = devppas.exp

devpnone_desc = 'OpenCP Player Device: None (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
devpnone_objs = devpnone.obj
devpnone_libs = plrbase.lib hardware.lib
devpnone_exp  = devpnone.exp

devpdisk_desc = 'OpenCP Player Device: Disk Writer (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
devpdisk_objs = devpdisk.obj
devpdisk_libs = plrbase.lib hardware.lib
devpdisk_exp  = devpdisk.exp

devpmpx_desc = 'OpenCP Player Device: MPx Writer (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
devpmpx_objs = devpmpx.obj mpencode.obj mp1enc.obj mp2enc.obj mpifdct.obj mpdsynth.obj mppsy.obj
devpmpx_libs = plrbase.lib
devpmpx_exp  = devpmpx.exp

devpews_desc = 'OpenCP Player Device: Terratec AudioSystem EWS64 Codec (c) 1998 Tammo Hinrichs'
devpews_objs = devpews.obj
devpews_libs = hardware.lib plrbase.lib
devpews_exp  = devpews.exp

devssb_desc = 'OpenCP Sampler Device: SoundBlaster (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
devssb_objs = devssb.obj
devssb_libs = smpbase.lib hardware.lib
devssb_exp  = devssb.exp

devsgus_desc = 'OpenCP Sampler Device: Gravis UltraSound (c) 1994-99 Niklas Beisert'
devsgus_objs = devsgus.obj
devsgus_libs = smpbase.lib hardware.lib
devsgus_exp  = devsgus.exp

devsnone_desc = 'OpenCP Sampler Device: None (c) 1994-99 Niklas Beisert'
devsnone_objs = devsnone.obj
devsnone_libs = smpbase.lib hardware.lib
devsnone_exp  = devsnone.exp

devswss_desc = 'OpenCP Sampler Device: Windows Sound System (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
devswss_objs = devswss.obj
devswss_libs = smpbase.lib hardware.lib
devswss_exp  = devswss.exp

loads3m_desc = 'OpenCP Module Loader: *.S3M (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
loads3m_objs = gmdls3m.obj
loads3m_libs = playgmd.lib mcpbase.lib
loads3m_exp  = loads3m.exp

loadmtm_desc = 'OpenCP Module Loader: *.MTM (c) 1994-99 Niklas Beisert'
loadmtm_objs = gmdlmtm.obj
loadmtm_libs = playgmd.lib mcpbase.lib
loadmtm_exp  = loadmtm.exp

load669_desc = 'OpenCP Module Loader: *.669 (c) 1994-99 Niklas Beisert'
load669_objs = gmdl669.obj
load669_libs = playgmd.lib mcpbase.lib
load669_exp  = load669.exp

loadams_desc = 'OpenCP Module Loader: *.AMS (c) 1994-99 Niklas Beisert'
loadams_objs = gmdlams.obj
loadams_libs = playgmd.lib mcpbase.lib
loadams_exp  = loadams.exp

loadokt_desc = 'OpenCP Module Loader: *.OKT (c) 1994-99 Niklas Beisert'
loadokt_objs = gmdlokt.obj
loadokt_libs = playgmd.lib mcpbase.lib
loadokt_exp  = loadokt.exp

loadptm_desc = 'OpenCP Module Loader: *.PTM (c) 1994-99 Niklas Beisert'
loadptm_objs = gmdlptm.obj
loadptm_libs = playgmd.lib mcpbase.lib
loadptm_exp  = loadptm.exp

loadult_desc = 'OpenCP Module Loader: *.ULT (c) 1994-99 Niklas Beisert'
loadult_objs = gmdlult.obj
loadult_libs = playgmd.lib mcpbase.lib
loadult_exp  = loadult.exp

loaddmf_desc = 'OpenCP Module Loader: *.DMF (c) 1994-99 Niklas Beisert'
loaddmf_objs = gmdldmf.obj
loaddmf_libs = playgmd.lib mcpbase.lib
loaddmf_exp  = loaddmf.exp

loadmdl_desc = 'OpenCP Module Loader: *.MDL (c) 1994-99 Niklas Beisert'
loadmdl_objs = gmdlmdl.obj
loadmdl_libs = playgmd.lib mcpbase.lib
loadmdl_exp  = loadmdl.exp

mchasm_desc = 'OpenCP Player/Sampler Auxiliary Routines (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
mchasm_objs = mchasm.obj null.obj
mchasm_libs =
mchasm_exp  = mchasm.exp

fstypes_desc = 'OpenCP Module Detection (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
fstypes_objs = gmdptype.obj mpptype.obj wavptype.obj gmiptype.obj &
               sidptype.obj xmpptype.obj itpptype.obj fstypes.obj &
               wmaptype.obj
fstypes_libs = readasf.lib
fstypes_libs = readasf.lib
fstypes_exp  = fstypes.exp

playgmd_desc = 'OpenCP General Module Player (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
playgmd_objs = gmdrtns.obj gmdplay.obj &
               gmdpplay.obj gmdpinst.obj gmdpchan.obj gmdpdots.obj gmdptrak.obj
playgmd_libs = mcpbase.lib cpiface.lib poutput.lib
playgmd_exp  = playgmd.exp

playgmi_desc = 'OpenCP GUSPatch Midi Player (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
playgmi_objs = gmiplay.obj gmiinit.obj gmiload.obj &
               gmipinst.obj gmipchan.obj gmipdots.obj gmipplay.obj
playgmi_libs = cpiface.lib mcpbase.lib poutput.lib
playgmi_exp  = playgmi.exp

playcda_desc = 'OpenCP CD-Audio Player (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
playcda_objs = cdapplay.obj cdaplay.obj
playcda_libs = smpbase.lib cpiface.lib pfilesel.lib poutput.lib
playcda_exp  = playcda.exp

playinp_desc = 'OpenCP Input Player (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
playinp_objs = inppplay.obj
playinp_libs = smpbase.lib cpiface.lib pfilesel.lib poutput.lib
playinp_exp  = playinp.exp

playwav_desc = 'OpenCP Wave Player (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
playwav_objs = wavpplay.obj wavplay.obj wavpasm.obj
playwav_libs = plrbase.lib hardware.lib cpiface.lib sets.lib poutput.lib
playwav_exp  = playwav.exp

arcrar_desc = 'OpenCP Archive Reader: RAR (c) 1994-99 Niklas Beisert, Felix Domke'
arcrar_objs = arcrar.obj
arcrar_libs = pfilesel.lib
arcrar_exp  = arcrar.exp

arcarj_desc = 'OpenCP Archive Reader: ARJ (c) 1994-99 Niklas Beisert'
arcarj_objs = arcarj.obj
arcarj_libs = pfilesel.lib
arcarj_exp  = arcarj.exp

arczip_desc = 'OpenCP Archive Reader: ZIP (c) 1994-99 Niklas Beisert'
arczip_objs = arczip.obj
arczip_libs = pfilesel.lib inflate.lib
arczip_exp  = arczip.exp

arcumx_desc = 'OpenCP Archive Reader: Unreal Music Resource (c) 1998 Tammo Hinrichs'
arcumx_objs = arcumx.obj
arcumx_libs = pfilesel.lib
arcumx_exp  = arcumx.exp

arcbpa_desc = 'OpenCP Archive Reader: Death Rally Archives (c) 1998 Felix Domke'
arcbpa_objs = arcbpa.obj
arcbpa_libs = pfilesel.lib
arcbpa_exp  = arcbpa.exp

arclha_desc = 'OpenCP Archive Reader: LHA (c) 1998 Felix Domke'
arclha_objs = arclha.obj
arclha_libs = pfilesel.lib
arclha_exp  = arclha.exp

arcace_desc = 'OpenCP Archive Reader: ACE (c) 1998 Felix Domke (v2.0)'
arcace_objs = arcace.obj uac_dcpr.obj
arcace_libs = pfilesel.lib
arcace_exp  = arcace.exp

arcpaq_desc = 'OpenCP Archive Reader: PAQ (c) 1999 Felix Domke'
arcpaq_objs = arcpaq.obj
arcpaq_libs = pfilesel.lib
arcpaq_exp  = arcpaq.exp

sets_desc = 'OpenCP Sound Settings Auxiliary Routines (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
sets_objs = sets.obj
sets_libs = hardware.lib mchasm.lib
sets_exp  = sets.exp

smpbase_desc = 'OpenCP Sampler Devices System (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
smpbase_objs = sampler.obj devisamp.obj
smpbase_libs = hardware.lib devi.lib mchasm.lib pfilesel.lib
smpbase_exp  = smpbase.exp

plrbase_desc = 'OpenCP Player Devices System (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
plrbase_objs = player.obj deviplay.obj plrasm.obj
plrbase_libs = hardware.lib devi.lib mchasm.lib pfilesel.lib
plrbase_exp  = plrbase.exp

mcpbase_desc = 'OpenCP Wavetable Devices System (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
mcpbase_objs = mcp.obj deviwave.obj smpman.obj mcpedit.obj mix.obj mixasm.obj freq.obj
mcpbase_libs = devi.lib sets.lib pfilesel.lib poutput.lib mcpbase.lib
mcpbase_exp  = mcpbase.exp

!ifeq buildtarget WIN32
hardware_desc = 'OpenCP Win32-Timer Routines (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
hardware_objs = timerw.obj
hardware_exp  = hardwarw.exp
hardware_libs =
hardware_stdlibs = kernel32.lib
!else
hardware_desc = 'OpenCP IRQ, DMA and Timer Routines (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
hardware_objs = timer.obj irq.obj dma.obj poll.obj
hardware_exp  = hardware.exp
hardware_libs =
!endif


devi_desc = 'OpenCP Devices Auxiliary Routines (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
devi_objs = devigen.obj
devi_libs =
devi_exp  = devi.exp

!ifeq buildtarget WIN32
cpiface_desc = 'OpenCP Interface (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
cpiface_objs = cpiinst.obj cpichan.obj cpianal.obj fft.obj &
               &
               cpimvol.obj cpiface.obj cpimsg.obj cpitext.obj &
               cpitrack.obj cpiptype.obj volctrl.obj
cpiface_libs = pfilesel.lib poutput.lib inflate.lib hardware.lib
cpiface_exp  = cpifacew.exp
!else
cpiface_desc = 'OpenCP Interface (c) 1994-99 Niklas Beisert, Tammo Hinrichs'
cpiface_objs = cpiinst.obj cpichan.obj cpianal.obj cpigraph.obj fft.obj &
               gif.obj tga.obj cpidots.obj cpipic.obj cpiscope.obj &
               cpilinks.obj cpimvol.obj cpiface.obj cpimsg.obj cpitext.obj &
               cpitrack.obj cpiphase.obj cpikube.obj cpiptype.obj volctrl.obj
cpiface_libs = pfilesel.lib poutput.lib inflate.lib hardware.lib
cpiface_exp  = cpiface.exp
!endif

dos4gfix_desc = 'OpenCP IRQ 8-15 Bugfix for DOS4GW (c) 1994-99 Niklas Beisert'
dos4gfix_objs = d4gfinit.obj dos4gfix.obj
dos4gfix_libs =
dos4gfix_exp  = dos4gfix.exp

cphelper_desc = 'OpenCP help browser (c) 1998 Fabian Giesen'
cphelper_objs = cphelper.obj
cphelper_libs = poutput.lib inflate.lib
cphelper_exp  = cphelper.exp

cphlpif_desc = 'OpenCP help browser CPIFACE wrapper (c) 1998 Fabian Giesen'
cphlpif_objs = cphlpif.obj
cphlpif_libs = poutput.lib cpiface.lib cphelper.lib
cphlpif_exp  = cphlpif.exp


binfile_objs = binfile.obj binfilef.obj binfarc.obj binfstd.obj binfmem.obj

ims_objs = &
  ims.obj imsstuff.obj imssetup.obj mcp.obj imsmix.obj timer.obj irq.obj dma.obj freq.obj smpman.obj imsplayr.obj poll.obj &
  devwnone.obj devwsb32.obj devwiw.obj dwnonea.obj devpsb.obj devpwss.obj devpgus.obj devppas.obj devwmix.obj dwmixa.obj dwmixqa.obj devwgus.obj devwdgus.obj devpess.obj &
  xmrtns.obj xmload.obj xmlmod.obj xmlmxm.obj xmplay.obj xmtime.obj &
  gmdplay.obj gmdrtns.obj gmdlptm.obj gmdlams.obj gmdlmdl.obj gmdls3m.obj gmdtime.obj &
  itplay.obj itload.obj itrtns.obj ittime.obj itsex.obj

winims_desc = 'WinIMS'
winims_objs = &
  $(binfile_objs) &
  ims.obj imsstuff.obj mcp.obj imsmix.obj freq.obj smpman.obj imsplayr.obj &
  devwmix.obj dwmixa.obj dwmixqa.obj devpdisk.obj devpwin.obj &
  xmrtns.obj xmload.obj xmlmod.obj xmlmxm.obj xmplay.obj xmtime.obj &
  gmdplay.obj gmdrtns.obj gmdlptm.obj gmdlams.obj gmdlmdl.obj gmdls3m.obj gmdtime.obj &
  itplay.obj itload.obj itrtns.obj ittime.obj itsex.obj &
  winims.obj

in_ocp_desc = 'in_ocp'
in_ocp_objs = &
  $(binfile_objs) &
  ims.obj imsstuff.obj mcp.obj imsmix.obj freq.obj smpman.obj imsplayr.obj &
  devwmix.obj dwmixa.obj dwmixqa.obj devpdisk.obj devpwin.obj &
  xmrtns.obj xmload.obj xmlmod.obj xmlmxm.obj xmplay.obj xmtime.obj &
  gmdplay.obj gmdrtns.obj gmdlptm.obj gmdlams.obj gmdlmdl.obj gmdls3m.obj gmdtime.obj &
  itplay.obj itload.obj itrtns.obj ittime.obj itsex.obj &
  ocpamp.obj

!ifeq buildtarget WIN32
dlldeps = cp.lib windll.lib
makedll = *wlink $(dlllopt) @makedllw.lnk name $*.dll &
                 libpath %WATCOM%\lib386 libpath %WATCOM%\lib386\nt &
                 export { @exp\$($*_exp) } &
                 library { cp.lib $($*_libs) $($*_stdlibs) plib3r.lib math387r.lib }  &
                 file { $($*_objs) } &
                 option version = $(cp_ver) &
                 option description $($*_desc)
!else ifeq buildtarget DOS32
dlldeps = cp.lib dosdll.lib
makedll = *wlink $(dlllopt) @makedll.lnk name $*.dll &
                 libpath %WATCOM%\lib386 libpath %WATCOM%\lib386\dos &
                 export { @exp\$($*_exp) } &
                 library { cp.lib $($*_libs) $($*_stdlibs) math387r.lib plib3r.lib clib3r.lib } &
                 file { $($*_objs) } &
                 option version = $(cp_ver) &
                 option description $($*_desc)
!endif

makeimplib = wlib -q -b -c -n -p=16 $*.lib +$*.dll
makeimpfromexp = conv.exe exp\$($*_exp) $*.lib
makeimpfromexpdep = btools


all: .symbolic
!ifeq buildtarget DOS32
  echo.
  echo openCP will now be compiled. Take a rest. This will LAST.
  echo.
  set DOS4G=QUIET
  echo Step 1: Compile the tools needed for compiling...
  %make btools
  echo Step 2: Compile the files needed for CP.EXE.
  %make cp.exe
  echo Step 3: Compile and link all DLLs. You may make some Coffee now.
  %make base
  echo Step 4: Put all DLLs together into one big CP.PAK file.
  %make cp.pak
  echo Step 5: Compile and link everything concerning the IMS library
  %make imstest.exe
  echo Congratulations - you made it through the compilation. have fun now :)
  echo.
!else ifeq buildtarget WIN32
  echo.
  echo openCP will now be compiled. Take a rest. This will LAST.
  echo.
  echo Step 1: Compile the tools needed for compiling...
  %make btools
  echo Step 2: Compile the files needed for CP.DLL.
  %make cp.dll
  %make cpwin.exe
  echo Step 3: Compile and link all DLLs. You may make some Coffee now.
  %make base
  echo Congratulations - you made it through the compilation. have fun now :)
  echo But be aware that the Win-NT port is a complete hack (tmb)
  echo.
!endif

cp.pak: pack.exe cp.hlp playgmi\ultrasnd.ini $(dlls)
  echo Now putting all DLLs into CP.PAK...
  copy playgmi\ultrasnd.ini . > nul
  echo cp.lst
  %create cp.lst
  for %%i in ($(dlls)) do %append cp.lst %%i
  %append cp.lst cp.hlp
  %append cp.lst ultrasnd.ini
  echo $@
  set DOS4G=QUIET
  pack cp.lst > nul
  del cp.lst > nul
  del ultrasnd.ini > nul
  echo.


base: .symbolic $(dlls) vxdapc

btools: .symbolic conv.exe

conv.exe: btools\conv.cpp
  echo Now making the conv-build-tool...
  echo conv.obj
  *wpp386 btools\conv.cpp -zq
  echo $@
!ifeq defaultlibrarysfordlls NO
  *wlink option quiet name $*.exe $(lopt) system win32 &
         option { caseexact stack=16384 eliminate dosseg } &
         library $(defaultlibs) &
         file conv.obj
!else
  *wlink option quiet name $*.exe $(lopt) system win32 &
         option { caseexact stack=16384 eliminate dosseg } &
         file conv.obj
!endif
  echo.

install: .symbolic all
  echo.
  echo Now copying all distribution files into BIN\
  if not exist bin md bin
  if not exist bin\system md bin\system
  copy cphost.exe bin > nul
  copy vocp.dll bin > nul
  copy vapc.vxd bin\system > nul
  copy cp.exe bin > nul
  copy cp.pak bin > nul
  copy cp.ini bin > nul
  copy daposer.gif bin > nul
  copy ryg.gif bin > nul
  copy readme.txt bin > nul
  copy copying.txt bin > nul
  echo.
  echo you should now find a working openCP version in the BIN directory.
  echo.

clean: .symbolic
  echo Tidying all files generated during compilation...
  if exist *.tmp del *.tmp >nul
  if exist *.obj del *.obj >nul
  if exist *.lib del *.lib >nul
  if exist *.map del *.map >nul
  if exist *.err del *.err >nul
  if exist *.dll del *.dll >nul
  if exist *.bak del *.bak >nul
  cd vxdapc\apc
	if exist *.exp del *.exp >nul
	if exist *.lib del *.lib >nul
	if exist *.lnk del *.lnk >nul
	if exist *.obj del *.obj >nul
  cd ..\host
	if exist *.obj del *.obj >nul
	if exist *.lib del *.lib >nul
	if exist *.res del *.res >nul
	if exist vocp.dll del vocp.dll >nul
	if exist cphost.exe del cphost.exe >nul
  cd ..\..
  echo.


cleanall: .symbolic distclean

distclean: .symbolic
  %make clean
  echo Now tidying all executables/datafiles...
  if exist *.vxd del *.vxd >nul
  if exist *.exe del *.exe >nul
  if exist *.dat del *.dat >nul
  if exist $#*.* del $#*.* >nul
  if exist *~ del *~ >nul
  if exist cp.hlp del cp.hlp >nul
  if exist cp.pak del cp.pak >nul
  if exist cparcs.dat del cparcs.dat >nul
  if exist cpmodnfo.dat del cpmodnfo.dat >nul
  deltree /y bin >nul
  if exist *.cr del *.cr >nul
  echo.


cp.exe: $(cp_objs) pstub.exe exp\$(cp_exp)
  echo Now making the CP.EXE executable...
  echo compdate.cpp
  *wpp386 /zq boot\compdate.cpp /d__AUTHOR__="$(author)"
  echo $*.tmp
  %create $*.tmp
  %append $*.tmp $(lopt)
  %append $*.tmp file compdate.obj
  for %i in ($(cp_objs)) do %append $*.tmp file %i
  %append $*.tmp option { stub=pstub.exe caseexact map manglednames stack=16384 eliminate dosseg }
  %append $*.tmp export { @exp\$(cp_exp) }
  %append $*.tmp disable 1027
  %append $*.tmp name $*.exe
  %append $*.tmp system dos4g
!ifeq defaultlibrarysfordlls NO
  %append $*.tmp library $(defaultlibs)
!endif
  %append $*.tmp option version = $(cp_ver)
  %append $*.tmp option description $($*_desc)
  echo $@
  wlink @$*.tmp option quiet
  del $*.tmp > nul
  echo.

cp.dll: $(cp_objs) exp\$(cpwin_exp)
  echo Now making the CP.DLL executable...
  echo compdate.cpp
  *wpp386 /zq boot\compdate.cpp /d__AUTHOR__="$(author)"
  echo $*.tmp
  %create $*.tmp
  %append $*.tmp $(lopt)
  %append $*.tmp file compdate.obj
  for %i in ($(cp_objs)) do %append $*.tmp file %i
  %append $*.tmp option { caseexact map manglednames stack=16384 eliminate }
  %append $*.tmp export { @exp\$(cpwin_exp) }
  %append $*.tmp name $*.dll
  %append $*.tmp system nt_win
  %append $*.tmp runtime windows=4.0
!ifeq defaultlibrarysfordlls NO
  %append $*.tmp library $(defaultlibs)
!endif
  %append $*.tmp option version = $(cp_ver)
  %append $*.tmp option description $($*_desc)
  echo $@
  wlink @$*.tmp option quiet
  del $*.tmp > nul
  echo.

ims.lib: $(ims_objs)
  echo Now making the IMS main libraries...
  echo.
  echo $*.tmp
  %create $*.tmp
  for %i in ($($*_objs)) do @%append $*.tmp +%i
  echo $@
  wlib /q /b /n $*.lib @$*.tmp
  del $*.tmp >nul

winims.exe: $(winims_objs)
  echo Now making the WINIMS... :)
  echo.
  echo It won't work, I'm sure, but it's fun, anyway. #'
  echo.	
  echo $*.tmp	
  %create $*.tmp	
  %append $*.tmp $(lopt)	
  for %i in ($(winims_objs)) do %append $*.tmp file %i
  %append $*.tmp option { caseexact map manglednames stack=16384 eliminate dosseg }
  %append $*.tmp disable 1027
  %append $*.tmp name $*.exe
  %append $*.tmp system nt
#  %append $*.tmp library $(defaultlibs)
#  %append $*.tmp option version = $(cp_ver)
  %append $*.tmp option description $($*_desc)
  %append $*.tmp libpath %WATCOM%\lib386 libpath %WATCOM%\lib386\nt &

  echo $@
  wlink @$*.tmp option quiet
  del $*.tmp > nul
  echo.

in_ocp.dll: $(in_ocp_objs)
  echo Now making the in_ocp
  echo.
  echo $*.tmp
  %create $*.tmp
  %append $*.tmp $(lopt)
  for %i in ($(in_ocp_objs)) do %append $*.tmp file %i
  %append $*.tmp option { caseexact map manglednames stack=16384 eliminate dosseg }
  %append $*.tmp disable 1027
  %append $*.tmp name $*.dll
  %append $*.tmp system nt_dll
  %append $*.tmp library $(defaultlibs)
  %append $*.tmp option version = $(cp_ver)
  %append $*.tmp option description $($*_desc)
  %append $*.tmp libpath %WATCOM%\lib386 libpath %WATCOM%\lib386\nt &

  echo $@
  wlink @$*.tmp option quiet
  del $*.tmp > nul
  echo.

binfile.lib: $(binfile_objs)
  echo Now making the binfile class libraries for IMS...
  echo $*.tmp
  %create $*.tmp
  for %i in ($($*_objs)) do @%append $*.tmp +%i
  echo $@
  wlib /q /b /n $*.lib @$*.tmp
  del $*.tmp >nul


!ifeq buildtarget DOS32
cp.lib: exp\$(cp_exp) $(makeimpfromexpdep)
  echo $@
  set DOS4G=QUIET
  conv.exe exp\$(cp_exp) $*.lib cp
!else ifeq buildtarget WIN32
cp.lib: exp\$(cpwin_exp) $(makeimpfromexpdep)
  echo $@
  set DOS4G=QUIET
  conv.exe exp\$(cpwin_exp) $*.lib cp
!endif

pstub.exe: pstub.obj
  echo $@
  *wlink option quiet name pstub.exe system dos file pstub.obj

pstub.obj: pstub.cpp
  echo $@
  *wpp /zq /5 /onatmir /s /ms boot\pstub.cpp

.cpp.obj:
  echo $@
  *wpp386 /zq $(copt) $(cdebopt) $<

.asm.obj:
  echo $@
  tasm32 /t $(aopt) $(adebopt) $<

.was.obj:
  echo $@
  wasm $(waopt) $(wadebopt) $<

playmp.dll: $(dlldeps) exp\$(playmp_exp) $(playmp_objs) $(playmp_libs)
  echo $@
  $(makedll)

playwma.dll: $(dlldeps) exp\$(playwma_exp) $(playwma_objs) $(playwma_libs)
  echo $@
  $(makedll)

readasf.dll: $(dlldeps) exp\$(readasf_exp) $(readasf_objs) $(readasf_libs)
  echo $@
  $(makedll)

readasf.lib: exp\$(readasf_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

playsid.dll: $(dlldeps) exp\$(playsid_exp) $(playsid_objs) $(playsid_libs)
  echo $@
  $(makedll)

arcrar.dll: $(dlldeps) exp\$(arcrar_exp) $(arcrar_objs) $(arcrar_libs)
  echo $@
  $(makedll)

arczip.dll: $(dlldeps) exp\$(arczip_exp) $(arczip_objs) $(arczip_libs)
  echo $@
  $(makedll)

arcarj.dll: $(dlldeps) exp\$(arcarj_exp) $(arcarj_objs) $(arcarj_libs)
  echo $@
  $(makedll)

arcumx.dll: $(dlldeps) exp\$(arcumx_exp) $(arcumx_objs) $(arcumx_libs)
  echo $@
  $(makedll)

arcbpa.dll: $(dlldeps) exp\$(arcbpa_exp) $(arcbpa_objs) $(arcbpa_libs)
  echo $@
  $(makedll)

arclha.dll: $(dlldeps) exp\$(arclha_exp) $(arclha_objs) $(arclha_libs)
  echo $@
  $(makedll)

arcpaq.dll: $(dlldeps) exp\$(arcpaq_exp) $(arcpaq_objs) $(arcpaq_libs)
  echo $@
  $(makedll)

arcace.dll: $(dlldeps) exp\$(arcace_exp) $(arcace_objs) $(arcace_libs)
  echo $@
  $(makedll)

dos4gfix.dll: $(dlldeps) exp\$(dos4gfix_exp) $(dos4gfix_objs) $(dos4gfix_libs)
  echo $@
  $(makedll)

mmcmphlp.dll: $(dlldeps) exp\$(mmcmphlp_exp) $(mmcmphlp_objs) $(mmcmphlp_libs)
  echo $@
  $(makedll)

playxm.dll: $(dlldeps) exp\$(playxm_exp) $(playxm_objs) $(playxm_libs)
  echo $@
  $(makedll)

playit.dll: $(dlldeps) exp\$(playit_exp) $(playit_objs) $(playit_libs)
  echo $@
  $(makedll)

fstypes.dll: $(dlldeps) exp\$(fstypes_exp) $(fstypes_objs) $(fstypes_libs)
  echo $@
  $(makedll)

playgmd.dll: $(dlldeps) exp\$(playgmd_exp) $(playgmd_objs) $(playgmd_libs)
  echo $@
  $(makedll)

playgmd.lib: exp\$(playgmd_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

poutput.dll: $(dlldeps) exp\$(poutput_exp) $(poutput_objs) $(poutput_libs)
  echo $@
  $(makedll)

poutput.lib: exp\$(poutput_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

inflate.dll: $(dlldeps) exp\$(inflate_exp) $(inflate_objs) $(inflate_libs)
  echo $@
  $(makedll)
inflate.lib: exp\$(inflate_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

pfilesel.dll: $(dlldeps) exp\$(pfilesel_exp) $(pfilesel_objs) $(pfilesel_libs)
  echo $@
  $(makedll)
pfilesel.lib: exp\$(pfilesel_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

cpiface.dll: $(dlldeps) exp\$(cpiface_exp) $(cpiface_objs) $(cpiface_libs)
  echo $@
  $(makedll)

cpiface.lib: exp\$(cpiface_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

devi.dll: $(dlldeps) exp\$(devi_exp) $(devi_objs) $(devi_libs)
  echo $@
  $(makedll)
devi.lib: exp\$(devi_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

mchasm.dll: $(dlldeps) exp\$(mchasm_exp) $(mchasm_objs) $(mchasm_libs)
  echo $@
  $(makedll)
mchasm.lib: exp\$(mchasm_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

hardware.dll: $(dlldeps) exp\$(hardware_exp) $(hardware_objs) $(hardware_libs)
  echo $@
  $(makedll)
hardware.lib: exp\$(hardware_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

sets.dll: $(dlldeps) exp\$(sets_exp) $(sets_objs) $(sets_libs)
  echo $@
  $(makedll)
sets.lib: exp\$(sets_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

playgmi.dll: $(dlldeps) exp\$(playgmi_exp) $(playgmi_objs) $(playgmi_libs)
  echo $@
  $(makedll)

playcda.dll: $(dlldeps) exp\$(playcda_exp) $(playcda_objs) $(playcda_libs)
  echo $@
  $(makedll)

playinp.dll: $(dlldeps) exp\$(playinp_exp) $(playinp_objs) $(playinp_libs)
  echo $@
  $(makedll)

playwav.dll: $(dlldeps) exp\$(playwav_exp) $(playwav_objs) $(playwav_libs)
  echo $@
  $(makedll)

smpbase.dll: $(dlldeps) exp\$(smpbase_exp) $(smpbase_objs) $(smpbase_libs)
  echo $@
  $(makedll)
smpbase.lib: exp\$(smpbase_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

plrbase.dll: $(dlldeps) exp\$(plrbase_exp) $(plrbase_objs) $(plrbase_libs)
  echo $@
  $(makedll)
plrbase.lib: exp\$(plrbase_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

mcpbase.dll: $(dlldeps) exp\$(mcpbase_exp) $(mcpbase_objs) $(mcpbase_libs)
  echo $@

  %create mcpbase.tmp
  %append mcpbase.tmp $(makedll)
  $(makedll)

mcpbase.lib: exp\$(mcpbase_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

devwmix.dll: $(dlldeps) exp\$(devwmix_exp) $(devwmix_objs) $(devwmix_libs)
  echo $@
  $(makedll)

devwmixf.dll: $(dlldeps) exp\$(devwmixf_exp) $(devwmixf_objs) $(devwmixf_libs)
  echo $@
  $(makedll)

devwnone.dll: $(dlldeps) exp\$(devwnone_exp) $(devwnone_objs) $(devwnone_libs)
  echo $@
  $(makedll)

devpsb.dll: $(dlldeps) exp\$(devpsb_exp) $(devpsb_objs) $(devpsb_libs)
  echo $@
  $(makedll)

devpvxd.dll: $(dlldeps) exp\$(devpvxd_exp) $(devpvxd_objs) $(devpvxd_libs)
  echo $@
  $(makedll)

devpwin.dll: $(dlldeps) exp\$(devpwin_exp) $(devpwin_objs) $(devpwin_libs)
  echo $@
  $(makedll)

devpess.dll: $(dlldeps) exp\$(devpess_exp) $(devpess_objs) $(devpess_libs)
  echo $@
  $(makedll)

devssb.dll: $(dlldeps) exp\$(devssb_exp) $(devssb_objs) $(devssb_libs)
  echo $@
  $(makedll)

devwgus.dll: $(dlldeps) exp\$(devwgus_exp) $(devwgus_objs) $(devwgus_libs)
  echo $@
  $(makedll)

devsgus.dll: $(dlldeps) exp\$(devsgus_exp) $(devsgus_objs) $(devsgus_libs)
  echo $@
  $(makedll)

devsnone.dll: $(dlldeps) exp\$(devsnone_exp) $(devsnone_objs) $(devsnone_libs)
  echo $@
  $(makedll)

devwsb32.dll: $(dlldeps) exp\$(devwsb32_exp) $(devwsb32_objs) $(devwsb32_libs)
  echo $@
  $(makedll)

devpwss.dll: $(dlldeps) exp\$(devpwss_exp) $(devpwss_objs) $(devpwss_libs)
  echo $@
  $(makedll)

devpnone.dll: $(dlldeps) exp\$(devpnone_exp) $(devpnone_objs) $(devpnone_libs)
  echo $@
  $(makedll)

devppas.dll: $(dlldeps) exp\$(devppas_exp) $(devppas_objs) $(devppas_libs)
  echo $@
  $(makedll)

devpdisk.dll: $(dlldeps) exp\$(devpdisk_exp) $(devpdisk_objs) $(devpdisk_libs)
  echo $@
  $(makedll)

devpews.dll: $(dlldeps) exp\$(devpews_exp) $(devpews_objs) $(devpews_libs)
  echo $@
  $(makedll)

devpmpx.dll: $(dlldeps) exp\$(devpmpx_exp) $(devpmpx_objs) $(devpmpx_libs)
  echo $@
  $(makedll)

devswss.dll: $(dlldeps) exp\$(devswss_exp) $(devswss_objs) $(devswss_libs)
  echo $@
  $(makedll)

devpgus.dll: $(dlldeps) exp\$(devpgus_exp) $(devpgus_objs) $(devpgus_libs)
  echo $@
  $(makedll)

devwiw.dll: $(dlldeps) exp\$(devwiw_exp) $(devwiw_objs) $(devwiw_libs)
  echo $@
  $(makedll)

reverb.dll: $(dlldeps) $(reverb_objs) $(reverb_libs) exp\$(reverb_exp)
  echo $@
  $(makedll)

freverb.dll: $(dlldeps) $(freverb_objs) $(freverb_libs) exp\$(freverb_exp)
  echo $@
  $(makedll)

freverb2.dll: $(dlldeps) $(freverb2_objs) $(freverb2_libs) exp\$(freverb2_exp)
  echo $@
  $(makedll)

devwdgus.dll: $(dlldeps) exp\$(devwdgus_exp) $(devwdgus_objs) $(devwdgus_libs)
  echo $@
  $(makedll)

loads3m.dll: $(dlldeps) exp\$(loads3m_exp) $(loads3m_objs) $(loads3m_libs)
  echo $@
  $(makedll)

loadmtm.dll: $(dlldeps) exp\$(loadmtm_exp) $(loadmtm_objs) $(loadmtm_libs)
  echo $@
  $(makedll)

load669.dll: $(dlldeps) exp\$(load669_exp) $(load669_objs) $(load669_libs)
  echo $@
  $(makedll)

loadams.dll: $(dlldeps) exp\$(loadams_exp) $(loadams_objs) $(loadams_libs)
  echo $@
  $(makedll)

loadokt.dll: $(dlldeps) exp\$(loadokt_exp) $(loadokt_objs) $(loadokt_libs)
  echo $@
  $(makedll)

loadptm.dll: $(dlldeps) exp\$(loadptm_exp) $(loadptm_objs) $(loadptm_libs)
  echo $@
  $(makedll)

loadult.dll: $(dlldeps) exp\$(loadult_exp) $(loadult_objs) $(loadult_libs)
  echo $@
  $(makedll)

loaddmf.dll: $(dlldeps) exp\$(loaddmf_exp) $(loaddmf_objs) $(loaddmf_libs)
  echo $@
  $(makedll)

loadmdl.dll: $(dlldeps) exp\$(loadmdl_exp) $(loadmdl_objs) $(loadmdl_libs)
  echo $@
  $(makedll)

cphelper.dll: $(dlldeps) exp\$(cphelper_exp) $(cphelper_objs) $(cphelper_libs)
  echo $@
  $(makedll)

cphelper.lib: exp\$(cphelper_exp) $(makeimpfromexpdep)
  echo $@
  $(makeimpfromexp)

cphlpif.dll: $(dlldeps) exp\cphlpif.exp $(cphlpif_objs) $(cphlpif_libs)
  echo $@
  $(makedll)

cpwin.exe: $(cpwin_objs) $(cpwin_libs)
  echo $@
  *wlink file $*.obj name $*.exe system nt library $(cpwin_libs)

windll.lib: dllstart.obj dmain.obj libmain.obj libterm.obj fpfix.obj dosdll\$(runtime) dosdll\sgdef086.obj
  echo $@
  wlib -q -n $*.lib +dosdll\sgdef086.obj +dosdll\$(runtime) +dllstart.obj +dmain.obj +libmain.obj +libterm.obj +fpfix.obj
  # -q -b -n -c -p=16

dosdll.lib: dllstart.obj dmain.obj libmain.obj libterm.obj fpfix.obj dosdll\$(runtime) dosdll\sgdef086.obj
  echo $@
  wlib -q -n $*.lib +dosdll\sgdef086.obj +dosdll\$(runtime) +dllstart.obj +dmain.obj +libmain.obj +libterm.obj +fpfix.obj
  # -q -b -n -c -p=16


goodies\helpc\ocphhc.exe: goodies\helpc\ocphhc.cpp goodies\helpc\adler32.c goodies\helpc\compress.c goodies\helpc\deflate.c goodies\helpc\trees.c goodies\helpc\zutil.c
  echo Now compiling the help compiler
  echo ocphhc.obj
  wpp386 -otexan goodies\helpc\ocphhc.cpp -zq
  echo adler32.obj
  wcc386 -otexan goodies\helpc\adler32.c -zq
  echo compress.obj
  wcc386 -otexan goodies\helpc\compress.c -zq
  echo deflate.obj
  wcc386 -otexan goodies\helpc\deflate.c -zq
  echo trees.obj
  wcc386 -otexan goodies\helpc\trees.c -zq
  echo zutil.obj
  wcc386 -otexan goodies\helpc\zutil.c -zq
  echo ocphhc.exe
  %write ocphhc.lnk file { ocphhc.obj adler32.obj compress.obj deflate.obj trees.obj zutil.obj }
  %write ocphhc.lnk name goodies\helpc\ocphhc.exe
  %write ocphhc.lnk library goodies\helpc\zlib.lib
  %write ocphhc.lnk system dos4g option { map dosseg quiet }
  wlink @ocphhc.lnk
  del ocphhc.lnk > nul

cp.hlp: doc\opencp.dox goodies\helpc\ocphhc.exe
   echo Now compiling the help file
   echo $@
   goodies\helpc\ocphhc doc\opencp.dox cp.hlp >nul


pack.exe: pack.obj
  echo Now making the Quake PAK generator...
  echo $@
  *wlink option quiet name $*.exe $(lopt) system dos4g &
         option { caseexact stack=16384 eliminate dosseg } &
         library $(defaultlibs) &
         file pack.obj
  echo.

imstest.exe: ims.lib binfile.lib imstest.obj
  echo Now making the IMS example program...
  echo $@
  %create $*.tmp
  %append $*.tmp $(lopt)
!ifeq defaultlibrarysfordlls NO
  %append $*.tmp library $(defaultlibs)
!endif
  %append $*.tmp file { ims.lib binfile.lib imstest.obj }
  %append $*.tmp disable 1027 system dos4g name $*.exe
  wlink @$*.tmp
  del $*.tmp > nul
# option quiet disable 1027
  echo.

vxdapc: .symbolic cphost.exe vapc.vxd vocp.dll

vapc.vxd:
  cd vxdapc\apc
  copy vapc.vxd ..\.. > nul
  cd ..\..

cphost.exe: vxdapc\host\cphost.cpp
  echo making cphost.exe
  cd vxdapc\host
  wmake -h
  copy cphost.exe ..\.. > nul
  cd ..\..

vocp.dll: vxdapc\host\devpdx5.cpp vxdapc\host\ocp.cpp
  echo making vxdapc dlls
  cd vxdapc\host
  wmake -h
  copy vocp.dll ..\.. > nul
  cd ..\..
