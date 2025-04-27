PKGDIR  ?= ..
L4DIR   ?= $(PKGDIR)/../../..

include $(PKGDIR)/contrib.inc

ADAINCLUDE_PATH=$(OBJ_BASE)/include/contrib/ada/adainclude

vpath %.ads $(ADAINCLUDE_PATH)
vpath %.adb $(ADAINCLUDE_PATH)

TARGET  = libgnat.a

objext=
# ==========================
# The following lists are more or less directly from Makefile.rtl
# We do not use objext, but it does no harm ...
GNATRTL_TASKING_OBJS= \
  a-dispat$(objext) \
  a-dynpri$(objext) \
  a-interr$(objext) \
  a-intnam$(objext) \
  a-reatim$(objext) \
  a-retide$(objext) \
  a-rttiev$(objext) \
  a-synbar$(objext) \
  a-sytaco$(objext) \
  a-tasatt$(objext) \
  a-taside$(objext) \
  a-tasini$(objext) \
  a-taster$(objext) \
  g-boubuf$(objext) \
  g-boumai$(objext) \
  g-semaph$(objext) \
  g-signal$(objext) \
  g-tastus$(objext) \
  g-thread$(objext) \
  s-inmaop$(objext) \
  s-interr$(objext) \
  s-intman$(objext) \
  s-mudido$(objext) \
  s-osinte$(objext) \
  s-proinf$(objext) \
  s-putaim$(objext) \
  s-solita$(objext) \
  s-stusta$(objext) \
  s-taasde$(objext) \
  s-tadeca$(objext) \
  s-tadert$(objext) \
  s-taenca$(objext) \
  s-taprob$(objext) \
  s-taprop$(objext) \
  s-tarest$(objext) \
  s-tasdeb$(objext) \
  s-tasinf$(objext) \
  s-tasini$(objext) \
  s-taskin$(objext) \
  s-taspri$(objext) \
  s-tasque$(objext) \
  s-tasren$(objext) \
  s-tasres$(objext) \
  s-tassta$(objext) \
  s-tasuti$(objext) \
  s-tataat$(objext) \
  s-tpinop$(objext) \
  s-tpoben$(objext) \
  s-tpobop$(objext) \
  s-tposen$(objext) \
  thread$(objext) \
  $(EXTRA_GNATRTL_TASKING_OBJS)

# Objects that require IEEE Float
GNATRTL_ALTIVEC_OBJS= \
  g-allein$(objext) \
  g-alleve$(objext) \
  g-altcon$(objext) \
  g-altive$(objext) \
  g-alveop$(objext) \
  g-alvety$(objext) \
  g-alvevi$(objext)

