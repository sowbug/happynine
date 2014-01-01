ZIPBASE ?= $(PWD)/../out/zip
BOOTSTRAP := bootstrap-3.0.3
IMAGES := img

all: force_look
	mkdir -p $(ZIPBASE)/newlib/Release
	cp -R *.html *.js *.json *.css *.map $(ZIPBASE)
	mkdir -p $(ZIPBASE)/$(BOOTSTRAP)
	cp -R $(BOOTSTRAP) $(ZIPBASE)
	mkdir -p $(ZIPBASE)/$(IMAGES)
	cp -R $(IMAGES) $(ZIPBASE)

clean:
	rm -f $(ZIPBASE)/*.html
	rm -f $(ZIPBASE)/*.js
	rm -f $(ZIPBASE)/*.json
	rm -f $(ZIPBASE)/$(BOOTSTRAP)
	rm -f $(ZIPBASE)/$(IMAGES)

force_look:
	true
