ZIPBASE ?= $(PWD)/../out/zip
IMAGES := img

all: force_look
	mkdir -p $(ZIPBASE)/newlib/Release
	cp -R *.html *.js *.json *.css $(ZIPBASE)
	mkdir -p $(ZIPBASE)
	cp -R js $(ZIPBASE)
	mkdir -p $(ZIPBASE)/$(IMAGES)
	cp -R $(IMAGES) $(ZIPBASE)
	rm $(ZIPBASE)/js/bootstrap/js/bootstrap.js
	rm $(ZIPBASE)/js/bootstrap/css/bootstrap.css
	rm $(ZIPBASE)/js/bootstrap/css/bootstrap-theme.css

clean:
	rm -f $(ZIPBASE)/*.html
	rm -f $(ZIPBASE)/*.js
	rm -f $(ZIPBASE)/*.json
	rm -rf $(ZIPBASE)/js
	rm -rf $(ZIPBASE)/$(IMAGES)

force_look:
	true