# Objects needed for non-tasking
GNATRTL_NONTASKING_OBJS= \
  a-assert$(objext) \
  a-btgbso$(objext) \
  a-calari$(objext) \
  a-calcon$(objext) \
  a-caldel$(objext) \
  a-calend$(objext) \
  a-calfor$(objext) \
  a-catizo$(objext) \
  a-cbdlli$(objext) \
  a-cbhama$(objext) \
  a-cbhase$(objext) \
  a-cbmutr$(objext) \
  a-cborma$(objext) \
  a-cborse$(objext) \
  a-cbprqu$(objext) \
  a-cbsyqu$(objext) \
  a-cdlili$(objext) \
  a-cfdlli$(objext) \
  a-cfhama$(objext) \
  a-cfhase$(objext) \
  a-cfinve$(objext) \
  a-cforma$(objext) \
  a-cforse$(objext) \
  a-cgaaso$(objext) \
  a-cgarso$(objext) \
  a-cgcaso$(objext) \
  a-chacon$(objext) \
  a-chahan$(objext) \
  a-charac$(objext) \
  a-chlat1$(objext) \
  a-chlat9$(objext) \
  a-chtgbk$(objext) \
  a-chtgbo$(objext) \
  a-chtgke$(objext) \
  a-chtgop$(objext) \
  a-chzla1$(objext) \
  a-chzla9$(objext) \
  a-cidlli$(objext) \
  a-cihama$(objext) \
  a-cihase$(objext) \
  a-cimutr$(objext) \
  a-ciorma$(objext) \
  a-ciormu$(objext) \
  a-ciorse$(objext) \
  a-clrefi$(objext) \
  a-coboho$(objext) \
  a-cobove$(objext) \
  a-cofove$(objext) \
  a-cofuba$(objext) \
  a-cofuma$(objext) \
  a-cofuse$(objext) \
  a-cofuve$(objext) \
  a-cogeso$(objext) \
  a-cohama$(objext) \
  a-cohase$(objext) \
  a-cohata$(objext) \
  a-coinho$(objext) \
  a-coinve$(objext) \
  a-colien$(objext) \
  a-colire$(objext) \
  a-comlin$(objext) \
  a-comutr$(objext) \
  a-conhel$(objext) \
  a-contai$(objext) \
  a-convec$(objext) \
  a-coorma$(objext) \
  a-coormu$(objext) \
  a-coorse$(objext) \
  a-coprnu$(objext) \
  a-costso$(objext) \
  a-coteio$(objext) \
  a-crbltr$(objext) \
  a-crbtgk$(objext) \
  a-crbtgo$(objext) \
  a-crdlli$(objext) \
  a-csquin$(objext) \
  a-cuprqu$(objext) \
  a-cusyqu$(objext) \
  a-cwila1$(objext) \
  a-cwila9$(objext) \
  a-decima$(objext) \
  a-dhfina$(objext) \
  a-diocst$(objext) \
  a-direct$(objext) \
  a-direio$(objext) \
  a-dirval$(objext) \
  a-einuoc$(objext) \
  a-elchha$(objext) \
  a-envvar$(objext) \
  a-except$(objext) \
  a-exctra$(objext) \
  a-finali$(objext) \
  a-flteio$(objext) \
  a-fwteio$(objext) \
  a-fzteio$(objext) \
  a-inteio$(objext) \
  a-ioexce$(objext) \
  a-iteint$(objext) \
  a-iwteio$(objext) \
  a-izteio$(objext) \
  a-lcteio$(objext) \
  a-lfteio$(objext) \
  a-lfwtio$(objext) \
  a-lfztio$(objext) \
  a-liteio$(objext) \
  a-liwtio$(objext) \
  a-liztio$(objext) \
  a-llctio$(objext) \
  a-llftio$(objext) \
  a-llfwti$(objext) \
  a-llfzti$(objext) \
  a-llitio$(objext) \
  a-lliwti$(objext) \
  a-llizti$(objext) \
  a-llltio$(objext) \
  a-lllwti$(objext) \
  a-lllzti$(objext) \
  a-locale$(objext) \
  a-nagefl$(objext) \
  a-naliop$(objext) \
  a-nallfl$(objext) \
  a-nalofl$(objext) \
  a-nashfl$(objext) \
  a-nbnbig$(objext) \
  a-nbnbin$(objext) \
  a-nbnbre$(objext) \
  a-ncelfu$(objext) \
  a-ngcefu$(objext) \
  a-ngcoar$(objext) \
  a-ngcoty$(objext) \
  a-ngelfu$(objext) \
  a-ngrear$(objext) \
  a-nlcefu$(objext) \
  a-nlcoar$(objext) \
  a-nlcoty$(objext) \
  a-nlelfu$(objext) \
  a-nllcar$(objext) \
  a-nllcef$(objext) \
  a-nllcty$(objext) \
  a-nllefu$(objext) \
  a-nllrar$(objext) \
  a-nlrear$(objext) \
  a-nscefu$(objext) \
  a-nscoty$(objext) \
  a-nselfu$(objext) \
  a-nuauco$(objext) \
  a-nuaufl$(objext) \
  a-nubinu$(objext) \
  a-nucoar$(objext) \
  a-nucoty$(objext) \
  a-nudira$(objext) \
  a-nuelfu$(objext) \
  a-nuflra$(objext) \
  a-numaux$(objext) \
  a-numeri$(objext) \
  a-nurear$(objext) \
  a-rbtgbk$(objext) \
  a-rbtgbo$(objext) \
  a-rbtgso$(objext) \
  a-sbecin$(objext) \
  a-sbhcin$(objext) \
  a-sblcin$(objext) \
  a-scteio$(objext) \
  a-secain$(objext) \
  a-sequio$(objext) \
  a-sfecin$(objext) \
  a-sfhcin$(objext) \
  a-sflcin$(objext) \
  a-sfteio$(objext) \
  a-sfwtio$(objext) \
  a-sfztio$(objext) \
  a-shcain$(objext) \
  a-siocst$(objext) \
  a-siteio$(objext) \
  a-siwtio$(objext) \
  a-siztio$(objext) \
  a-slcain$(objext) \
  a-ssicst$(objext) \
  a-ssitio$(objext) \
  a-ssiwti$(objext) \
  a-ssizti$(objext) \
  a-stboha$(objext) \
  a-stbubo$(objext) \
  a-stbufi$(objext) \
  a-stbufo$(objext) \
  a-stbuun$(objext) \
  a-stbuut$(objext) \
  a-stfiha$(objext) \
  a-stmaco$(objext) \
  a-storio$(objext) \
  a-strbou$(objext) \
  a-stream$(objext) \
  a-strfix$(objext) \
  a-strhas$(objext) \
  a-string$(objext) \
  a-strmap$(objext) \
  a-strsea$(objext) \
  a-strsto$(objext) \
  a-strsup$(objext) \
  a-strunb$(objext) \
  a-ststbo$(objext) \
  a-ststio$(objext) \
  a-ststun$(objext) \
  a-sttebu$(objext) \
  a-stunau$(objext) \
  a-stunha$(objext) \
  a-stuten$(objext) \
  a-stwibo$(objext) \
  a-stwifi$(objext) \
  a-stwiha$(objext) \
  a-stwima$(objext) \
  a-stwise$(objext) \
  a-stwisu$(objext) \
  a-stwiun$(objext) \
  a-stzbou$(objext) \
  a-stzfix$(objext) \
  a-stzhas$(objext) \
  a-stzmap$(objext) \
  a-stzsea$(objext) \
  a-stzsup$(objext) \
  a-stzunb$(objext) \
  a-suecin$(objext) \
  a-suenco$(objext) \
  a-suenst$(objext) \
  a-suewst$(objext) \
  a-suezst$(objext) \
  a-suhcin$(objext) \
  a-sulcin$(objext) \
  a-suteio$(objext) \
  a-swbwha$(objext) \
  a-swfwha$(objext) \
  a-swmwco$(objext) \
  a-swunau$(objext) \
  a-swuwha$(objext) \
  a-swuwti$(objext) \
  a-szbzha$(objext) \
  a-szfzha$(objext) \
  a-szmzco$(objext) \
  a-szunau$(objext) \
  a-szuzha$(objext) \
  a-szuzti$(objext) \
  a-tags$(objext) \
  a-teioed$(objext) \
  a-textio$(objext) \
  a-tgdico$(objext) \
  a-tiboio$(objext) \
  a-ticoau$(objext) \
  a-ticoio$(objext) \
  a-tideau$(objext) \
  a-tideio$(objext) \
  a-tienau$(objext) \
  a-tienio$(objext) \
  a-tifiau$(objext) \
  a-tifiio$(objext) \
  a-tiflau$(objext) \
  a-tiflio$(objext) \
  a-tigeau$(objext) \
  a-tiinau$(objext) \
  a-tiinio$(objext) \
  a-timoio$(objext) \
  a-tiocst$(objext) \
  a-tirsfi$(objext) \
  a-titest$(objext) \
  a-tiunio$(objext) \
  a-unccon$(objext) \
  a-uncdea$(objext) \
  a-undesu$(objext) \
  a-wichha$(objext) \
  a-wichun$(objext) \
  a-widcha$(objext) \
  a-witeio$(objext) \
  a-wrstfi$(objext) \
  a-wtcoau$(objext) \
  a-wtcoio$(objext) \
  a-wtcstr$(objext) \
  a-wtdeau$(objext) \
  a-wtdeio$(objext) \
  a-wtedit$(objext) \
  a-wtenau$(objext) \
  a-wtenio$(objext) \
  a-wtfiau$(objext) \
  a-wtfiio$(objext) \
  a-wtflau$(objext) \
  a-wtflio$(objext) \
  a-wtgeau$(objext) \
  a-wtinau$(objext) \
  a-wtinio$(objext) \
  a-wtmoio$(objext) \
  a-wttest$(objext) \
  a-wwboio$(objext) \
  a-wwunio$(objext) \
  a-zchara$(objext) \
  a-zchhan$(objext) \
  a-zchuni$(objext) \
  a-zrstfi$(objext) \
  a-ztcoau$(objext) \
  a-ztcoio$(objext) \
  a-ztcstr$(objext) \
  a-ztdeau$(objext) \
  a-ztdeio$(objext) \
  a-ztedit$(objext) \
  a-ztenau$(objext) \
  a-ztenio$(objext) \
  a-ztexio$(objext) \
  a-ztfiau$(objext) \
  a-ztfiio$(objext) \
  a-ztflau$(objext) \
  a-ztflio$(objext) \
  a-ztgeau$(objext) \
  a-ztinau$(objext) \
  a-ztinio$(objext) \
  a-ztmoio$(objext) \
  a-zttest$(objext) \
  a-zzboio$(objext) \
  a-zzunio$(objext) \
  ada$(objext) \
  calendar$(objext) \
  directio$(objext) \
  g-arrspl$(objext) \
  g-awk$(objext) \
  g-binenv$(objext) \
  g-brapre$(objext) \
  g-bubsor$(objext) \
  g-busora$(objext) \
  g-busorg$(objext) \
  g-byorma$(objext) \
  g-bytswa$(objext) \
  g-calend$(objext) \
  g-casuti$(objext) \
  g-catiio$(objext) \
  g-cgi$(objext)    \
  g-cgicoo$(objext) \
  g-cgideb$(objext) \
  g-comlin$(objext) \
  g-comver$(objext) \
  g-crc32$(objext) \
  g-ctrl_c$(objext) \
  g-curexc$(objext) \
  g-debpoo$(objext) \
  g-debuti$(objext) \
  g-decstr$(objext) \
  g-deutst$(objext) \
  g-diopit$(objext) \
  g-dirope$(objext) \
  g-dynhta$(objext) \
  g-dyntab$(objext) \
  g-encstr$(objext) \
  g-enutst$(objext) \
  g-excact$(objext) \
  g-except$(objext) \
  g-exctra$(objext) \
  g-expect$(objext) \
  g-exptty$(objext) \
  g-flocon$(objext) \
  g-forstr$(objext) \
  g-graphs$(objext) \
  g-heasor$(objext) \
  g-hesora$(objext) \
  g-hesorg$(objext) \
  g-htable$(objext) \
  g-io$(objext) \
  g-io_aux$(objext) \
  g-lists$(objext) \
  g-locfil$(objext) \
  g-mbdira$(objext) \
  g-mbflra$(objext) \
  g-md5$(objext) \
  g-memdum$(objext) \
  g-moreex$(objext) \
  g-os_lib$(objext) \
  g-pehage$(objext) \
  g-rannum$(objext) \
  g-regexp$(objext) \
  g-regpat$(objext) \
  g-rewdat$(objext) \
  g-sechas$(objext) \
  g-sehamd$(objext) \
  g-sehash$(objext) \
  g-sercom$(objext) \
  g-sestin$(objext) \
  g-sets$(objext) \
  g-sha1$(objext) \
  g-sha224$(objext) \
  g-sha256$(objext) \
  g-sha384$(objext) \
  g-sha512$(objext) \
  g-shsh32$(objext) \
  g-shsh64$(objext) \
  g-shshco$(objext) \
  g-souinf$(objext) \
  g-spchge$(objext) \
  g-speche$(objext) \
  g-spipat$(objext) \
  g-spitbo$(objext) \
  g-spogwa$(objext) \
  g-sptabo$(objext) \
  g-sptain$(objext) \
  g-sptavs$(objext) \
  g-strhas$(objext) \
  g-string$(objext) \
  g-strspl$(objext) \
  g-table$(objext) \
  g-tasloc$(objext) \
  g-timsta$(objext) \
  g-traceb$(objext) \
  g-trasym$(objext) \
  g-tty$(objext) \
  g-u3spch$(objext) \
  g-utf_32$(objext) \
  g-wispch$(objext) \
  g-wistsp$(objext) \
  g-zspche$(objext) \
  g-zstspl$(objext) \
  gnat$(objext) \
  i-c$(objext) \
  i-cexten$(objext) \
  i-cobol$(objext) \
  i-cpoint$(objext) \
  i-cstrea$(objext) \
  i-cstrin$(objext) \
  i-fortra$(objext) \
  i-pacdec$(objext) \
  interfac$(objext) \
  ioexcept$(objext) \
  machcode$(objext) \
  s-addima$(objext) \
  s-addope$(objext) \
  s-aoinar$(objext) \
  s-aomoar$(objext) \
  s-aotase$(objext) \
  s-aridou$(objext) \
  s-arit32$(objext) \
  s-arit64$(objext) \
  s-assert$(objext) \
  s-atacco$(objext) \
  s-atocou$(objext) \
  s-atoope$(objext) \
  s-atopex$(objext) \
  s-atopri$(objext) \
  s-auxdec$(objext) \
  s-bignum$(objext) \
  s-bitfie$(objext) \
  s-bitops$(objext) \
  s-bituti$(objext) \
  s-boarop$(objext) \
  s-boustr$(objext) \
  s-bytswa$(objext) \
  s-carsi8$(objext) \
  s-carun8$(objext) \
  s-casi16$(objext) \
  s-casi32$(objext) \
  s-casi64$(objext) \
  s-casuti$(objext) \
  s-caun16$(objext) \
  s-caun32$(objext) \
  s-caun64$(objext) \
  s-chepoo$(objext) \
  s-commun$(objext) \
  s-conca2$(objext) \
  s-conca3$(objext) \
  s-conca4$(objext) \
  s-conca5$(objext) \
  s-conca6$(objext) \
  s-conca7$(objext) \
  s-conca8$(objext) \
  s-conca9$(objext) \
  s-crc32$(objext)  \
  s-crtl$(objext)   \
  s-dfmkio$(objext) \
  s-dfmopr$(objext) \
  s-dgmgop$(objext) \
  s-diflio$(objext) \
  s-diflmk$(objext) \
  s-digemk$(objext) \
  s-diinio$(objext) \
  s-dilomk$(objext) \
  s-dim$(objext)    \
  s-dimkio$(objext) \
  s-dimmks$(objext) \
  s-direio$(objext) \
  s-dlmkio$(objext) \
  s-dlmopr$(objext) \
  s-dmotpr$(objext) \
  s-dourea$(objext) \
  s-dsaser$(objext) \
  s-elaall$(objext) \
  s-excdeb$(objext) \
  s-except$(objext) \
  s-exctab$(objext) \
  s-exctra$(objext) \
  s-exnflt$(objext) \
  s-exnint$(objext) \
  s-exnlfl$(objext) \
  s-exnllf$(objext) \
  s-exnlli$(objext) \
  s-expint$(objext) \
  s-explli$(objext) \
  s-expllu$(objext) \
  s-expmod$(objext) \
  s-exponn$(objext) \
  s-exponr$(objext) \
  s-expont$(objext) \
  s-exponu$(objext) \
  s-expuns$(objext) \
  s-fatflt$(objext) \
  s-fatgen$(objext) \
  s-fatlfl$(objext) \
  s-fatllf$(objext) \
  s-ficobl$(objext) \
  s-filatt$(objext) \
  s-fileio$(objext) \
  s-finmas$(objext) \
  s-finroo$(objext) \
  s-flocon$(objext) \
  s-fode32$(objext) \
  s-fode64$(objext) \
  s-fofi32$(objext) \
  s-fofi64$(objext) \
  s-fore_d$(objext) \
  s-fore_f$(objext) \
  s-forrea$(objext) \
  s-gearop$(objext) \
  s-genbig$(objext) \
  s-geveop$(objext) \
  s-gloloc$(objext) \
  s-htable$(objext) \
  s-imageb$(objext) \
  s-imaged$(objext) \
  s-imagef$(objext) \
  s-imagei$(objext) \
  s-imagen$(objext) \
  s-imager$(objext) \
  s-imageu$(objext) \
  s-imagew$(objext) \
  s-imde32$(objext) \
  s-imde64$(objext) \
  s-imen16$(objext) \
  s-imen32$(objext) \
  s-imenu8$(objext) \
  s-imfi32$(objext) \
  s-imfi64$(objext) \
  s-imgbiu$(objext) \
  s-imgboo$(objext) \
  s-imgcha$(objext) \
  s-imgflt$(objext) \
  s-imgint$(objext) \
  s-imglfl$(objext) \
  s-imgllb$(objext) \
  s-imgllf$(objext) \
  s-imglli$(objext) \
  s-imgllu$(objext) \
  s-imgllw$(objext) \
  s-imgrea$(objext) \
  s-imguns$(objext) \
  s-imguti$(objext) \
  s-imgwch$(objext) \
  s-imgwiu$(objext) \
  s-io$(objext)     \
  s-llflex$(objext) \
  s-maccod$(objext) \
  s-mantis$(objext) \
  s-mastop$(objext) \
  s-memcop$(objext) \
  s-memory$(objext) \
  s-multip$(objext) \
  s-os_lib$(objext) \
  s-oscons$(objext) \
  s-osprim$(objext) \
  s-pack03$(objext) \
  s-pack05$(objext) \
  s-pack06$(objext) \
  s-pack07$(objext) \
  s-pack09$(objext) \
  s-pack10$(objext) \
  s-pack11$(objext) \
  s-pack12$(objext) \
  s-pack13$(objext) \
  s-pack14$(objext) \
  s-pack15$(objext) \
  s-pack17$(objext) \
  s-pack18$(objext) \
  s-pack19$(objext) \
  s-pack20$(objext) \
  s-pack21$(objext) \
  s-pack22$(objext) \
  s-pack23$(objext) \
  s-pack24$(objext) \
  s-pack25$(objext) \
  s-pack26$(objext) \
  s-pack27$(objext) \
  s-pack28$(objext) \
  s-pack29$(objext) \
  s-pack30$(objext) \
  s-pack31$(objext) \
  s-pack33$(objext) \
  s-pack34$(objext) \
  s-pack35$(objext) \
  s-pack36$(objext) \
  s-pack37$(objext) \
  s-pack38$(objext) \
  s-pack39$(objext) \
  s-pack40$(objext) \
  s-pack41$(objext) \
  s-pack42$(objext) \
  s-pack43$(objext) \
  s-pack44$(objext) \
  s-pack45$(objext) \
  s-pack46$(objext) \
  s-pack47$(objext) \
  s-pack48$(objext) \
  s-pack49$(objext) \
  s-pack50$(objext) \
  s-pack51$(objext) \
  s-pack52$(objext) \
  s-pack53$(objext) \
  s-pack54$(objext) \
  s-pack55$(objext) \
  s-pack56$(objext) \
  s-pack57$(objext) \
  s-pack58$(objext) \
  s-pack59$(objext) \
  s-pack60$(objext) \
  s-pack61$(objext) \
  s-pack62$(objext) \
  s-pack63$(objext) \
  s-parame$(objext) \
  s-parint$(objext) \
  s-pehage$(objext) \
  s-pooglo$(objext) \
  s-pooloc$(objext) \
  s-poosiz$(objext) \
  s-powflt$(objext) \
  s-powlfl$(objext) \
  s-powllf$(objext) \
  s-purexc$(objext) \
  s-putima$(objext) \
  s-rannum$(objext) \
  s-ransee$(objext) \
  s-regexp$(objext) \
  s-regpat$(objext) \
  s-resfil$(objext) \
  s-restri$(objext) \
  s-rident$(objext) \
  s-rpc$(objext)    \
  s-scaval$(objext) \
  s-secsta$(objext) \
  s-sequio$(objext) \
  s-shabig$(objext) \
  s-shasto$(objext) \
  s-soflin$(objext) \
  s-soliin$(objext) \
  s-spsufi$(objext) \
  s-stache$(objext) \
  s-stalib$(objext) \
  s-statxd$(objext) \
  s-stausa$(objext) \
  s-stoele$(objext) \
  s-stopoo$(objext) \
  s-stposu$(objext) \
  s-stratt$(objext) \
  s-strhas$(objext) \
  s-string$(objext) \
  s-ststop$(objext) \
  s-tasloc$(objext) \
  s-traceb$(objext) \
  s-traent$(objext) \
  s-trasym$(objext) \
  s-unstyp$(objext) \
  s-utf_32$(objext) \
  s-vade32$(objext) \
  s-vade64$(objext) \
  s-vaen16$(objext) \
  s-vaen32$(objext) \
  s-vaenu8$(objext) \
  s-vafi32$(objext) \
  s-vafi64$(objext) \
  s-valboo$(objext) \
  s-valcha$(objext) \
  s-valflt$(objext) \
  s-valint$(objext) \
  s-vallfl$(objext) \
  s-valllf$(objext) \
  s-vallli$(objext) \
  s-valllu$(objext) \
  s-valrea$(objext) \
  s-valued$(objext) \
  s-valuef$(objext) \
  s-valuei$(objext) \
  s-valuen$(objext) \
  s-valuer$(objext) \
  s-valueu$(objext) \
  s-valuns$(objext) \
  s-valuti$(objext) \
  s-valwch$(objext) \
  s-veboop$(objext) \
  s-vector$(objext) \
  s-vercon$(objext) \
  s-wchcnv$(objext) \
  s-wchcon$(objext) \
  s-wchjis$(objext) \
  s-wchstw$(objext) \
  s-wchwts$(objext) \
  s-widboo$(objext) \
  s-widcha$(objext) \
  s-widenu$(objext) \
  s-widint$(objext) \
  s-widlli$(objext) \
  s-widllu$(objext) \
  s-widthi$(objext) \
  s-widthu$(objext) \
  s-widuns$(objext) \
  s-widwch$(objext) \
  s-wwdcha$(objext) \
  s-wwdenu$(objext) \
  s-wwdwch$(objext) \
  sequenio$(objext) \
  system$(objext) \
  text_io$(objext) \
  unchconv$(objext) \
  unchdeal$(objext) \
  $(GNATRTL_ALTIVEC_OBJS) \
  $(GNATRTL_SOCKETS_OBJS) \
  $(EXTRA_GNATRTL_NONTASKING_OBJS)

