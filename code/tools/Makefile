SUBDIRS = oc2sec sec2ts zpipe \
	tsfilter tsloop tsmodder tsnullfiller tsnullshaper \
	tspcrmeasure tspidmapper esaudio2pes esaudioinfo \
	esvideompeg2pes esvideompeg2info pes2es pesaudio2ts \
	pesvideo2ts pesinfo tsstamp \
	ts2pes mpe2sec tscbrmuxer mpeg2videovbv tstdt i13942ts \
	tsvbr2cbr tsfixcc tsudpreceive tsudpsend dsmcc-receive \
	tspcrstamp tstcpreceive tstcpsend tstimeout \
	tstimedwrite tsinputswitch tsdoubleoutput pes2txt \
	tsoutputswitch tsccc txt2pes ts2sec \
	pesdata2ts tsmask tsorts tsororts tspcrrestamp \
	eitsecfilter eitsecmapper m2ts2cbrts \
	ts2m2ts tsdiscont eitsecactualtoanother 

subdirs:        $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@
.PHONY:		subdirs $(SUBDIRS)

all: subdirs

install: all
	@for T in $(SUBDIRS); do make -C $$T $@; done

clean:
	@for T in $(SUBDIRS); do make -C $$T $@; done
