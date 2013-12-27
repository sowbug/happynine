BOOTSTRAP := bootstrap-3.0.3

all: force_look
	mkdir -p $(ZIPBASE)/newlib/Release
	cp -R *.html *.js *.json $(ZIPBASE)
	mkdir -p $(ZIPBASE)/$(BOOTSTRAP)
	cp -R $(BOOTSTRAP) $(ZIPBASE)

clean:
	rm -f $(ZIPBASE)/*.html
	rm -f $(ZIPBASE)/*.js
	rm -f $(ZIPBASE)/*.json
	rm -f $(ZIPBASE)/$(BOOTSTRAP)

force_look:
	true