#GNATRTL_SOCKETS_OBJS = g-soccon$(objext) g-socket$(objext) g-socthi$(objext) \
#  g-soliop$(objext) g-sothco$(objext) g-socpol$(objext)
GNATRTL_SOCKETS_OBJS =

TRASYM_DWARF_COMMON_OBJS = s-objrea$(objext) s-dwalin$(objext) s-mmap$(objext) \
  s-mmosin$(objext)

TRASYM_DWARF_UNIX_OBJS = $(TRASYM_DWARF_COMMON_OBJS) s-mmauni$(objext)

# Objects needed for 128-bit types
GNATRTL_128BIT_OBJS = \
  s-arit128$(objext) \
  s-casi128$(objext) \
  s-caun128$(objext) \
  s-exnllli$(objext) \
  s-expllli$(objext) \
  s-explllu$(objext) \
  s-fode128$(objext) \
  s-fofi128$(objext) \
  s-imde128$(objext) \
  s-imfi128$(objext) \
  s-imglllb$(objext) \
  s-imgllli$(objext) \
  s-imglllu$(objext) \
  s-imglllw$(objext) \
  s-pack65$(objext)  \
  s-pack66$(objext)  \
  s-pack67$(objext)  \
  s-pack68$(objext)  \
  s-pack69$(objext)  \
  s-pack70$(objext)  \
  s-pack71$(objext)  \
  s-pack72$(objext)  \
  s-pack73$(objext)  \
  s-pack74$(objext)  \
  s-pack75$(objext)  \
  s-pack76$(objext)  \
  s-pack77$(objext)  \
  s-pack78$(objext)  \
  s-pack79$(objext)  \
  s-pack80$(objext)  \
  s-pack81$(objext)  \
  s-pack82$(objext)  \
  s-pack83$(objext)  \
  s-pack84$(objext)  \
  s-pack85$(objext)  \
  s-pack86$(objext)  \
  s-pack87$(objext)  \
  s-pack88$(objext)  \
  s-pack89$(objext)  \
  s-pack90$(objext)  \
  s-pack91$(objext)  \
  s-pack92$(objext)  \
  s-pack93$(objext)  \
  s-pack94$(objext)  \
  s-pack95$(objext)  \
  s-pack96$(objext)  \
  s-pack97$(objext)  \
  s-pack98$(objext)  \
  s-pack99$(objext)  \
  s-pack100$(objext) \
  s-pack101$(objext) \
  s-pack102$(objext) \
  s-pack103$(objext) \
  s-pack104$(objext) \
  s-pack105$(objext) \
  s-pack106$(objext) \
  s-pack107$(objext) \
  s-pack108$(objext) \
  s-pack109$(objext) \
  s-pack110$(objext) \
  s-pack111$(objext) \
  s-pack112$(objext) \
  s-pack113$(objext) \
  s-pack114$(objext) \
  s-pack115$(objext) \
  s-pack116$(objext) \
  s-pack117$(objext) \
  s-pack118$(objext) \
  s-pack119$(objext) \
  s-pack120$(objext) \
  s-pack121$(objext) \
  s-pack122$(objext) \
  s-pack123$(objext) \
  s-pack124$(objext) \
  s-pack125$(objext) \
  s-pack126$(objext) \
  s-pack127$(objext) \
  s-vade128$(objext) \
  s-vafi128$(objext) \
  s-valllli$(objext) \
  s-vallllu$(objext) \
  s-widllli$(objext) \
  s-widlllu$(objext)

# Based on x86 Linux

EXTRA_GNATRTL_NONTASKING_OBJS-amd64 += $(GNATRTL_128BIT_OBJS)
EXTRA_GNATRTL_NONTASKING_OBJS += $(EXTRA_GNATRTL_NONTASKING_OBJS-$(BUILD_ARCH))
EXTRA_GNATRTL_NONTASKING_OBJS += g-sse g-ssvety
EXTRA_GNATRTL_NONTASKING_OBJS += $(TRASYM_DWARF_UNIX_OBJS)
EXTRA_GNATRTL_TASKING_OBJS += a-exetim

# TODO: Not for arm exception handling mechanism!
EXTRA_GNATRTL_NONTASKING_OBJS += g-cppexc s-excmac
EXTRA_LIBGNAT_OBJS+=raise-gcc.o

LIBGNAT_OBJS = adadecode.o adaint.o argv.o aux-io.o       \
  cal.o cio.o cstreams.o ctrl_c.o         \
  env.o errno.o exit.o expect.o final.o rtfinal.o rtinit.o    \
  init.o initialize.o locales.o mkdir.o         \
  raise.o seh_init.o socket.o sysdep.o          \
  targext.o terminals.o tracebak.o          \
  $(EXTRA_LIBGNAT_OBJS)

# L4Re
# ======================
# Now use the gnat definitions

SRC_C = $(patsubst %.o,%.c,$(LIBGNAT_OBJS))

GNATRTL_OBJS = $(GNATRTL_NONTASKING_OBJS)
GNARLRTL_OBJS = $(GNATRTL_TASKING_OBJS)

GNAT_ADB_CANDIDATES = $(addprefix $(ADAINCLUDE_PATH)/,$(patsubst %,%.adb,$(GNATRTL_OBJS)))
GNAT_ADB_FILES = $(patsubst $(ADAINCLUDE_PATH)/%,%,$(wildcard $(ADB_CANDIDATES)))
# All files, that did not have an adb match
GNAT_ADS_FILES = $(patsubst %,%.ads,$(filter-out $(ADB_FILES:.adb=),$(GNATRTL_OBJS)))

SRC_ADA_libgnat.a = $(GNAT_ADB_FILES) $(GNAT_ADS_FILES)


# === libgnatr === Experimental and untested. Use at own risk
#TARGET += libgnarl.a
#GNARL_ADB_CANDIDATES = $(addprefix $(ADAINCLUDE_PATH)/,$(patsubst %,%.adb,$(GNARLRTL_OBJS)))
#GNARL_ADB_FILES = $(patsubst $(ADAINCLUDE_PATH)/%,%,$(wildcard $(ADB_CANDIDATES)))
## All files, that did not have an adb match
#GNARL_ADS_FILES = $(patsubst %,%.ads,$(filter-out $(ADB_FILES:.adb=),$(GNARLRTL_OBJS)))
#
#SRC_ADA_libgnarl.a = $(GNARL_ADB_FILES) $(GNARL_ADS_FILES)
#
#PRIVATE_INCDIR_thread.c += $(ADAINCLUDE_PATH)
#vpath thread.c $(CONTRIB_DIR)/libgnarl


REQUIRES_LIBS  += libgcc libc_support_misc libc_be_sig
PRIVATE_INCDIR_raise-gcc.c += $(SRC_DIR)/include
PRIVATE_INCDIR_adaint.c    += $(CONTRIB_DIR)

ADA_ALI_INSTALL_PATH=$(OBJ_BASE)/include/contrib/ada/adalib

vpath %.c $(CONTRIB_DIR)


ADAC = $(CC)

# Only search the builddir and main contrib dir, not the dir of the src file
# This avoids pulling in "generic" variants of ads files we installed specific
# versions for
ADACFLAGS += -I- -I$(CONTRIB_DIR)
# Build in rts compile mode
ADACFLAGS += -gnatpg
DEFINES += -DIN_RTS -DSTANDALONE -DL4

include $(L4DIR)/mk/lib.mk

# Upstream does not declare prototypes before function definition
WARNINGS += -Wno-missing-prototypes -Wno-missing-declarations
# Upstream contrib code has unused parameters
WARNINGS += -Wno-unused-parameter
# Upstream contrib code has non-strict prototypes
WARNINGS += -Wno-strict-prototypes
# Upstream code uses bad casts
WARNINGS += -Wno-bad-cast

# Empty body in non-debug builds
WARNINGS_raise-gcc.c += -Wno-empty-body
# Upstream uses casts flagged as bad
WARNINGS_raise-gcc.c += -Wno-bad-function-cast
WARNINGS_tracebak.c += -Wno-bad-function-cast

# Upstream defines this on their own
DEFINES_terminals.c += -U_GNU_SOURCE
DEFINES_adaint.c += -U_GNU_SOURCE -U__USE_GNU

# We do not have an RTS when building this
ADACFLAGS := $(filter-out --RTS%,$(ADACFLAGS))

GNATGEN_ADB=s-excmac:__gcc s-trasym:__dwarf s-mmosin:__unix s-osprim:__posix
GNATGEN_ADS=s-mmauni:__long a-coinho__shared.adb

#libgnat/s-trasym.o: libgnat/s-tsmona.adb

# 1: Name 2: Variant 3: Ext
define gen_name
libgnat/$(1).o: .stamp-$1.$3
libgnat/$1.$3 .stamp-$1.$3 &: $(CONTRIB_DIR)/libgnat/$1$2.$3
	$(MKDIR) $$(@D)
	$(VERBOSE)$(LN) -s $$< libgnat/$1.$3
	touch .stamp-$1.$3
endef

$(foreach f,$(GNATGEN_ADS),$(eval $(call gen_name,$(word 1,$(subst :,$(BID_SPACE),$(f))),$\
                                           $(word 2,$(subst :,$(BID_SPACE),$(f))),$\
                                           ads)))
$(foreach f,$(GNATGEN_ADB),$(eval $(call gen_name,$(word 1,$(subst :,$(BID_SPACE),$(f))),$\
                                           $(word 2,$(subst :,$(BID_SPACE),$(f))),$\
                                           adb)))

libgnat/a-nuflra.o: libgnat/interfac.o

